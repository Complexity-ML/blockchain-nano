from pathlib import Path
import subprocess,json,hashlib
r=Path(__file__).parent;parent=Path('/private/tmp/nano-published-signed-inventory-draft-20261007/native-build');out=r/'repair';original=parent/'programs/libnano_core.a';before=hashlib.sha256(original.read_bytes()).hexdigest()
for name in ['nano','capture']:
 j=json.loads((parent/('link-'+name+'-command.json')).read_text());a=j['args'];a[a.index('-o')+1]=str(out/name);a[a.index(str(original))]=str(out/'libnano_core.a');(out/('link-'+name+'-command.json')).write_text(json.dumps({'args':a,'cwd':j['cwd']})+'\n')
 with (out/('link-'+name+'.log')).open('w') as f:p=subprocess.run(a,cwd=j['cwd'],stdout=f,stderr=subprocess.STDOUT)
 (out/('link-'+name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0;print(name,0,flush=True)
assert hashlib.sha256(original.read_bytes()).hexdigest()==before
(out/'runtime-sha256.json').write_text(json.dumps({name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ['nano','capture','test-draft','libnano_core.a']},indent=2)+'\n')
