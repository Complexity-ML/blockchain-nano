from pathlib import Path
import subprocess,json,hashlib,shutil,time,os
source=Path('/var/lib/docker/volumes/nano-restored-three-node-vps-20261005-data/_data');old=Path('/tmp/nano-restored-three-node-vps-muxbpvx9');audit=Path('/tmp/nano-v33-post-stop-readonly-20261007');root=Path('/tmp/nano-startup-phase-profile-20261007');name='nano-startup-phase-diagnostic-amd64-20261007';image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';binary='/build/startup-stage-diagnostic-amd64-20261007/nano';expectedBinary='4c6c68ef29de669c7704f854e6b463b9e7961eaff2f8449bd350ab8409bfd965'
def run(a):return subprocess.check_output(a,text=True)
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
def hashes(p):return {str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}
assert not run(['docker','ps','-q','--filter','volume=nano-restored-three-node-vps-20261005-data']).strip()
s=json.loads(run(['docker','inspect',old.name,'--format','{{json .State}}']));assert not s['Running'] and s['ExitCode']==137 and not s['OOMKilled'];assert json.loads((audit/'exit.json').read_text())['exitCode']==0
expected={line.split('  ',1)[1].removeprefix('/var/lib/nano/'):line.split('  ',1)[0] for line in (audit/'data-before.sha256').read_text().splitlines()};before=hashes(source);assert before==expected
assert shutil.disk_usage('/tmp').free>2*sum(f.stat().st_size for f in source.rglob('*') if f.is_file())+2**30
root.mkdir(mode=0o700);data=root/'data';shutil.copytree(source,data,copy_function=shutil.copy2);assert hashes(data)==before==hashes(source);assert all((source/f).stat().st_ino!=(data/f).stat().st_ino for f in before);save(root/'private-source-hashes.json',before);os.chmod(root/'private-source-hashes.json',0o600)
cfg=root/'config';cfg.mkdir(mode=0o700)
for f in ['genesis.json','profile.json','initial.json','vps.json','vps.env']:shutil.copy2(old/f,cfg/f);os.chmod(cfg/f,0o600)
assert run(['docker','run','--rm','--network','none','--read-only','-v','nano-clean-rpc-build-20261002:/build:ro','--entrypoint','sha256sum',image,binary]).split()[0]==expectedBinary
save(root/'copy-receipt.json',{'sourceFiles':len(before),'allHashesEqual':True,'independentInodes':True,'fullJournalsAndWitnessesPreserved':True,'sourceStopped':True,'sourceConfigurationUnchanged':True,'hostLocalPrivateConfiguration':True,'imageReused':True,'networkIsolated':True,'productionQualified':False})
started=False;failure=None;ready=False;began=None
try:
 run(['docker','run','-d','--name',name,'--network','none','--ulimit','nofile=65536:65536','--memory','4g','--cpus','2','--read-only','--tmpfs','/tmp','--cap-drop','ALL','--security-opt','no-new-privileges','-v','nano-clean-rpc-build-20261002:/build:ro','-v',str(data)+':/var/lib/nano','-v',str(cfg)+':/config:ro','--env-file',str(cfg/'vps.env'),'-e','NANO_TEST_NETWORK_TRACE=1','--entrypoint',binary,image,'run','/config/genesis.json','/var/lib/nano/chain','/config/vps.json']);started=True;began=time.monotonic()
 while time.monotonic()-began<180:
  s=json.loads(run(['docker','inspect',name,'--format','{{json .State}}']));assert s['Running'] and not s['OOMKilled'],s
  text=subprocess.run(['docker','logs',name],capture_output=True,text=True,check=True);rows=[]
  for line in (text.stdout+text.stderr).splitlines():
   try:x=json.loads(line)
   except json.JSONDecodeError:continue
   if x.get('event')=='capella-startup-stage':rows.append(x)
  if any(x.get('stage')=='runtime-ready' for x in rows):ready=True;break
  time.sleep(1)
except Exception as e:failure=str(e)
finally:
 if started:
  stop=subprocess.run(['docker','stop','-t','20',name],capture_output=True,text=True);save(root/'stop-command.json',{'exitCode':stop.returncode})
  state=json.loads(run(['docker','inspect',name,'--format','{{json .State}}']));save(root/'state.json',state)
  text=subprocess.run(['docker','logs',name],capture_output=True,text=True,check=True);(root/'runtime.log').write_text(text.stdout+text.stderr)
 assert hashes(source)==before
 save(root/'result.json',{'diagnosticCompleted':True,'runtimeReadyWithin180Seconds':ready,'deadlineSeconds':180,'elapsedIncludingStopSeconds':None if began is None else time.monotonic()-began,'failure':failure,'sourceHashesUnchanged':True,'sourceFiles':len(before),'usesIndependentCopy':True,'network':'none','memory':'4g','cpus':2,'newCommonFinalities':0,'networkQualificationClaimed':False,'productionQualified':False})
print(json.dumps(json.loads((root/'result.json').read_text())))
