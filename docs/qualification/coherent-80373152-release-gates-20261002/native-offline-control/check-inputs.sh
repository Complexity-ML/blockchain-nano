set -euo pipefail
cd /tmp/nano-snapshot-ssz-view-candidate-20261002
inputs(){ python3 - <<'PY'
import hashlib,json,subprocess
from pathlib import Path
build=Path('build-release');files=set(build.rglob('*.a'))|set(build.rglob('*.o'))
programs=['nano','nano-capella-transition-tests','nano-capella-snapshot-hex-tests','nano-capella-audit-ancestry-tests','nano-hex-encoding-tests','nano-tests','nano-capella-rotation-audit','nano-checkpoint-json-tests','nano-capella-snapshot-json-tests']
files.update(build/name for name in programs);files.update([Path('/opt/homebrew/bin/node'),Path('/Users/boris/Dev/blockchain-nano/build-network/nano-p2p')])
pending=list(files);seen=set()
while pending:
 f=pending.pop()
 if f in seen:continue
 seen.add(f)
 if f.suffix in ['.a','.o']:continue
 r=subprocess.run(['otool','-L',str(f)],capture_output=True,text=True)
 if r.returncode!=0:continue
 for line in r.stdout.splitlines()[1:]:
  name=line.strip().split(' (',1)[0]
  if name.startswith('/') and Path(name).is_file() and Path(name) not in files:
   dep=Path(name);files.add(dep);pending.append(dep)
print(json.dumps({str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(files)},sort_keys=True))
PY
}
inputs > /tmp/nano-coherent-80373152-native-offline-reports-20261002/inputs-after-offline.json
cmp /tmp/nano-coherent-80373152-native-test-reports-20261002/inputs-after.json /tmp/nano-coherent-80373152-native-offline-reports-20261002/inputs-after-offline.json
