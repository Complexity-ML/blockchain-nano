set -euo pipefail
source_dir=/home/ubuntu/nano-capella-serialization-candidate-20261002
control=/home/ubuntu/nano-capella-serialization-amd64-control-20261002
reports=/home/ubuntu/nano-capella-serialization-amd64-reports-20261002
python3 - <<'GATE'
from pathlib import Path
import json,xml.etree.ElementTree as E
p=Path('/home/ubuntu/nano-capella-sha-provider-control-20261002-reports')
assert (p/'container-exit.txt').exists(), 'Parent suite is not terminal'
assert (p/'post-test-manifest-exit.txt').read_text().strip()=='0'
for a,b in [('source-manifest.json','source-manifest-after-tests.json'),('executables-before-tests.json','executables-after-tests.json'),('link-inputs-before.txt','link-inputs-after.txt')]:assert (p/a).read_bytes()==(p/b).read_bytes()
cases=list(E.parse(p/'results.xml').getroot().iter('testcase'))
assert len(cases)==5 and all(c.find('skipped') is None for c in cases)
print('Parent five-case suite terminal, unchanged inputs; results preserved including failures')
GATE
test "$(git -C "$source_dir" rev-parse --short=8 HEAD)" = 3030b799
test -z "$(git -C "$source_dir" status --porcelain)"
mkdir "$reports"
chmod 0777 "$reports"
cp "$control/provenance.json" "$reports/"
find "$control" -type f -exec shasum -a 256 {} \; > "$reports/control-sha256.txt"
cd "$source_dir"
git rev-parse HEAD > "$reports/runtime-source-revision.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest.json"
set +e
sudo docker run -i --rm --name nano-capella-serialization-amd64-test --read-only --cap-drop ALL --security-opt no-new-privileges --cpus 2 --memory 3g --ulimit nofile=65536:65536 --tmpfs /tmp:rw,nosuid,nodev,noexec,size=128m --mount "type=bind,src=$source_dir,dst=/work,readonly" --mount "type=bind,src=$control,dst=/control,readonly" --mount "type=bind,src=$reports,dst=/reports" --mount type=volume,src=nano-linux-sync-build,dst=/build -w /work sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c bash -s < "$control/container-control.sh" > "$reports/container.log" 2>&1
container_exit=$?
set -e
printf '%s\n' "$container_exit" > "$reports/container-exit.txt"
git ls-files -z | python3 -c 'import sys,json,hashlib;print(json.dumps({p.decode():hashlib.sha256(open(p,"rb").read()).hexdigest() for p in sys.stdin.buffer.read().split(b"\0") if p},sort_keys=True))' > "$reports/source-manifest-after-tests.json"
cmp "$reports/source-manifest.json" "$reports/source-manifest-after-tests.json"
exit "$container_exit"
