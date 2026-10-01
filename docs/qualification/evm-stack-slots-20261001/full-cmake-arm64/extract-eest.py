#!/usr/bin/env python3
"""Extract the complete EEST test block, never truncated JUnit system-out."""
import json, sys
from pathlib import Path
lines = Path(sys.argv[1]).read_text().splitlines()
starts = [i for i, line in enumerate(lines) if "Testing: ethereum_official_regressions" in line]
if len(starts) != 1:
    raise SystemExit("Missing or ambiguous EEST test block")
rows = []
ended = False
for line in lines[starts[0] + 1:]:
    if "Test time =" in line:
        ended = True
        break
    if line.startswith("{"):
        row = json.loads(line)
        if not ("status" in row or "summary" in row):
            raise SystemExit("Unexpected JSON in EEST block")
        rows.append(line)
if not ended or not rows or "summary" not in json.loads(rows[-1]):
    raise SystemExit("Incomplete EEST output")
Path(sys.argv[2]).write_text("\n".join(rows) + "\n")
