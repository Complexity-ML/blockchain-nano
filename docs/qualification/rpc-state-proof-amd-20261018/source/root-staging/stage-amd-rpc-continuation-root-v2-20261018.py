"""Stage a new retained AMD continuation; private authorities stay on the VPS."""
from pathlib import Path
import base64,hashlib,json,os,shlex,stat,subprocess,time
E=Path(__file__).parent.resolve()
S=E/'amd-rpc-continuation-source-v2-20261018'
O=E/'amd-rpc-continuation-staging-root-20261018-01'
HEAD='4388f77b9e9c1b3b94ba5da6f3e486eaa13a3f5c'
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def read(p,want=None):
 assert p.is_absolute() and p.resolve(strict=True)==p
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);assert stat.S_ISREG(a.st_mode) and a.st_nlink==1 and 0<a.st_size<=16777216
  row={k:getattr(a,k)for k in F};chunks=[];n=0
  while b:=os.read(fd,1048576):n+=len(b);assert n<=a.st_size;chunks.append(b)
  raw=b''.join(chunks);assert n==a.st_size and row=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F}
  row['sha256']=hashlib.sha256(raw).hexdigest()
  assert want is None or row==want
  return row,raw
 finally:os.close(fd)
manifest_row,manifest_raw=read(S/'manifest.json')
# Final public source manifest; no execution until peer GO.
EXPECTED_MANIFEST=('b211a8e83a80a7420fe0459d6fedf0bf4744d0c181488956064b3d8c355a7ef8',4160)
assert (manifest_row['sha256'],len(manifest_raw))==EXPECTED_MANIFEST
mf=json.loads(manifest_raw);held={str(S/'manifest.json'):manifest_row}
for item in mf['payloads']:
 p=S/item['path'];row,raw=read(p);assert (row['sha256'],len(raw))==(item['sha256'],item['bytes']);held[str(p)]=row
for item in mf['parents']:
 p=Path(item['path']);row,raw=read(p);assert (row['sha256'],len(raw))==(item['sha256'],item['bytes']);held[str(p)]=row
specs={
 'host.py':(S/'host.py','582389ce8855286667016e012a1f1df32859cbe1686962640e626356ddc3f54c',39355),
 'worker.py':(E/'rpc-state-proof-runtime-linux-worker-source-v2-20261017/worker.py','e9bfc40c92f8659af4eda8a4ebcd306699531b058b56acb5d2a426e34ebb7301',37768),
 'terminal-audit-public.json':(E/'rpc-candidate-amd-terminal-failure-root-20261018-01/root-failure-audit-public.json','e1b1f0d01107b64c14559afda379ee538eb6b2e27b174020b0bdc72f80a52ea7',2054)}
payload={}
for name,(p,sha,size)in specs.items():
 row,raw=read(p,held.get(str(p)));assert (row['sha256'],len(raw))==(sha,size);held[str(p)]=row
 payload[name]={'sha256':sha,'bytes':size,'base64':base64.b64encode(raw).decode()}
