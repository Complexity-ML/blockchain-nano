"""Pure, bounded BSD/GNU ar object comparison. No file/process/CLI operations."""
import hashlib
import re

_MAGIC = b"!<arch>\n"
_BSD_INDEX = {"__.SYMDEF", "__.SYMDEF SORTED", "__.SYMDEF_64", "__.SYMDEF_64 SORTED"}
_MAX_ARCHIVE = 256 * 1024 * 1024
_MAX_MEMBERS = 4096


def _need(ok, reason):
    if not ok:
        raise ValueError(reason)


def _number(field, base=10, empty=False):
    text = field.strip(b" ")
    _need(bool(text) or empty, "empty archive numeric field")
    if not text:
        return 0
    _need(re.fullmatch(b"[0-9]+" if base == 10 else b"[0-7]+", text) is not None,
          "invalid archive numeric field")
    return int(text, base)


def _object_name(encoded):
    _need(0 < len(encoded) <= 4096, "object name length")
    _need(re.fullmatch(rb"[A-Za-z0-9_+.-]+[.]o", encoded) is not None,
          "unsupported object name/path or metadata reference")
    return encoded.decode("ascii")


def _gnu_names(body):
    _need(0 < len(body) <= 2 * 1024 * 1024, "GNU name table bound")
    names, at = {}, 0
    while at < len(body):
        # GNU/LLVM may include one LF of even-size padding inside this member.
        if body[at:] == b"\n":
            _need(at % 2 == 1, "invalid GNU name table padding")
            at += 1
            break
        end = body.find(b"/\n", at)
        _need(end >= at and len(names) < _MAX_MEMBERS, "unterminated GNU name table")
        names[at] = _object_name(body[at:end])
        at = end + 2
    _need(at == len(body) and bool(names), "empty or malformed GNU name table")
    _need(len(set(names.values())) == len(names), "duplicate GNU table name")
    return names


def archive_object_rows(raw):
    """Return ordered {name, bytes, sha256} rows; reject unsupported encodings."""
    _need(type(raw) is bytes and 8 < len(raw) <= _MAX_ARCHIVE and raw[:8] == _MAGIC,
          "bounded regular ar required; thin/fat/AIX archives unsupported")
    at, pending, names = 8, [], None
    symbol_index = False
    while at < len(raw):
        _need(len(pending) < _MAX_MEMBERS and at + 60 <= len(raw), "archive header/member bound")
        header = raw[at:at + 60]
        _need(header[58:] == b"`\n", "invalid archive header terminator")
        for a, b in ((16, 28), (28, 34), (34, 40)):
            _number(header[a:b], empty=True)
        _number(header[40:48], 8, empty=True)
        size = _number(header[48:58])
        start, end = at + 60, at + 60 + size
        _need(end <= len(raw), "archive member exceeds payload")
        token = header[:16].rstrip(b" ")
        _need(bool(token) and not token.startswith(b" "), "invalid archive name field")
        payload = memoryview(raw)[start:end]
        reference = None
        if token.startswith(b"#1/"):
            length = _number(token[3:])
            _need(0 < length <= min(size, 4096), "BSD extended-name bound")
            encoded = raw[start:start + length]
            name_bytes = encoded.rstrip(b"\0")
            _need(bool(name_bytes) and b"\0" not in name_bytes, "BSD embedded name NUL")
            payload = memoryview(raw)[start + length:end]
            try:
                name = name_bytes.decode("ascii")
            except UnicodeDecodeError as error:
                raise ValueError("non-ASCII BSD name unsupported") from error
        elif token in (b"/", b"/SYM64/", b"//"):
            name = token.decode("ascii")
        elif token.startswith(b"/"):
            reference = _number(token[1:])
            name = None
        else:
            name_bytes = token[:-1] if token.endswith(b"/") else token
            try:
                name = name_bytes.decode("ascii")
            except UnicodeDecodeError as error:
                raise ValueError("non-ASCII archive name unsupported") from error
        if name == "//":
            _need(names is None and not pending, "duplicate/late GNU name table")
            names = _gnu_names(payload.tobytes())
        elif name in _BSD_INDEX or name in ("/", "/SYM64/"):
            _need(not symbol_index and not pending and size > 0, "duplicate/late/empty archive index")
            symbol_index = True
        else:
            if reference is None:
                name = _object_name(name.encode("ascii"))
            else:
                _need(names is not None and reference in names, "GNU offset is not an entry start")
                name = names[reference]
            _need(len(payload) > 0, "empty object member")
            pending.append({"name": name, "bytes": len(payload),
                            "sha256": hashlib.sha256(payload).hexdigest()})
        if size & 1:
            _need(end < len(raw) and raw[end:end + 1] == b"\n", "missing archive alignment LF")
        at = end + (size & 1)
    _need(at == len(raw) and bool(pending), "trailing archive bytes or no objects")
    _need(len({row["name"] for row in pending}) == len(pending), "duplicate resolved object name")
    return pending


def _pin(value):
    _need(type(value) is dict and set(value) == {"bytes", "sha256"}, "pin shape")
    _need(type(value["bytes"]) is int and 0 < value["bytes"] <= _MAX_ARCHIVE and
          type(value["sha256"]) is str and re.fullmatch(r"[0-9a-f]{64}", value["sha256"]),
          "pin value")
    return {"bytes": value["bytes"], "sha256": value["sha256"]}


