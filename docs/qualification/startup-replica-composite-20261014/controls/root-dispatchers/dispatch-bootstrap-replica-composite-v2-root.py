import hashlib,io,json,pathlib,shlex,subprocess,sys,tarfile,ast
E=pathlib.Path(__file__).resolve().parent;P='/home/ubuntu/nanobees-qualification';arch=sys.argv[1];assert arch in ('arm64','amd64');arm=arch=='arm64'
K=E/'bootstrap-maintenance-replica-deadline-composite-controller-v2-20261014';L=E/'bootstrap-archive-owned-copies-cli-proposal-v6-20261014';D=E/'capella-startup-steady-deadline-proposal-v2-20261014'
CONTROL='6ea1caeb48ff1e5fa4ff6b0126dfff35a157203c3e25fdba8c1140baca06d6b5';MANIFEST='4b37e0b2406591f94133d87aaa2bf125dbc6efc267ee2a1df7c1b35b389e95fb';PROPOSAL='78a3e907bde06e43c05d79239297e261078cdcc2b8ed9d5fedae033b11cb22f8';GUARD='cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2';REG='4d6c4445f7d7e5abb462a2de4b5b76b9ce908e4e1caf8ddcd29d9c5bf3f3bbaf'
sha=lambda b:hashlib.sha256(b).hexdigest()
assert sha((K/'manifest.json').read_bytes())==MANIFEST and sha((K/'build-replica-new-deadline-root-only.py').read_bytes())==CONTROL and sha((L/'manifest.json').read_bytes())==PROPOSAL and sha((D/'registration-block.txt').read_bytes())==REG
base=str(E) if arm else P;root=lambda s:str(pathlib.Path(base)/s)
code=P+'/bootstrap-replica-composite-public-code-v2-20261014';out=root('bootstrap-replica-composite-v2-'+arch+'-20261014');dr=root('capella-startup-deadline-coherent-v2-'+arch+'-20261014/run-01');mr=root('bootstrap-maintenance-canonical-state-'+arch+'-20261014/run-01')
args=['python3',str(K/'build-replica-new-deadline-root-only.py') if arm else code+'/control/build-replica-new-deadline-root-only.py','--controller-sha',CONTROL,
 '--guard',root('bootstrap-owner-runtime-incremental-build-proposal-20261014/build-bootstrap-owner-runtime-root-only.py') if arm else root('bootstrap-owner-runtime-public-code-20261014/control/build-bootstrap-owner-runtime-root-only.py'),'--guard-sha',GUARD,
 '--parent-plan',root('bootstrap-owner-runtime-'+arch+'-control-v2-20261014/plan.json'),'--parent-plan-sha','49adc633bad9093810589237756f2c0b9fbf491c6a2ad9d4ce919343b94c2b57' if arm else 'c45b1e1d68c48dc1307989a93609a0abc5576cab75fefc430ef95c387b5bd733',
 '--deadline-terminal',dr+'/receipts/terminal-build.json','--deadline-terminal-sha','f94dc2e207ed69db1170d9772477c6139e9924f0ce0686f8ab2c80f5ca3ae506' if arm else 'ee35976510525abd54182cb187a480e6d8991ccfcc0588c312162553b91c1e29',
 '--deadline-metadata',dr+'/root-build-public-bindings.json','--deadline-metadata-sha','6746b596979597adbd0ebe5e759ac158805c5667a02933b1687233aa2ef29529' if arm else 'b15d61a2587f931718ce418c61609de0718ce00c996b03d481c740692910430c',
 '--maintenance-terminal',mr+'/receipts/terminal-build.json','--maintenance-terminal-sha','c834f222294f717b0b4eba35af70254b3a6113b8678b1e13ef629e7a3f606691' if arm else '0ca57ffd219db7f5276e96ea0825f5edb4cd8ec9444254ff8dccf2eacfa36008',
 '--maintenance-metadata',mr+'/root-build-public-bindings.json','--maintenance-metadata-sha','d3fba3410d4632e617cd0a406a5c026e24372c55da803baaf64d660f9e357018' if arm else '3c8527b2dc303fa58ae9ffed145bab5ad81fb8946fe1d96fdc2048d7bd77498b',
 '--proposal',str(L/'manifest.json') if arm else code+'/proposal/manifest.json','--proposal-sha',PROPOSAL,'--deadline-registration',str(D/'registration-block.txt') if arm else code+'/deadline-registration-block.txt','--deadline-registration-sha',REG,'--output',out+'/run-01']
def save(path,j):
 with path.open('x') as f:json.dump(j,f,indent=2);f.write('\n')
