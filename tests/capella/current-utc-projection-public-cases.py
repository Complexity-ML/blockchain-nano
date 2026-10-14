#!/usr/bin/env python3
"""Generate public, explicitly synthetic requests; never execute the diagnostic."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import stat

PINS = {
    "initial.json": ("33aee17d626a3e541c7934c5576c025f8819d442555df6fadc90dde49bd045a7", 62713),
    "profile.json": ("e3440eba0a871a8eedf84a6e2cdccee53cc4389e8ce6b979166621c8e893890d", 1331),
    "genesis.json": ("3373c76de09192ad440625613db97c17b50fbfeddb5adb3190c8f0eb83a22005", 15996),
    "manifest.json": ("ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6", 23260),
}
TOOL_SOURCE_SHA = "d496f9e5ea94579cec22dc4f9cc43288b42d203e5704284a17cf88928784251f"


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded(value):
    return (json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False) + "\n").encode()


def q(n):
    need(type(n) is int and n >= 0, "unsigned integer required")
    return hex(n)


def read_public(path, pin):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        before = os.fstat(fd)
        need(stat.S_ISREG(before.st_mode) and before.st_size == pin[1], "public fixture size/type differs")
        data = b""
        while len(data) < before.st_size:
            part = os.read(fd, min(65536, before.st_size - len(data)))
            need(part, "short public fixture read")
            data += part
        after = os.fstat(fd)
        need((before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns)
             == (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns), "public fixture changed")
        need(sha(data) == pin[0], "official public fixture SHA differs")
        return data
    finally:
        os.close(fd)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path, help="fresh output directory, must not exist")
    parser.add_argument("--runtime-root", default="/public-cases", help="absolute mount point seen by C++; no execution")
    args = parser.parse_args()
    runtime = Path(args.runtime_root)
    need(runtime.is_absolute() and str(runtime) == os.path.normpath(str(runtime)), "canonical lexical runtime root required")
    fixture = args.fixture.resolve(strict=True)
    source = {name: read_public(fixture / name, pin) for name, pin in PINS.items()}
    manifest, initial = json.loads(source["manifest.json"]), json.loads(source["initial.json"])
    need(manifest["campaignId"] == "cb532955c358607d94e4e1074e26be4f4a235dd61b1a00ac2117129973a385ed", "wrong public campaign")
    need(len(initial["blocks"]) == 1 and initial["blocks"][0][0] == initial["anchor"], "public fixture head shape differs")
    for role, name in (("initial", "initial.json"), ("profile", "profile.json"), ("genesis", "genesis.json")):
        need(manifest["files"][role]["sha256"] == PINS[name][0]
             and manifest["files"][role]["bytes"] == PINS[name][1], "public manifest file binding differs")
    head = initial["blocks"][0]
    cp = lambda pair: {"epoch": pair[0], "root": pair[1]}
    expected = {
        # Deliberate TEST DATA, not any existing ChainStore/capture commitment.
        "primary": {"sequence": "0x1", "digest": "0x" + "11" * 32},
        "witness": {"sequence": "0x2", "digest": "0x" + "22" * 32},
        "chainId": manifest["chainId"], "genesisHash": manifest["executionGenesisHash"],
        "genesisValidatorsRoot": manifest["genesisValidatorsRoot"],
        "networkAnchor": initial["anchor"], "storageAnchor": initial["anchor"],
        "headRoot": head[0], "headStateRoot": head[1][3],
        "executionBlockHash": head[5][0]["hash"],
        "clockGenesisTimeMs": initial["clock"][0], "clockPublishedTimeMs": initial["clock"][1],
        "storeCheckpoints": {"justified": cp(initial["justified"]), "finalized": cp(initial["finalized"]),
                             "unrealizedJustified": cp(initial["clock"][2]), "unrealizedFinalized": cp(initial["clock"][3])},
    }
    blobs = {"files/" + name: source[name] for name in ("initial.json", "profile.json", "genesis.json")}
    artifacts = {role: {"path": str(runtime / "files" / name), "sha256": PINS[name][0], "bytes": q(PINS[name][1])}
                 for role, name in (("snapshot", "initial.json"), ("profile", "profile.json"), ("genesis", "genesis.json"))}
    base = {
        "format": "nano-capella-current-utc-projection-request-v1", "artifacts": artifacts,
        "expected": expected,
        "snapshotLimits": {k: q(v) for k, v in {"envelopeBytes": 134217728, "blocks": 256,
            "messages": 2048, "equivocations": 1000, "stateBytes": 4194304, "totalStateBytes": 67108864,
            "replaySlots": 64, "syncTrials": 100000}.items()},
        "projectionLimits": {k: q(v) for k, v in {"maximumHeadGapSlots": 2000000,
            "maximumChunkSlots": 64, "maximumChunks": 100000, "maximumElapsedMs": 60000,
            "maximumRssKiB": 131072, "maximumInputBytes": 1048576, "maximumObservationAgeMs": 65000}.items()},
        "ordinaryClockStepMs": "0x1d4c0",
    }
    cases = []

    def make_case(name, mutate=None, reasons=None, stage=None, request_sha_bad=False, flexible=False):
        request = copy.deepcopy(base)
        if mutate:
            mutate(request)
        # Each case rebinds an explicitly synthetic extraction declaration. A wrong
        # head binding can therefore reach the actual SSZ/header comparison.
        extraction = {"format": "nano-capella-current-projection-extraction-v1",
            "artifacts": {k: {"sha256": v["sha256"], "bytes": v["bytes"]}
                          for k, v in request["artifacts"].items()}, "binding": copy.deepcopy(request["expected"])}
        epath = "extractions/" + name + ".json"
        eb = encoded(extraction)
        blobs[epath] = eb
        request["artifacts"]["extraction"] = {"path": str(runtime / epath), "sha256": sha(eb), "bytes": q(len(eb))}
        rpath = "requests/" + name + ".json"
        rb = encoded(request)
        need(len(rb) <= 65536, "request bound")
        blobs[rpath] = rb
        actual_sha = sha(rb)
        supplied_sha = ("0" if actual_sha[0] != "0" else "1") + actual_sha[1:] if request_sha_bad else actual_sha
        row = {"name": name, "requestRelativePath": rpath, "actualRequestSha256": actual_sha,
               "suppliedRequestSha256": supplied_sha,
               "argv": ["--request", str(runtime / rpath), supplied_sha],
               "expectedResumeAuthorized": False, "tokensAreSynthetic": True,
               "actualToolExecution": False, "expectedResult": "OBSERVE_ACTUAL" if flexible else "REFUSED"}
        if reasons:
            row["expectedReasons"] = reasons
        if stage:
            row["expectedStage"] = stage
        cases.append(row)

    make_case("request-sha-refusal", reasons=["input_sha256_mismatch"], stage="request", request_sha_bad=True)
    def wrong_snapshot(j):
        old = j["artifacts"]["snapshot"]["sha256"]
        j["artifacts"]["snapshot"]["sha256"] = ("0" if old[0] != "0" else "1") + old[1:]
    make_case("snapshot-sha-refusal", wrong_snapshot, ["input_sha256_mismatch"], "public-input-authentication")
    make_case("head-state-binding-refusal", lambda j: j["expected"].update(headStateRoot="0x" + "00" * 32),
              ["current_head_domain_binding"], "current-snapshot-authentication")
    make_case("head-gap-refusal", lambda j: j["projectionLimits"].update(maximumHeadGapSlots="0x1"),
              ["head_gap_or_next_slot_budget"], "current-utc-observation")
    make_case("chunk-count-refusal", lambda j: j["projectionLimits"].update(maximumChunks="0x1", maximumChunkSlots="0x1"),
              ["chunk_count_exceeded"], "empty-slot-projection")
    make_case("steady-deadline-refusal", lambda j: j["projectionLimits"].update(maximumElapsedMs="0x1"),
              ["steady_deadline_expired"])
    make_case("fifo-type-refusal", lambda j: j["artifacts"]["genesis"].update(path=str(runtime / "public-fifo")),
              ["input_file_metadata"], "public-input-authentication")
    make_case("real-utc-observation", flexible=True)

    output = args.output.absolute()
    need(output.parent.is_dir() and output.parent.resolve() == output.parent, "canonical existing output parent required")
    need(not output.exists() and not output.is_symlink(), "fresh output directory required")
    output.mkdir(mode=0o700)
    inventory = {}
    for rel, data in blobs.items():
        p = output / rel
        p.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
        fd = os.open(p, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o400)
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
        inventory[rel] = {"sha256": sha(data), "bytes": len(data)}
    os.mkfifo(output / "public-fifo", 0o400)
    index = {"format": "nano-current-utc-public-synthetic-cases-v1", "fixtureManifestSha256": PINS["manifest.json"][0],
        "sourceFilePins": {n: {"sha256": p[0], "bytes": p[1]} for n, p in PINS.items()},
        "toolSourceSha256": TOOL_SOURCE_SHA, "runtimeRoot": str(runtime), "files": inventory, "cases": cases,
        "fifo": {"path": "public-fifo", "type": "FIFO", "writerStarted": False},
        "tokensAreSynthetic": True, "currentVpsPinsRead": False, "currentSnapshotAuthorityQualified": False,
        "actualToolExecutions": 0, "genesisOrPublishedClockChanged": False, "newSignatures": 0,
        "resumeAuthorized": False, "productionQualified": False}
    data = encoded(index)
    (output / "cases.json").write_bytes(data)
    (output / "cases.json").chmod(0o400)
    print(json.dumps({"output": str(output), "cases": len(cases), "indexSha256": sha(data),
                      "toolExecutions": 0, "tokensAreSynthetic": True, "resumeAuthorized": False}))


if __name__ == "__main__":
    main()
