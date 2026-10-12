from pathlib import Path
import json,hashlib,stat,os,subprocess,time
E=Path(__file__).resolve().parent;Q=E.parent
SOURCE_SHA='65f7f64b8f57038b83975b24d7452b131f3649cbf4d726e5311ba293ce48aa90'
BINARY_SHA='1f3702707dac618cd8a48c2ec1ceade58089a477e607c13a9492c11232f3f90e'
EXPECTED={
 'isolated-public-stage/exports/native/isolated-domain.json':'ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6',
 'isolated-public-stage/exports/native/profile.json':'e3440eba0a871a8eedf84a6e2cdccee53cc4389e8ce6b979166621c8e893890d',
 'isolated-public-stage/exports/native/genesis.json':'3373c76de09192ad440625613db97c17b50fbfeddb5adb3190c8f0eb83a22005',
 'isolated-stopped-capture-20261012/mac-native-capture.public.json':'08f49a0c6e3862e53ec90d8a89dc1458a7d66937b3d5a50dfb681561e01a5557',
 'isolated-stopped-capture-20261012/captures/mac-native-pins.private.json':'28f73a51e96054d6a53f7166fb3dddaa5a02cc95f3924e53ba89d02bc549c770',
 'isolated-three-node-cb532-20261012/config/mac-native.json':'4be93895ce1954b4944508219550033ab7b4a266515a5ca7cffa8694fe476ff5',
 'isolated-three-node-cb532-20261012/live-baseline/mac-native-external-stop-receipt.json':'f8a552c9e7eebb37a69cd7e28a737ed111a7dc501d7d1ef99cb6dd9f776c2c3a',
 'isolated-stopped-capture-20261012/mac-docker-capture.public.json':'9f2f4385f2147591881f1724f880689c158591f5030b7507babaa5ce1993b402',
 'isolated-stopped-capture-20261012/captures/mac-docker-pins.private.json':'c80e9ea9cdfe5a2226381d3ae8e7def4adee4ee5a4810048c06eafbff4b55d1a',
 'isolated-three-node-cb532-20261012/config/mac-docker.json':'11eb3a795a333a8264a979d728f3e99c442a4dd0f87aeeb560b86040b815ad37',
 'isolated-three-node-cb532-20261012/live-baseline/mac-docker-external-stop-receipt.json':'87e2ee2a25b13b96279a527811119c00ca5d0ae9ba61096ea5c14a77e19145e5'
}
sha=lambda b:hashlib.sha256(b).hexdigest()
assert sha((E/'project-actual-stopped-store.cpp').read_bytes())==SOURCE_SHA
assert sha((E/'profile-path-repair/build/project-actual-stopped-store').read_bytes())==BINARY_SHA
build=json.loads((E/'profile-path-repair/receipts/build-terminal.json').read_text());assert build['passed'] and build['compileExitCode']==0 and build['linkExitCode']==0 and build['reusedParentsUnchanged']
original=json.loads((E/'receipts/input-files-before.json').read_text())
def observe():
 rows=[]
 for relative,expected in EXPECTED.items():
  p=Q/relative;st=p.lstat();assert p.is_file() and not p.is_symlink() and p.resolve()==p and stat.S_IMODE(st.st_mode)&0o22==0 and st.st_uid==501
  actual=sha(p.read_bytes());assert actual==expected,'literal source binding changed: '+relative
  if relative.endswith('pins.private.json'):assert stat.S_IMODE(st.st_mode)==0o600
  rows.append({'path':str(p),'sha256':actual,'bytes':st.st_size,'device':str(st.st_dev),'inode':str(st.st_ino),'uid':st.st_uid,'mode':stat.S_IMODE(st.st_mode),'mtimeNs':st.st_mtime_ns,'ctimeNs':st.st_ctime_ns})
 return rows
before=observe();assert before==original,'input identity changed after read-only inspection'
templateBytes=(E/'request-template.json').read_bytes();assert sha(templateBytes)=='7154a67e148dbef037d7439d625330ffcc46da7693003eedd87b4e3d086a7c9c'
request=json.loads(templateBytes);assert request['actualWallObservedMs'] is None
run=E/('actual-run-'+time.strftime('%Y%m%dT%H%M%SZ',time.gmtime()));run.mkdir(mode=0o700)
asof=time.time_ns()//1000000;request['actualWallObservedMs']=hex(asof);requestBytes=(json.dumps(request,indent=2)+'\n').encode();fd=os.open(run/'request.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
with os.fdopen(fd,'wb') as f:f.write(requestBytes)
args=['docker','run','--rm','--pull=never','--platform','linux/arm64','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--cpus','1','--memory','128m','--ulimit','cpu=60:60','--mount',f'type=bind,source={E}/profile-path-repair/build,target=/program,readonly','--mount',f'type=bind,source={Q}/isolated-public-stage/exports/native,target=/inputs/public-fixture,readonly','--mount',f'type=bind,source={Q}/isolated-stopped-capture-20261012,target=/inputs/capture-work,readonly','--mount',f'type=bind,source={Q}/isolated-three-node-cb532-20261012/config,target=/inputs/config,readonly','--mount',f'type=bind,source={Q}/isolated-three-node-cb532-20261012/live-baseline,target=/inputs/stops,readonly','--mount',f'type=bind,source={run},target=/request,readonly','--entrypoint','/program/project-actual-stopped-store','sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','--project-common-mac-stopped-store','/request/request.json',sha(requestBytes)]
(run/'command.json').write_text(json.dumps({'args':args,'timeoutSeconds':65,'scope':'one pure in-memory projection of shared exact stopped Mac core checkpoint','sourceAndInputsReadOnly':True},indent=2)+'\n')
start=time.monotonic();exitcode=None;diagnostic=None
try:
 with (run/'projection.log').open('wb') as log:p=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,timeout=65)
 exitcode=p.returncode
except subprocess.TimeoutExpired:exitcode=124;diagnostic='collector finite65second timeout'
after=observe();assert after==before
(run/'input-files-after.json').write_text(json.dumps(after,indent=2)+'\n')
receipt={'format':'nano-actual-common-stopped-mac-projection-run-v1','actualWallObservedMs':str(asof),'actualWallObservedUTC':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime(asof/1000)),'exitCode':exitcode,'collectorDiagnostic':diagnostic,'elapsedSeconds':time.monotonic()-start,'sourceSha256':SOURCE_SHA,'binarySha256':BINARY_SHA,'requestSha256':sha(requestBytes),'inputHashesLiteralPinned':EXPECTED,'allInputsBytesAndInodesUnchanged':True,'projectionLogSha256':sha((run/'projection.log').read_bytes()),'scope':'actual shared stopped canonical Store only; no new DB/journal audit or live startup/duty qualification','sourceCapsulesRemainOnMac':True,'runtimeStarted':False,'networkUsed':False,'vpsAccessed':False,'productionQualified':False}
(run/'terminal-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'runDirectory':str(run),'exitCode':exitcode,'actualWallObservedUTC':receipt['actualWallObservedUTC'],'elapsedSeconds':receipt['elapsedSeconds'],'terminalReceiptSha256':sha((run/'terminal-receipt.json').read_bytes())}));raise SystemExit(exitcode)
