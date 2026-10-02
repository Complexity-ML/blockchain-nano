"""Observe qualification evidence only; never imports or executes the launcher."""
import json,subprocess,sys
from pathlib import Path
names=['ctest-exit.txt','post-test-manifest-exit.txt','audit-exit.txt']
def stage(root,files):return {n:(root/n).read_text().strip() if (root/n).exists() else None for n in files}
local={'armFull':stage(Path('/tmp/nano-coherent-network-arm64-reports-20261002'),names),'armNetwork':stage(Path('/tmp/nano-network-supplement-arm64-reports-20261002'),['ctest-exit.txt','container-exit.txt','audit-exit.txt'])}
script="from pathlib import Path;import json;p=Path('/home/ubuntu');groups={'amdFull':('nano-coherent-network-amd64-reports-20261002',['ctest-exit.txt','post-test-manifest-exit.txt','audit-exit.txt']),'amdNetwork':('nano-network-supplement-amd64-reports-20261002',['ctest-exit.txt','container-exit.txt','audit-exit.txt']),'amdSigned':('nano-coherent-amd64-signed-audit-reports-20261002',['qualification-exit.txt','container-exit.txt'])};print(json.dumps({k:{n:(p/d/n).read_text().strip() if (p/d/n).exists() else None for n in names} for k,(d,names) in groups.items()}))"
import shlex
r=subprocess.run(['/tmp/nano-coherent-wan-transport-control-20261002/bin/ssh','-o','ConnectTimeout=10','ubuntu@145.239.199.166',shlex.join(['python3','-c',script])],capture_output=True,text=True,timeout=30)
if r.returncode:print(r.stderr,file=sys.stderr);raise SystemExit(r.returncode)
groups={**local,**json.loads(r.stdout)}
print(json.dumps({'phase':'read-only-evidence-inspection','terminalFiles':groups,'allTerminalExitFilesZero':all(v=='0' for rows in groups.values() for v in rows.values()),'wanLauncherExecuted':False,'nodesStarted':False},indent=2))
