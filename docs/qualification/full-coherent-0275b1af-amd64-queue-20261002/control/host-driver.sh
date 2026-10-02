set -euo pipefail
control=/tmp/nano-full-coherent-0275b1af-amd-control-20261002
remote_control=/home/ubuntu/nano-full-coherent-0275b1af-amd-control-20261002
remote_reports=/home/ubuntu/nano-full-coherent-0275b1af-amd-reports-20261002
local_reports=/tmp/nano-full-coherent-0275b1af-amd-host-reports-20261002
mkdir "$local_reports"
trap 'driver_exit=$?; printf "%s\n" "$driver_exit" > "$local_reports/driver-exit.txt"' EXIT
ssh -o BatchMode=yes ubuntu@145.239.199.166 "bash $remote_control/prepare.sh" > "$local_reports/prepare-session.log" 2>&1
scp ubuntu@145.239.199.166:"$remote_reports/registry.json" ubuntu@145.239.199.166:"$remote_reports/executables.json" "$local_reports/"
/opt/homebrew/bin/node "$control/docker-inventory.mjs" "$local_reports/registry.json" "$local_reports/executables.json" > "$local_reports/host-inventory.json"
scp "$local_reports/host-inventory.json" ubuntu@145.239.199.166:"$remote_reports/host-inventory.json"
set +e
ssh -o BatchMode=yes ubuntu@145.239.199.166 "bash $remote_control/tests.sh" > "$local_reports/tests-session.log" 2>&1
runtime_exit=$?
printf '%s\n' "$runtime_exit" > "$local_reports/session-exit.txt"
set -e
ssh -o BatchMode=yes ubuntu@145.239.199.166 "cd $remote_reports; find . -maxdepth 1 -type f -print0 | tar --null -T - -czf -" > "$local_reports/terminal-reports.tar.gz"
mkdir "$local_reports/terminal"
tar -xzf "$local_reports/terminal-reports.tar.gz" -C "$local_reports/terminal"
set +e
bash "$control/verify-results.sh" "$local_reports/terminal/registry.json" "$local_reports/terminal/results.xml" > "$local_reports/result-audit.json" 2> "$local_reports/result-audit.error"
result_audit=$?
/opt/homebrew/bin/node "$control/audit-eest-ctest.mjs" "$local_reports/terminal/LastTest.log" > "$local_reports/eest-counts.json" 2> "$local_reports/eest-counts.error"
eest_audit=$?
set -e
python3 - "$local_reports/terminal" <<'AUDIT'
from pathlib import Path
import sys
p=Path(sys.argv[1])
for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json'),('link-inputs-before.txt','link-inputs-after.txt')]:assert (p/a).read_bytes()==(p/b).read_bytes()
assert (p/'post-test-manifest-exit.txt').read_text().strip()=='0'
AUDIT
printf '%s %s\n' "$result_audit" "$eest_audit" > "$local_reports/audit-exits.txt"
test "$result_audit" = 0 && test "$eest_audit" = 0
exit "$runtime_exit"