if arm:
 n=pathlib.Path(out);assert not n.exists();n.mkdir(mode=0o700);save(n/'root-dispatch-command.json',{'argv':args,'testsExecuted':0,'privateInputsRead':False});r=subprocess.run(args);save(n/'root-dispatch-exit.json',{'exitCode':r.returncode,'testsExecuted':0});raise SystemExit(r.returncode)
t=E/'bootstrap-replica-composite-public-dispatch-v2-20261014';assert not t.exists();t.mkdir(mode=0o700);files={'control/manifest.json':(K/'manifest.json').read_bytes(),'proposal/manifest.json':(L/'manifest.json').read_bytes(),'deadline-registration-block.txt':(D/'registration-block.txt').read_bytes()}
for row in json.loads(files['control/manifest.json'])['files']:
 q=pathlib.Path(row['path']);assert q.parent==K;b=q.read_bytes();assert sha(b)==row['sha256'] and len(b)==row['bytes'];files['control/'+q.name]=b
j=json.loads(files['proposal/manifest.json'])
for row in j['sourceFiles']:
 q=L/'source'/row['path'];assert str(q)==row['sourcePath'];b=q.read_bytes();assert sha(b)==row['sha256'] and len(b)==row['bytes'];files['proposal/source/'+row['path']]=b
for name in ('registration-block.txt','README.md'):
 q=L/name;row=next(x for x in j['files'] if x['path']==str(q));b=q.read_bytes();assert sha(b)==row['sha256'] and len(b)==row['bytes'];files['proposal/'+name]=b
assert len(files)==14
rows={n:{'sha256':sha(b),'bytes':len(b)} for n,b in files.items()};files['transport.json']=(json.dumps({'files':rows,'privateInputsIncluded':False},indent=2)+'\n').encode();tarpath=t/'public-code.tar'
with tarfile.open(tarpath,'w') as archive:
 for n,b in files.items():
  assert not n.startswith('/') and '..' not in pathlib.PurePosixPath(n).parts
  info=tarfile.TarInfo(n);info.size=len(b);info.mode=0o444;archive.addfile(info,io.BytesIO(b))
tarsha=sha(tarpath.read_bytes());ssh=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=15','ubuntu@145.239.199.166']
remote=r"""import hashlib,json,pathlib,subprocess,tarfile
D=pathlib.Path(DEST);N=pathlib.Path(OUTPUT);tar=D/'public-code.tar';assert hashlib.sha256(tar.read_bytes()).hexdigest()==TARSHA
with tarfile.open(tar) as archive:
 members=archive.getmembers();rows=json.loads(archive.extractfile('transport.json').read())['files'];assert len(members)==len(rows)+1 and set(x.name for x in members)==set(rows)|{'transport.json'} and all(x.isfile() for x in members)
 for x in members:
  name=x.name;assert not name.startswith('/') and '..' not in pathlib.PurePosixPath(name).parts and chr(92) not in name
  b=archive.extractfile(x).read()
  if name!='transport.json':assert hashlib.sha256(b).hexdigest()==rows[name]['sha256'] and len(b)==rows[name]['bytes']
  f=D/name;assert not f.exists();f.parent.mkdir(parents=True,exist_ok=True)
  with f.open('xb') as stream:stream.write(b)
  f.chmod(0o444)
args=ARGV
with (N/'root-dispatch-command.json').open('x') as stream:json.dump({'argv':args,'testsExecuted':0,'privateInputsRead':False},stream,indent=2);stream.write('\n')
r=subprocess.run(args)
with (N/'root-dispatch-exit.json').open('x') as stream:json.dump({'exitCode':r.returncode,'testsExecuted':0},stream,indent=2);stream.write('\n')
raise SystemExit(r.returncode)
""".replace('DEST',repr(code)).replace('OUTPUT',repr(out)).replace('TARSHA',repr(tarsha)).replace('ARGV',repr(args));ast.parse(remote)
prepare='import pathlib; d=pathlib.Path('+repr(code)+'); n=pathlib.Path('+repr(out)+'); assert not d.exists() and not n.exists(); d.mkdir(mode=0o700); n.mkdir(mode=0o700)'
r=subprocess.run(ssh+['python3 -c '+shlex.quote(prepare)],text=True,capture_output=True);assert r.returncode==0,r.stderr
r=subprocess.run(['scp','-q','-o','BatchMode=yes','-o','ConnectTimeout=15',str(tarpath),'ubuntu@145.239.199.166:'+code+'/public-code.tar'],text=True,capture_output=True);assert r.returncode==0,r.stderr
save(t/'root-public-dispatch.json',{'argv':args,'tarSha256':tarsha,'publicFiles':rows,'remotePython':remote,'testsExecuted':0,'privateInputsRead':False});r=subprocess.run(ssh+['python3 -c '+shlex.quote(remote)]);save(t/'root-dispatch-exit.json',{'exitCode':r.returncode,'testsExecuted':0});raise SystemExit(r.returncode)
