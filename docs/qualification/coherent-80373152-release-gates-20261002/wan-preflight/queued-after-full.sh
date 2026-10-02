#!/usr/bin/env bash
set -euo pipefail
control=/tmp/nano-coherent-80373152-wan-preflight-20261002
full=/tmp/nano-full-coherent-80373152-amd-host-reports-20261002
status=$control/queue-state.json
trap 'code=$?; printf "%s\n" "$code" > "$control/queue-exit.txt"' EXIT
# Observe the precise current driver. A missing terminal report is a refusal,
# never authority to rerun builds or tests.
while ps -p 46814 -o command= | grep -Fx 'bash /tmp/nano-full-coherent-80373152-amd-control-20261002/host-driver.sh' >/dev/null; do sleep 15; done
for attempt in $(seq 1 12); do test -f "$full/driver-exit.txt" && break; sleep 5; done
python3 - "$full" "$status" <<'FULL_GATE'
import json,sys,xml.etree.ElementTree as E
from pathlib import Path
p=Path(sys.argv[1]);out=Path(sys.argv[2])
def save(x):out.write_text(json.dumps(x,indent=2)+'\n')
save({'phase':'checking-full','newWanNodesStarted':False})
try:
 assert (p/'driver-exit.txt').read_text().strip()=='0'
 assert (p/'audit-exits.txt').read_text().strip()=='0 0'
 a=json.loads((p/'result-audit.json').read_text());assert a['complete'] and a['registered']==426 and not a['failed'] and a['skipped']==0
 r=p/'terminal';tests=json.loads((r/'registry.json').read_text())['tests'];cases=list(E.parse(r/'results.xml').getroot().iter('testcase'))
 assert len(tests)==len(cases)==426 and {t['name'] for t in tests}=={c.attrib['name'] for c in cases}
 assert all(c.find('failure') is None and c.find('error') is None and c.find('skipped') is None for c in cases)
 for x,y in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json'),('link-inputs-before.txt','link-inputs-after.txt')]:assert (r/x).read_bytes()==(r/y).read_bytes()
 assert (r/'source-revision.txt').read_text().strip()=='803731526c33111ea128bf6fddf5d7323fb5b220'
except Exception as e:
 save({'phase':'full-refused','reason':str(e) or 'Required full evidence failed','newWanNodesStarted':False});raise
save({'phase':'offline-audit-running','newWanNodesStarted':False})
FULL_GATE
set +e
ssh -o BatchMode=yes ubuntu@145.239.199.166 'bash /tmp/nano-coherent-80373152-amd64-offline-control/run.sh' > "$control/amd-offline-driver.log" 2>&1
code=$?
set -e
printf '%s\n' "$code" > "$control/amd-offline-driver-exit.txt"
test "$code" = 0
# Launch rechecks all eight gates, exact artifact identity and provenance.
# A recorded ready flag from an earlier observation grants no authority.
exec python3 "$control/launch.py"
