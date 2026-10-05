from pathlib import Path
import time,subprocess,json,datetime,hashlib
root=Path('/private/tmp/nano-restored-three-node-v7-20261006/campaign')
out=Path('/private/tmp/nano-v7-terminal-audits-20261006');out.mkdir(exist_ok=True)
deadline=time.monotonic()+1200
while not (root/'result.json').exists():
 if time.monotonic()>=deadline: raise SystemExit('Observation timeout waiting for original controller; no audit and no restart')
 time.sleep(1)
rows=[]
for name,script in [('mac','/private/tmp/nano-v7-post-stop-native-diagnostic-20261006.py'),('vps','/private/tmp/nano-post-v7-vps-audit-20261006.py')]:
 with (out/(name+'.stdout')).open('w') as stdout,(out/(name+'.stderr')).open('w') as stderr:
  run=subprocess.run(['python3',script],stdout=stdout,stderr=stderr)
 rows.append({'audit':name,'exitCode':run.returncode,'scriptSha256':hashlib.sha256(Path(script).read_bytes()).hexdigest()})
 (out/'status.json').write_text(json.dumps({'time':datetime.datetime.now(datetime.timezone.utc).isoformat(),'rows':rows,'scope':'Original v7 stop receipts and own signer pins; read-only storage audit only','productionQualified':False},indent=2)+'\n')
 print(json.dumps(rows[-1]),flush=True)
 if run.returncode: raise SystemExit(run.returncode)
print('Both post-stop storage audits completed; inspect captures and original campaign result separately.',flush=True)
