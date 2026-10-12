from pathlib import Path
import subprocess,json,shutil
out=Path('/home/ubuntu/nanobees-qualification/v37-source-preserved-20261012');assert not out.exists()
docker=['sudo','-n','docker'];source='nano-restored-three-node-vps-20261005-data';image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';old='/tmp/nano-v37-offline-recovery-copy-20261007';origin='/tmp/nano-restored-three-node-vps-muxgxadj';helper='/tmp/nano-publication-cancel-boundary-20261007'
state=json.loads(subprocess.check_output(docker+['inspect','nano-restored-three-node-vps-muxgxadj','--format','{{json .State}}'],text=True));assert not state['Running'] and state['ExitCode']==1 and not state['OOMKilled'];assert not subprocess.check_output(docker+['ps','-q','--filter','volume='+source],text=True).strip();assert shutil.disk_usage('/home/ubuntu').free>3*1024**3
out.mkdir(parents=True,mode=0o700)
code=r'''
from pathlib import Path
import hashlib,json,shutil,os
p=Path('/persist');old=Path('/old');proof=json.loads((old/'recovery-proof.json').read_text());audit=json.loads(Path('/helper/source-runtime-refusal-copy-audit.json').read_text());assert proof['result']=='PASSED_OFFLINE_COPY_ONLY'
assert hashlib.sha256((old/'recovery-run-repair/capture.json').read_bytes()).hexdigest()==proof['captureSha256']==audit['externalCurrentCaptureSha256']
def manifest(root,prefix):
 rows=[]
 for f in root.rglob('*'):
  assert not f.is_symlink(),f
  if f.is_file():rows.append(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+prefix+'/'+str(f.relative_to(root))+'\n')
 return ''.join(sorted(rows))
a=manifest(Path('/source'),'/source');b=manifest(old/'data','/recovery/data')
assert a==(old/'source-before.sha256').read_text();assert hashlib.sha256(a.encode()).hexdigest()==audit['sourceManifestSha256'];assert b==(old/'copy-after.sha256').read_text()
shutil.copytree('/source',p/'source-original');shutil.copytree(old/'data',p/'recovered-data')
assert manifest(p/'source-original','/source')==a and manifest(p/'recovered-data','/recovery/data')==b
for src,dst in [(Path('/source'),p/'source-original'),(old/'data',p/'recovered-data')]:
 for f in src.rglob('*'):
  if f.is_file():assert (f.stat().st_dev,f.stat().st_ino)!=((dst/f.relative_to(src)).stat().st_dev,(dst/f.relative_to(src)).stat().st_ino)
private=p/'private-audit';private.mkdir();origin=p/'config-origin';origin.mkdir()
for n in ['capture.json','capture-command.json','capture-exit.json','recovery.json','idempotent-reopen.json']:shutil.copyfile(old/'recovery-run-repair'/n,private/n)
for n in ['copy-proof.json','recovery-proof.json','source-before.sha256','copy-before.sha256','copy-after.sha256','config.json','request.json']:shutil.copyfile(old/n,private/n)
shutil.copyfile('/helper/source-runtime-refusal-copy-audit.json',private/'source-runtime-refusal-copy-audit.json');(private/'source-volume-current.sha256').write_text(a)
for n in ['genesis.json','profile.json','initial.json','vps.env','vps.json']:shutil.copyfile(Path('/origin')/n,origin/n)
assert manifest(Path('/source'),'/source')==a and manifest(old/'data','/recovery/data')==b
result={'preservedSourceFiles':len(a.splitlines()),'preservedRecoveredFiles':len(b.splitlines()),'sourceManifestSha256':hashlib.sha256(a.encode()).hexdigest(),'recoveredManifestSha256':hashlib.sha256(b.encode()).hexdigest(),'captureSha256':proof['captureSha256'],'fullFileHashesVerified':True,'physicallyIndependentFiles':True,'originalsUnchanged':True,'originalVolumeMountedReadOnly':True,'durablePath':'/home/ubuntu/nanobees-qualification/v37-source-preserved-20261012','runtimeDutiesStarted':False,'networkQualified':False,'productionQualified':False}
(p/'preservation-proof.json').write_text(json.dumps(result,indent=2)+'\n')
for f in p.rglob('*'):os.chown(f,1000,1000)
os.chown(p,1000,1000);os.chmod(p,0o700)
print(json.dumps(result),flush=True)
'''
args=docker+['run','--rm','--network','none','--read-only','--tmpfs','/tmp','-v',source+':/source:ro','-v',old+':/old:ro','-v',origin+':/origin:ro','-v',helper+':/helper:ro','-v',str(out)+':/persist','--entrypoint','python3',image,'-c',code]
p=subprocess.run(args);raise SystemExit(p.returncode)
