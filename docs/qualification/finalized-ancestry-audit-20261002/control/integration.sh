set -euo pipefail
cp /reports/audit-driver /scratch/audit-driver;chmod +x /scratch/audit-driver
cd /fixture;find . -type f -print0 | sort -z | xargs -0 sha256sum > /reports/integration-fixture-before.sha256
sha256sum /scratch/audit-driver > /reports/integration-program-before.sha256
for name in positive-current reject-head-as-finalized reject-future-epoch reject-malformed-root;do
 set +e
 timeout 180 /scratch/audit-driver /work/docs/qualification/capella-offline-audit-20260930/genesis.json /fixture/chain /control/$name.json > /reports/$name.json 2> /reports/$name.stderr
 result=$?;set -e;printf '%s\n' "$result" > /reports/$name.exit.txt
 if test "$name" = positive-current;then test "$result" = 0;else test "$result" = 1;fi
done
grep -q 'observed checkpoint differs from finalized epoch boundary' /reports/reject-head-as-finalized.stderr
grep -q 'observed finalized epoch outside replay bounds' /reports/reject-future-epoch.stderr
grep -q 'required finalized root must be canonical 32-byte hex' /reports/reject-malformed-root.stderr
find . -type f -print0 | sort -z | xargs -0 sha256sum > /reports/integration-fixture-after.sha256
sha256sum /scratch/audit-driver > /reports/integration-program-after.sha256
cmp /reports/integration-fixture-before.sha256 /reports/integration-fixture-after.sha256
cmp /reports/integration-program-before.sha256 /reports/integration-program-after.sha256
printf '0\n' > /reports/integration-exit.txt
