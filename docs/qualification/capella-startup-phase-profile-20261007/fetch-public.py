from pathlib import Path
import subprocess,json,sys,shlex
version=sys.argv[1];assert version in ['refusal','v2'];root='/tmp/nano-startup-phase-profile'+('' if version=='refusal' else '-v2')+'-20261007';dest=Path(__file__).parent/version;dest.mkdir()
code="from pathlib import Path;import json;root=Path("+repr(root)+");print(json.dumps({n:(root/n).read_text() for n in ['copy-receipt.json','stop-command.json','state.json','runtime.log','result.json']}))"
r=subprocess.check_output(['ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166',shlex.join(['sudo','-n','python3','-c',code])],text=True)
for n,t in json.loads(r).items():(dest/n).write_text(t)
print('Public diagnostic metadata retrieved; no configuration, environment, database or file-hash dictionary exported')
