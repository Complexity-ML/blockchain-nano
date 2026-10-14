import hashlib,io,json,pathlib,shlex,subprocess,sys,tarfile,ast
E=pathlib.Path(__file__).resolve().parent;P='/home/ubuntu/nanobees-qualification';arch=sys.argv[1];assert arch in ('arm64','amd64');arm=arch=='arm64'
K=E/'bootstrap-maintenance-replica-two-ctest-runner-proposal-20261014'
CONTROL='74517e91f15443a9b608ad446f0dc95b10b83cd11f48ed9f2c32fd3327dd6766';MANIFEST='901b658b85e6cac5dbcb18f1631b459f75ff19bad7d00659c319f3805f054af1';GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2';BUILD='6ea1caeb48ff1e5fa4ff6b0126dfff35a157203c3e25fdba8c1140baca06d6b5'
sha=lambda b:hashlib.sha256(b).hexdigest();assert sha((K/'manifest.json').read_bytes())==MANIFEST;assert sha((K/'run-maintenance-replica-two-tests.py').read_bytes())==CONTROL
base=str(E) if arm else P;root=lambda s:str(pathlib.Path(base)/s)
run=root('bootstrap-replica-composite-v2-'+arch+'-20261014/run-01');out=root('bootstrap-replica-two-tests-v1-'+arch+'-20261014');code=P+'/bootstrap-replica-two-tests-public-code-20261014'
terminal='f988e9e18838cdef127619f1c68bd2109272e855e3f88bbb1bfe04665077e2fe' if arm else 'c7d21478473df6c72d86b99269ed7260998df6e3c8b8a9f4f531301c40ae9c36'
metadata='8b81e4facd6df1f4f76a27c1877f835f9c15001be87384af45453d4d169cf759' if arm else 'fb1d5f4accaa00b35d5292d275fb8a4f1855c3c02136a8acdeb1582f67bb8e5c'
args=['python3',str(K/'run-maintenance-replica-two-tests.py') if arm else code+'/run-maintenance-replica-two-tests.py','--controller-sha',CONTROL,
 '--guard',root('bootstrap-owner-runtime-incremental-build-proposal-20261014/build-bootstrap-owner-runtime-root-only.py') if arm else root('bootstrap-owner-runtime-public-code-20261014/control/build-bootstrap-owner-runtime-root-only.py'),'--guard-sha',GUARD,
 '--parent-plan',root('bootstrap-owner-runtime-'+arch+'-control-v2-20261014/plan.json'),'--parent-plan-sha','49adc633bad9093810589237756f2c0b9fbf491c6a2ad9d4ce919343b94c2b57' if arm else 'c45b1e1d68c48dc1307989a93609a0abc5576cab75fefc430ef95c387b5bd733',
 '--build-root',run,'--build-terminal',run+'/receipts/terminal-build.json','--build-terminal-sha',terminal,'--expected-build-controller-sha',BUILD,'--metadata-manifest',run+'/root-build-public-bindings.json','--metadata-manifest-sha',metadata,'--output',out]
def save(p,j):
 with p.open('x') as stream:json.dump(j,stream,indent=2);stream.write('\n')
if arm:
 dispatch=E/'bootstrap-replica-two-tests-arm64-root-dispatch-20261014.json';assert not dispatch.exists();save(dispatch,{'argv':args,'privateInputsRead':False,'requestedTests':2});r=subprocess.run(args);save(E/'bootstrap-replica-two-tests-arm64-root-exit-20261014.json',{'exitCode':r.returncode});raise SystemExit(r.returncode)
t=E/'bootstrap-replica-two-tests-amd64-root-dispatch-20261014';assert not t.exists();t.mkdir(mode=0o700);files={'manifest.json':(K/'manifest.json').read_bytes()}
for row in json.loads(files['manifest.json'])['files']:
 q=pathlib.Path(row['path']);assert q.parent==K;b=q.read_bytes();assert sha(b)==row['sha256'] and len(b)==row['bytes'];files[q.name]=b
assert len(files)==6
rows={n:{'sha256':sha(b),'bytes':len(b)} for n,b in files.items()};files['transport.json']=(json.dumps({'files':rows,'privateInputsIncluded':False},indent=2)+'\n').encode();tarpath=t/'public-code.tar'
with tarfile.open(tarpath,'w') as archive:
 for n,b in files.items():
  info=tarfile.TarInfo(n);info.size=len(b);info.mode=0o444;archive.addfile(info,io.BytesIO(b))
tarsha=sha(tarpath.read_bytes());ssh=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=15','ubuntu@145.239.199.166']
remote=r"""import hashlib,json,pathlib,subprocess,tarfile
D=pathlib.Path(DEST);N=pathlib.Path(OUT);tar=D/'public-code.tar';assert not N.exists();assert hashlib.sha256(tar.read_bytes()).hexdigest()==TARSHA
with tarfile.open(tar) as archive:
 members=archive.getmembers();rows=json.loads(archive.extractfile('transport.json').read())['files'];assert len(members)==len(rows)+1 and set(x.name for x in members)==set(rows)|{'transport.json'} and all(x.isfile() for x in members)
 for member in members:
  name=member.name;assert not name.startswith('/') and '..' not in pathlib.PurePosixPath(name).parts and chr(92) not in name
  b=archive.extractfile(member).read()
  if name!='transport.json':assert hashlib.sha256(b).hexdigest()==rows[name]['sha256'] and len(b)==rows[name]['bytes']
  f=D/name;assert not f.exists();f.parent.mkdir(parents=True,exist_ok=True)
  with f.open('xb') as stream:stream.write(b)
  f.chmod(0o444)
args=ARGV
with (D/'root-dispatch-command.json').open('x') as stream:json.dump({'argv':args,'privateInputsRead':False,'requestedTests':2},stream,indent=2);stream.write('\n')
r=subprocess.run(args)
with (D/'root-dispatch-exit.json').open('x') as stream:json.dump({'exitCode':r.returncode},stream,indent=2);stream.write('\n')
raise SystemExit(r.returncode)
""".replace('DEST',repr(code)).replace('OUT',repr(out)).replace('TARSHA',repr(tarsha)).replace('ARGV',repr(args));ast.parse(remote)
prepare='import pathlib; d=pathlib.Path('+repr(code)+'); n=pathlib.Path('+repr(out)+'); assert not d.exists() and not n.exists(); d.mkdir(mode=0o700)'
r=subprocess.run(ssh+['python3 -c '+shlex.quote(prepare)],capture_output=True,text=True);assert r.returncode==0,r.stderr
r=subprocess.run(['scp','-q','-o','BatchMode=yes','-o','ConnectTimeout=15',str(tarpath),'ubuntu@145.239.199.166:'+code+'/public-code.tar'],capture_output=True,text=True);assert r.returncode==0,r.stderr
save(t/'root-public-dispatch.json',{'argv':args,'tarSha256':tarsha,'publicFiles':rows,'remotePython':remote,'privateInputsRead':False})
r=subprocess.run(ssh+['python3 -c '+shlex.quote(remote)]);save(t/'root-dispatch-exit.json',{'exitCode':r.returncode});raise SystemExit(r.returncode)