remote=r"""from pathlib import Path
import base64,hashlib,json,os,subprocess,sys,time,types
os.umask(0o077)
R=Path('/home/ubuntu/blockchain-nano-git');HEAD='4388f77b9e9c1b3b94ba5da6f3e486eaa13a3f5c'
OLD=Path('/home/ubuntu/rpc-candidate-amd-root-20261018-01')
OLD_CONTROL=Path('/home/ubuntu/rpc-candidate-amd-control-root-20261018-01/control.private.json')
TOOLS=Path('/home/ubuntu/rpc-candidate-amd-continuation-tools-20261018-01')
P=Path('/home/ubuntu/rpc-candidate-amd-continuation-control-20261018-01')
OUT=Path('/home/ubuntu/rpc-candidate-amd-continuation-root-20261018-01')
assert not TOOLS.exists() and not P.exists() and not OUT.exists()
g=lambda *a:subprocess.check_output(['/usr/bin/git','-C',str(R),*a],timeout=30)
assert g('rev-parse','HEAD').decode().strip()==HEAD and g('status','--porcelain')==b''
A=R/'docs/qualification/current-runtime-assemblies-20261016/source/amd-host/qualify-current-amd-helpers-root.py'
raw=A.read_bytes();assert (hashlib.sha256(raw).hexdigest(),len(raw))==('f28ae7a55fadc9fc9bfe04a70c18f991e7440d4c4ab1c4d860fd7aca872fc350',52822)
m=types.ModuleType('root_retained_staging');m.__file__=str(A);exec(compile(raw,str(A),'exec'),m.__dict__)
control_row,control_raw=m.material(OLD_CONTROL)
assert m.digest(control_raw)=={'sha256':'ac2eeedf8406ae807043bb91ec56d135f8b9829b89e54665cd2d3c0e84e2d3d1','bytes':1178681}
old=json.loads(control_raw)
admission=json.loads(m.material(OLD/'control-first-admission.private.json')[1])
assert admission['knownPin']==m.digest(control_raw) and admission['oldAuthoritiesRefreshed'] is False
samples=admission['observations'];assert len(samples)>=3 and samples[-1]['row']==samples[-2]['row']==samples[-3]['row']
assert control_row==samples[-1]['row']
m.hold(OLD_CONTROL,samples[-1]['row'])
for path,row in old['originalFiles'].items():m.hold(path,row)
def admit(path,want):
 key=str(path)
 if key in m.HELD:
  raw=m.hold(path,m.HELD[key]);assert m.digest(raw)==want;return raw
 readings=[]
 for i in range(3):
  row,raw=m.material(path);assert m.digest(raw)==want
  readings.append(row)
  if i<2:time.sleep(1.05)
 assert readings[0]==readings[1]==readings[2]
 m.hold(path,readings[-1]);return raw
auth_path=OLD/'outputs/final-observations.private.json'
authority=json.loads(admit(auth_path,{'sha256':'866e3f3d0042ec8cf90b487498bfa70601e9f9af826379d6dfe5eb03239e43c9','bytes':1512552}))
products=authority['products'];assert len(products)==265
for path,value in products.items():
 assert Path(path).parent==OLD/'outputs'
 m.hold(path,value['hostFD9'])
assert str(OLD/'outputs/unit-candidate-proof.o')not in products and str(OLD/'outputs/unit-candidate-proof.o.d')not in products
report_pin={'sha256':'6a8966c4d1edbbe1f08c2e11c48d13a1a2e40ed592b5e7fbdb4fc586b006a879','bytes':235082}
proof_pin={'sha256':'acabca41c17b518fa2a82dcf8ec7751dab9358344769dd812b53c7b5a43e17cb','bytes':8779}
report_raw=m.material(OLD/'outputs/result-public.json')[1];assert m.digest(report_raw)==report_pin
proof_raw=m.material(OLD/'outputs/core-member-proof.json')[1];assert m.digest(proof_raw)==proof_pin
report=json.loads(report_raw);proof=json.loads(proof_raw)
assert report['passed'] is False and report['firstFailureType']=='TimeoutExpired' and not report['testResults']
assert proof==report['coreProof'] and proof['passed'] is True
j=json.load(sys.stdin)
expected={'host.py':('582389ce8855286667016e012a1f1df32859cbe1686962640e626356ddc3f54c',39355),'worker.py':('e9bfc40c92f8659af4eda8a4ebcd306699531b058b56acb5d2a426e34ebb7301',37768),'terminal-audit-public.json':('e1b1f0d01107b64c14559afda379ee538eb6b2e27b174020b0bdc72f80a52ea7',2054)}
assert set(j)==set(expected)
TOOLS.mkdir(mode=0o700);P.mkdir(mode=0o700)
for name,(sha,size)in expected.items():
 v=j[name];assert(v['sha256'],v['bytes'])==(sha,size)
 b=base64.b64decode(v['base64'],validate=True);assert(hashlib.sha256(b).hexdigest(),len(b))==(sha,size)
 if name.endswith('.py'):compile(b,str(TOOLS/name),'exec')
 m.write_new(TOOLS/name,b,0o400)
for name,raw in [('worker-result-public.json',report_raw),('core-proof-public.json',proof_raw)]:m.write_new(TOOLS/name,raw,0o400)
TOOLS.chmod(0o500)
for name,(sha,size)in expected.items():admit(TOOLS/name,{'sha256':sha,'bytes':size})
admit(TOOLS/'worker-result-public.json',report_pin);admit(TOOLS/'core-proof-public.json',proof_pin)
core=OLD/'outputs/libnano_core.a';cr=products[str(core)]['hostFD9']
assert(cr['sha256'],cr['st_size'])==('3ef6c2cdd75c793b8cec88f3268ab0309e2ebc30aba3be86c1fb9125b13dc652',40988476)
runtime={name:u for name,u in report['unitProducts'].items()if u['scope']=='runtime'}
assert len(runtime)==8
retained=[]
for name,u in sorted(runtime.items()):
 exe=OLD/'outputs'/Path(u['executable']).name;row=products[str(exe)]['hostFD9']
 retained.append({'name':name,'source':u['source'],'file':str(exe),'guest':'/old/executables/'+name,'pin':{'sha256':row['sha256'],'bytes':row['st_size']}})
control={key:old[key]for key in ('sourceRoots','libraries','generatedProviders','runtimeInputs')}
control.update(format='nano-rpc-candidate-amd-retained-continuation-control-v1',ready=True,actualHead=HEAD,
 outputRoot=str(OUT),toolDir=str(TOOLS),retainedCore={'file':str(core),'pin':{'sha256':cr['sha256'],'bytes':cr['st_size']},'producerPin':m.digest(m.material(auth_path)[1])},
 retainedProducer={'control':{'file':str(OLD_CONTROL),'pin':m.digest(control_raw)},'workerResult':{'file':str(TOOLS/'worker-result-public.json'),'pin':report_pin},'coreProof':{'file':str(TOOLS/'core-proof-public.json'),'pin':proof_pin},'terminalAudit':{'file':str(TOOLS/'terminal-audit-public.json'),'pin':{'sha256':expected['terminal-audit-public.json'][0],'bytes':expected['terminal-audit-public.json'][1]}}},
 units=[u for u in old['units']if u['scope']=='candidate'],cases=[t for t in old['cases']if t['unit']not in runtime],
 timeoutSeconds=1380,retainedExecutables=retained,retainedCases=[t for t in old['cases']if t['unit']in runtime])
m.fence();control['originalFiles']=dict(m.HELD)
assert len(control)==17 and len(control['units'])==len(control['cases'])==4 and len(control['retainedCases'])==9
raw=(json.dumps(control,sort_keys=True,indent=2)+'\n').encode()
m.write_new(P/'control.private.json',raw,0o400)
result={'format':'nano-amd-retained-continuation-staging-v1','passed':True,'actualHead':HEAD,'sourceRevision':'c6cdf780c68eab8b06306d76257bda9028928ea3','controlPin':m.digest(raw),'controlFields':17,'originalProductRowsRetained':265,'authorityBytePinPreviouslyPublished':True,'authority866HistoricalFD9Verified':False,'authority866NewInputAdmissionOnly':True,'publicReportsCopiedOnsite':2,'originalHostInputRows':len(m.HELD),'stagedPublicFiles':{k:{'sha256':v[0],'bytes':v[1]}for k,v in expected.items()},'freshCandidateUnits':4,'firstRuntimeCliInvocationsRequested':9,'repeatedCoreCompilation':0,'repeatedRuntimeAssemblies':0,'privateFilesTransferred':0,'cppExecuted':False,'newImagesBuilt':0,'previousFailedEnvelopeReclassified':False,'productionQualified':False}
m.write_new(P/'staging-result-public.json',(json.dumps(result,indent=2)+'\n').encode(),0o400)
assert g('rev-parse','HEAD').decode().strip()==HEAD and g('status','--porcelain')==b''
m.fence()
print(json.dumps(result),flush=True)
"""
compile(remote,'<root-retained-continuation-staging>','exec')
assert not O.exists();os.umask(0o077);O.mkdir(mode=0o700)
q=subprocess.run(['/usr/bin/ssh','-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=10','ubuntu@145.239.199.166','/usr/bin/python3 -c '+shlex.quote(remote)],input=json.dumps(payload).encode(),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=240)
(O/'stage.stdout.json').write_bytes(q.stdout);(O/'stage.stderr.log').write_bytes(q.stderr);(O/'process-exit.json').write_text(json.dumps({'exitCode':q.returncode})+'\n')
assert q.returncode==0 and not q.stderr,(q.returncode,q.stderr.decode())
result=json.loads(q.stdout)
for path,row in held.items():read(Path(path),row)
(O/'result-public.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result),flush=True)
