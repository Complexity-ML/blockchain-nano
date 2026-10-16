import json,pathlib,subprocess,uuid,time,hashlib,os
E=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012');O=E/'rpc-simulation-context-ci-root-qualification-20261016-05';assert not O.exists();O.mkdir(mode=0o700)
D=['/Applications/Docker.app/Contents/Resources/bin/docker','--config',str(E/'fresh-three-owner-current-20261016-02/docker-client'),'--host','unix:///Users/boris/.docker/run/docker.sock'];image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';name='nano-rpc-ci-python-'+uuid.uuid4().hex
S=E/'rpc-simulation-context-ci-root-qualification-20261016-04/source';N=E/'stopped-backup-node-linux-arm64-20261016/node';F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
def material(p):
 with p.open('rb') as f:
  a=os.fstat(f.fileno());b=f.read();z={k:getattr(a,k) for k in F};assert z=={k:getattr(p.lstat(),k) for k in F}=={k:getattr(os.fstat(f.fileno()),k) for k in F};return dict(z,sha256=hashlib.sha256(b).hexdigest())
old=json.loads((N.parent/'prospective-admission.private.json').read_text())['observations'][-1]['table']['node'];assert material(N)==old
held={str(p):material(p) for p in [N]+[p for p in S.rglob('*') if p.is_file()]};(O/'inputs-before.private.json').write_text(json.dumps(held)+'\n')
def run(args,n,t=60):
 (O/(n+'-argv.json')).write_text(json.dumps(args)+'\n');start=time.monotonic()
 with (O/(n+'.log')).open('xb') as f:z=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,timeout=t)
 (O/(n+'-exit.json')).write_text(json.dumps(dict(exitCode=z.returncode,elapsedMs=round((time.monotonic()-start)*1000)))+'\n');assert z.returncode==0,n
 return (O/(n+'.log')).read_text()
identifier=None;err=None
try:
 identifier=run(D+['run','--detach','--pull=never','--name',name,'--label','nano.rpc.ci.python='+name,'--security-opt','no-new-privileges','--cpus','2','--memory','1g','--pids-limit','128','--tmpfs','/tmp:rw,exec,nosuid,nodev,size=128m','--env','HOME=/tmp','--env','PATH=/tool:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin','--mount',f'type=bind,src={S},dst=/work,readonly','--mount',f'type=bind,src={N},dst=/tool/node,readonly','--workdir','/work','--entrypoint','/bin/sleep',image,'180'],'create').strip();assert len(identifier)==64
 info=json.loads(subprocess.check_output(D+['inspect',identifier]))[0];assert info['Name']=='/'+name and info['Image']==image and info['Config']['Labels']['nano.rpc.ci.python']==name
 run(D+['exec',identifier,'/bin/sh','-ec','apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends python3'],'python-setup',90)
 run(D+['exec',identifier,'/bin/sh','-ec','python3 --version; dpkg-query -W python3 python3-minimal python3.12 python3.12-minimal libpython3.12-stdlib libpython3.12-minimal'],'python-provenance')
 run(D+['network','disconnect','bridge',identifier],'network-disconnect')
 info=json.loads(subprocess.check_output(D+['inspect',identifier]))[0];assert info['NetworkSettings']['Networks']=={}
 run(D+['exec','--user','501:20',identifier,'/tool/node','--test','--test-name-pattern','daily shell','scripts/ci/check-core.test.mjs'],'daily-guard',30)
except BaseException as e:err=e
finally:
 if identifier:
  info=json.loads(subprocess.check_output(D+['inspect',identifier]))[0];assert info['Name']=='/'+name and info['Image']==image and info['Config']['Labels']['nano.rpc.ci.python']==name
  run(D+['rm','--force',identifier],'cleanup')
 changed=[p for p,z in held.items() if material(pathlib.Path(p))!=z]
 (O/'terminal.json').write_text(json.dumps(dict(passed=err is None and not changed,error=str(err) if err else None,changed=changed,imageBuilds=0,candidateCoreTestsExecuted=0,pythonInstalledOnlyInRemovedContainer=True,productionQualified=False))+'\n')
if err:raise err
assert not changed
