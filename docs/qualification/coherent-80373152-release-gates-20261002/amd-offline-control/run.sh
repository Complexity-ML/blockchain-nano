#!/usr/bin/env bash
set -euo pipefail
source_dir=/home/ubuntu/nano-snapshot-ssz-view-candidate-20261002
parent=/home/ubuntu/nano-coherent-80373152-amd64-test-reports-20261002
reports=/home/ubuntu/nano-coherent-80373152-amd64-offline-reports-20261002
control=/tmp/nano-coherent-80373152-amd64-offline-control
fixture=/home/ubuntu/nano-stopped-arm-chain-fixture-20261002
# Wait for the existing process; never start a replacement test campaign.
while sudo docker inspect nano-coherent-80373152-amd64-test >/dev/null 2>&1; do sleep 15; done
for attempt in $(seq 1 8); do test -f "$parent/result-audit.json" && break; sleep 5; done
test -f "$parent/result-audit.json"
python3 - "$parent/result-audit.json" <<'GATE'
import json,sys
a=json.load(open(sys.argv[1]));assert a["cases"]==16 and a["skipped"]==0 and a["timeoutsPreserved"]
GATE
test "$(cat "$parent/input-compare-exit.txt")" = 0
cmp "$parent/source-manifest-before.json" "$parent/source-manifest-after.json"
if sudo docker ps --format '{{.Names}}' | grep -E '^nano-full-coherent-80373152-amd64-(prepare|test)$'; then echo "Full regression is active; audit not launched" >&2; exit 2; fi
if test "$(cat "$parent/ctest-exit.txt")" = 0 && ! test -f /home/ubuntu/nano-full-coherent-80373152-amd-reports-20261002/source-manifest-after-tests.json; then
 echo "Successful selection requires completed full regression before this audit; not launched" >&2
 exit 2
fi
cd "$source_dir"
test "$(git rev-parse HEAD)" = 803731526c33111ea128bf6fddf5d7323fb5b220
test -z "$(git status --porcelain)"
mkdir "$reports"; chmod 0777 "$reports"
cp "$parent/result-audit.json" "$reports/parent-result-audit.json"
manifest(){ python3 - "$1" <<'PY'
import hashlib,json,sys
from pathlib import Path
p=Path(sys.argv[1]);print(json.dumps({str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()},sort_keys=True))
PY
}
manifest "$fixture/chain" > "$reports/fixture-before.json"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-before.json"
cp "$parent/programs-after.sha256" "$reports/parent-programs.sha256"
set +e
sudo docker run --rm -i --name nano-coherent-80373152-amd64-offline --network none --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$fixture,dst=/fixture,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build,readonly -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s > "$reports/container.log" 2>&1 <<'RUN'
set -euo pipefail
binary=/build/coherent-80373152-amd64-20261002/nano
sha256sum "$binary" > /reports/binary-before.sha256
awk -v binary="$binary" '$2==binary {print}' /reports/parent-programs.sha256 > /reports/parent-binary.sha256
cmp /reports/binary-before.sha256 /reports/parent-binary.sha256
for name in positive-current reject-head-as-finalized reject-future-epoch reject-malformed-root; do
 start=$(date +%s)
 set +e
 timeout 180 "$binary" audit-capella /work/docs/qualification/capella-offline-audit-20260930/genesis.json /fixture/chain "/control/$name.json" > "/reports/$name.json" 2> "/reports/$name.stderr"
 result=$?
 set -e
 printf '%s\n' "$result" > "/reports/$name.exit"
 printf '%s\n' "$(( $(date +%s) - start ))" > "/reports/$name.seconds"
done
sha256sum "$binary" > /reports/binary-after.sha256
cmp /reports/binary-before.sha256 /reports/binary-after.sha256
RUN
container_exit=$?
set -e
printf '%s\n' "$container_exit" > "$reports/container-exit.txt"
manifest "$fixture/chain" > "$reports/fixture-after.json"
cmp "$reports/fixture-before.json" "$reports/fixture-after.json"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-after.json"
cmp "$reports/source-before.json" "$reports/source-after.json"
test "$container_exit" = 0
python3 - "$reports" "$control" <<'PY'
import json,sys
from pathlib import Path
p,c=map(Path,sys.argv[1:]);request=json.loads((c/'positive-current.json').read_text())
assert (p/'positive-current.exit').read_text().strip()=='0'
a=json.loads((p/'positive-current.json').read_text())
assert a['canonicalHistoryVerified'] is True and a['externalFinalizedAncestorVerified'] is True
assert a['requiredFinalized']==request['requiredFinalized'] and a['finalized']==request['requiredFinalized']
assert a['checkpointSequence']==int(request['expectedCheckpoint']['sequence'],16) and a['checkpointDigest']==request['expectedCheckpoint']['digest']
assert a['blocksIncludingTrustedAnchor']==99 and a['transactions']==97
assert a['head']=='0x4585e50ec1194a31fbf0c5144e93ae87f5ed01ac7a5175fe566c4f8f1528f181'
assert a['historicalForkChoiceVerified'] is False and a['historicalStandaloneVotesReplayed'] is False
errors={'reject-head-as-finalized':'observed checkpoint differs from finalized epoch boundary','reject-future-epoch':'observed finalized epoch outside replay bounds','reject-malformed-root':'required finalized root must be canonical 32-byte hex'}
for name,message in errors.items():
 assert (p/f'{name}.exit').read_text().strip()=='1',(name,(p/f'{name}.stderr').read_text())
 assert message in (p/f'{name}.stderr').read_text()
(p/'result-audit.json').write_text(json.dumps({'cases':4,'passed':4,'fixtureUnchanged':True,'binaryMatchesSelectedCampaign':True,'sourceUnchanged':True,'actualSignedCanonicalReplay':True,'newWanRun':False,'dutyResumptionVerified':False},indent=2)+'\n')
PY
printf '0\n' > "$reports/qualification-exit.txt"
