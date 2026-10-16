import pathlib,json,hashlib,os,subprocess,uuid,time
E=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012');O=E/'rpc-simulation-context-ci-root-qualification-20261016-07';assert not O.exists();O.mkdir(mode=0o700)
S=E/'rpc-simulation-context-ci-root-qualification-20261016-04/source';N=E/'stopped-backup-node-linux-arm64-20261016/node';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
held=json.loads((E/'rpc-simulation-context-ci-root-qualification-20261016-05/inputs-before.private.json').read_text());assert json.loads((E/'rpc-simulation-context-ci-root-qualification-20261016-05/terminal.json').read_text())['passed']
def material(p):
 with p.open('rb') as f:
  a=os.fstat(f.fileno());b=f.read();z={k:getattr(a,k) for k in F};assert z=={k:getattr(p.lstat(),k) for k in F}=={k:getattr(os.fstat(f.fileno()),k) for k in F};return dict(z,sha256=hashlib.sha256(b).hexdigest())
assert all(material(pathlib.Path(p))==z for p,z in held.items());(O/'retained-inputs-before.private.json').write_text(json.dumps(held)+'\n')
D=['/Applications/Docker.app/Contents/Resources/bin/docker','--config',str(E/'fresh-three-owner-current-20261016-02/docker-client'),'--host','unix:///Users/boris/.docker/run/docker.sock'];image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';name='nano-rpc-ci-final-'+uuid.uuid4().hex
args=D+['run','--rm','--pull=never','--name',name,'--label','nano.rpc.ci.final='+name,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--cpus','2','--memory','1g','--pids-limit','128','--tmpfs','/tmp:rw,exec,nosuid,nodev,size=64m','--env','HOME=/tmp','--env','PATH=/tool:/usr/bin:/bin','--mount',f'type=bind,src={S},dst=/work,readonly','--mount',f'type=bind,src={N},dst=/tool/node,readonly','--workdir','/work','--entrypoint','/tool/node',image,'--test','--test-name-pattern','^(exact fifteen|missing, duplicated|all CTest|green overrides|explicit positive|new capture|duty availability|RPC simulation context|operations job)','scripts/ci/check-core.test.mjs']
(O/'argv.private.json').write_text(json.dumps(args)+'\n');first=None;t=time.monotonic()
try:
 with (O/'stdout.log').open('xb') as f:z=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,timeout=60)
 assert z.returncode==0
except BaseException as e:first=e
finally:
 z=subprocess.run(D+['inspect',name],capture_output=True,timeout=30)
 if z.returncode==0:
  info=json.loads(z.stdout)[0];assert info['Name']=='/'+name and info['Image']==image and info['Config']['Labels']['nano.rpc.ci.final']==name;subprocess.run(D+['stop','--time','5',info['Id']],check=True,capture_output=True,timeout=30)
 else:assert z.returncode==1 and b'no such object' in z.stderr.lower()
 changed=[p for p,z in held.items() if material(pathlib.Path(p))!=z]
 (O/'terminal.json').write_text(json.dumps(dict(passed=first is None and not changed,error=str(first) if first else None,changed=changed,retainedPriorAuthority=True,elapsedMs=round((time.monotonic()-t)*1000),imageBuilds=0,productionQualified=False))+'\n')
if first:raise first
assert not changed
