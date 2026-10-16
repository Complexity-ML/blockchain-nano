"""Root-injected first admission of Native products; no CLI or automatic work."""
import os
import pathlib
import re
import time


def install(m):
    """Install the bounded replacement, preserving all pre-existing authorities."""
    def admit_products(paths, name, executable=False):
        if not isinstance(name, str) or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]{0,79}", name):
            raise ValueError("unsafe product admission name")
        first = None
        observations = []
        pins, protected, rows = {}, {}, {}
        passed = committed = False
        started = time.monotonic()
        stable_count, stable_since = 0, None
        try:
            paths = [pathlib.Path(p) for p in paths]
            m.need(0 < len(paths) <= 16 and len(set(map(str, paths))) == len(paths), "bounded unique products")
            m.need(type(executable) is bool, "executable admission flag")
            m.fence()
            started = time.monotonic()
            deadline = min(started + 30, m.DEADLINE)

            def budget():
                m.need(time.monotonic() < deadline, "prospective product admission deadline")

            def unchanged(key, row):
                for old in protected.get(key, []):
                    m.need(row == old, "retained product/input authority changed: " + key)
                for table in (m.AUTH, m.HELD, m.PRODUCTS):
                    if key in table:
                        m.need(row == table[key], "existing product/input identity changed: " + key)

            for p in paths:
                budget()
                key = str(p)
                protected[key] = [dict(table[key]) for table in (m.AUTH, m.HELD, m.PRODUCTS) if key in table]
                row, body = m.material(p)
                unchanged(key, row)
                m.need(row["st_uid"] == 501 and row["st_nlink"] == 1, "owned single-link Native product before chmod")
                # As in the original helper, a new output may receive its first
                # byte pin here. Existing NEW pins are checked, never replaced.
                if key not in m.NEW:
                    m.output(p)
                pins[key] = dict(m.NEW[key])
                m.need(set(pins[key]) == {"sha256", "bytes"}, "NEW pin shape")
                m.need(type(pins[key]["bytes"]) is int and pins[key]["bytes"] > 0, "NEW byte count")
                m.need(isinstance(pins[key]["sha256"], str) and re.fullmatch(r"[0-9a-f]{64}", pins[key]["sha256"]), "NEW SHA256")
                m.need(m.digest(body) == pins[key], "known product bytes changed")
                if executable and row["st_mode"] & 0o7777 != 0o500:
                    m.need(not protected[key], "chmod of retained product refused")
                    os.chmod(p, 0o500)
            previous = None
            while True:
                budget()
                rows = {}
                event = {"elapsedMs": int((time.monotonic() - started) * 1000), "complete": False, "files": rows}
                observations.append(event)
                for p in paths:
                    budget()
                    key = str(p)
                    row, body = m.material(p)
                    rows[key] = row
                    unchanged(key, row)
                    m.need(m.NEW[key] == pins[key] and m.digest(body) == pins[key], "sample differs from NEW SHA/bytes")
                    m.need(row["st_uid"] == 501 and row["st_nlink"] == 1, "owned single-link Native product")
                    if executable:
                        m.need(row["st_mode"] & 0o7777 == 0o500, "executable product mode")
                now = time.monotonic()
                budget()
                if rows == previous:
                    stable_count += 1
                else:
                    stable_count, stable_since = 1, now
                event.update(complete=True, elapsedMs=int((now - started) * 1000),
                             stableCount=stable_count, stableSpanMs=int((now - stable_since) * 1000))
                if stable_count >= 3 and now - stable_since >= 2:
                    m.fence()
                    budget()
                    for key, row in rows.items():
                        unchanged(key, row)
                    # Commit only after all checks. Once committed, even a later
                    # failure cannot reset this first identity or replace it.
                    for key, row in rows.items():
                        if key not in m.PRODUCTS:
                            m.PRODUCTS[key] = row
                    committed = True
                    m.fence()
                    passed = True
                    break
                previous = rows
                time.sleep(min(1, max(0, deadline - time.monotonic())))
        except BaseException as error:
            first = error
        finally:
            try:
                record = {"format": "nano-native-prospective-product-admission-v1", "passed": passed,
                          "identitiesCommitted": committed, "knownBytePins": pins, "observations": observations,
                          "stableCount": stable_count, "elapsedMs": int((time.monotonic() - started) * 1000),
                          "errorType": type(first).__name__ if first else None,
                          "errorMessage": str(first) if first else None, "oldAuthoritiesRefreshed": False}
                m.save(name + "-product-first-observations.private.json", record)
            except BaseException as error:
                if first is None:
                    first = error
        if first is not None:
            raise first

    m.admit_products = admit_products
    return admit_products