def prove_object_replacement(before, after, *, object_name, qualified_old_rows,
                              old_archive_pin, new_object_pin, expected_members):
    """Caller supplies authenticated bytes/rows; this function performs no I/O."""
    _need(type(expected_members) is int and 1 <= expected_members <= _MAX_MEMBERS,
          "explicit qualified object count required")
    _need(type(object_name) is str, "explicit replacement object name required")
    selected = _object_name(object_name.encode("ascii"))
    old_pin, replacement = _pin(old_archive_pin), _pin(new_object_pin)
    _need(type(before) is bytes and len(before) == old_pin["bytes"] and
          hashlib.sha256(before).hexdigest() == old_pin["sha256"], "old archive pin mismatch")
    _need(type(qualified_old_rows) is list and len(qualified_old_rows) == expected_members,
          "qualified old rows required")
    qualified = []
    for row in qualified_old_rows:
        _need(type(row) is dict and set(row) == {"name", "bytes", "sha256"} and
              type(row["name"]) is str, "qualified row shape")
        name = _object_name(row["name"].encode("ascii"))
        qualified.append(dict(name=name, **_pin({"bytes": row["bytes"], "sha256": row["sha256"]})))
    old, new = archive_object_rows(before), archive_object_rows(after)
    _need(old == qualified, "old archive differs from qualified object rows/order")
    _need(len(new) == expected_members and [r["name"] for r in new] == [r["name"] for r in old],
          "new object names/count/order changed")
    _need(sum(r["name"] == selected for r in old) == 1, "one exact selected object required")
    changed = [a["name"] for a, b in zip(old, new) if a != b]
    _need(changed == [selected], "replacement must change only the selected object")
    row = next(r for r in new if r["name"] == selected)
    _need({k: row[k] for k in ("bytes", "sha256")} == replacement, "new selected object pin mismatch")
    return {"format": "nano-single-object-archive-member-proof-v1", "passed": True,
            "members": expected_members, "unchangedMembers": expected_members - 1,
            "replacedMembers": changed, "oldArchive": old_pin,
            "newArchive": {"bytes": len(after), "sha256": hashlib.sha256(after).hexdigest()},
            "newObject": replacement, "memberOrderPreserved": True,
            "symbolTablesSemanticallyValidated": False, "archiveFileNineVerifiedByThisFunction": False,
            "productionQualified": False}


def prove_object_replacements(before, after, *, replacements, qualified_old_rows,
                               old_archive_pin, expected_members):
    """Prove an explicit replacement set; selected bodies may remain identical."""
    _need(type(expected_members) is int and 1 <= expected_members <= _MAX_MEMBERS,
          "explicit qualified object count required")
    _need(type(replacements) is dict and 1 <= len(replacements) <= expected_members,
          "bounded nonempty replacement set required")
    selected = {}
    for name, value in replacements.items():
        _need(type(name) is str, "replacement object name")
        decoded = _object_name(name.encode("ascii"))
        _need(decoded not in selected, "duplicate selected object")
        selected[decoded] = _pin(value)
    old_pin = _pin(old_archive_pin)
    _need(type(before) is bytes and len(before) == old_pin["bytes"] and
          hashlib.sha256(before).hexdigest() == old_pin["sha256"], "old archive pin mismatch")
    _need(type(qualified_old_rows) is list and len(qualified_old_rows) == expected_members,
          "qualified old rows required")
    qualified = []
    for row in qualified_old_rows:
        _need(type(row) is dict and set(row) == {"name", "bytes", "sha256"} and
              type(row["name"]) is str, "qualified row shape")
        name = _object_name(row["name"].encode("ascii"))
        qualified.append(dict(name=name, **_pin({"bytes": row["bytes"], "sha256": row["sha256"]})))
    old, new = archive_object_rows(before), archive_object_rows(after)
    _need(old == qualified, "old archive differs from qualified object rows/order")
    _need(len(new) == expected_members and [r["name"] for r in new] == [r["name"] for r in old],
          "new object names/count/order changed")
    names = {r["name"] for r in old}
    _need(set(selected) <= names, "selected object absent from qualified archive")
    for previous, current in zip(old, new):
        name = current["name"]
        if name in selected:
            _need({k: current[k] for k in ("bytes", "sha256")} == selected[name],
                  "selected new object pin mismatch")
        else:
            _need(previous == current, "unselected object body changed")
    changed = [a["name"] for a, b in zip(old, new) if a != b]
    return {"format": "nano-object-set-archive-member-proof-v1", "passed": True,
            "members": expected_members, "selectedReplacementCount": len(selected),
            "replacementMembers": [r["name"] for r in old if r["name"] in selected],
            "unchangedOutsideReplacementSet": expected_members - len(selected),
            "actualChangedMembers": changed, "actualChangedCount": len(changed),
            "oldArchive": old_pin,
            "newArchive": {"bytes": len(after), "sha256": hashlib.sha256(after).hexdigest()},
            "newObjects": selected, "memberOrderPreserved": True,
            "symbolTablesSemanticallyValidated": False, "archiveFileNineVerifiedByThisFunction": False,
            "productionQualified": False}
