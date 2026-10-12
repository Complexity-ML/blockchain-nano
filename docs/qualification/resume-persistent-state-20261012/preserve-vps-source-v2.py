from pathlib import Path
import subprocess,json,shutil,hashlib
out=Path('/home/ubuntu/nanobees-qualification/v37-source-preserved-20261012');assert out.exists() and not list(out.iterdir()),'Only empty first-attempt destination is accepted'
docker=['sudo','-n','docker'];source='nano-restored-three-node-vps-20261005-data';image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';old=Path('/tmp/nano-v37-offline-recovery-copy-20261007');origin=Path('/tmp/nano-restored-three-node-vps-muxgxadj');helper=Path('/tmp/nano-publication-cancel-boundary-20261007')
def stopped():
 state=json.loads(subprocess.check_output(docker+['inspect','nano-restored-three-node-vps-muxgxadj','--format','{{json .State}}'],text=True));assert not state['Running'] and state['ExitCode']==1 and not state['OOMKilled'];assert not subprocess.check_output(docker+['ps','-q','--filter','volume='+source],text=True).strip()
stopped();assert shutil.disk_usage('/home/ubuntu').free>3*1024**3
proof=json.loads((old/'recovery-proof.json').read_text());audit=json.loads((helper/'source-runtime-refusal-copy-audit.json').read_text());assert proof['result']=='PASSED_OFFLINE_COPY_ONLY';assert hashlib.sha256((old/'recovery-run-repair/capture.json').read_bytes()).hexdigest()==proof['captureSha256']==audit['externalCurrentCaptureSha256']
base=docker+['run','--rm','--network','none','--read-only','--tmpfs','/tmp','-v',source+':/source:ro','-v',str(old)+':/old:ro','-v',str(out)+':/persist','--entrypoint','sh',image]
script='test -z "$(find /source /old/data -type l -print -quit)" && find /source -type f -exec sha256sum {} + | sort > /persist/source-volume-current.sha256 && mkdir /persist/source-original /persist/recovered-data && cp -a /source/. /persist/source-original/ && cp -a /old/data/. /persist/recovered-data/ && find /source -type f -printf "%D:%i:%P\\n" | sort > /persist/original-inodes.txt && find /persist/source-original -type f -printf "%D:%i:%P\\n" | sort > /persist/copied-inodes.txt && chown -R 1000:1000 /persist'
p=subprocess.run(base+['-c',script]);assert p.returncode==0,p.returncode

def manifest(root,prefix):
 rows=[]
 for f in root.rglob('*'):
  assert not f.is_symlink(),f
  if f.is_file():rows.append(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+prefix+'/'+str(f.relative_to(root))+'\n')
 return ''.join(sorted(rows))
a=(out/'source-volume-current.sha256').read_text();b=manifest(old/'data','/recovery/data');assert a==(old/'source-before.sha256').read_text();assert hashlib.sha256(a.encode()).hexdigest()==audit['sourceManifestSha256'];assert b==(old/'copy-after.sha256').read_text()
assert manifest(out/'source-original','/source')==a and manifest(out/'recovered-data','/recovery/data')==b

def inodes(name):
 return {line.split(':',2)[2]:tuple(line.split(':',2)[:2]) for line in (out/name).read_text().splitlines()}
x=inodes('original-inodes.txt');y=inodes('copied-inodes.txt');assert x.keys()==y.keys() and len(x)==len(a.splitlines());assert all(x[k]!=y[k] for k in x)
for f in (out/'recovered-data').rglob('*'):
 if f.is_file():assert (f.stat().st_dev,f.stat().st_ino)!=((old/'data'/f.relative_to(out/'recovered-data')).stat().st_dev,(old/'data'/f.relative_to(out/'recovered-data')).stat().st_ino)
private=out/'private-audit';private.mkdir();destination=out/'config-origin';destination.mkdir()
for n in ['capture.json','capture-command.json','capture-exit.json','recovery.json','idempotent-reopen.json']:shutil.copyfile(old/'recovery-run-repair'/n,private/n)
for n in ['copy-proof.json','recovery-proof.json','source-before.sha256','copy-before.sha256','copy-after.sha256','config.json','request.json']:shutil.copyfile(old/n,private/n)
shutil.copyfile(helper/'source-runtime-refusal-copy-audit.json',private/'source-runtime-refusal-copy-audit.json');shutil.copyfile(out/'source-volume-current.sha256',private/'source-volume-current.sha256')
for n in ['genesis.json','profile.json','initial.json','vps.env','vps.json']:shutil.copyfile(origin/n,destination/n)
current=subprocess.check_output(base+['-c','find /source -type f -exec sha256sum {} + | sort'],text=True);assert current==a and manifest(old/'data','/recovery/data')==b;stopped()
result={'preservedSourceFiles':len(a.splitlines()),'preservedRecoveredFiles':len(b.splitlines()),'sourceManifestSha256':hashlib.sha256(a.encode()).hexdigest(),'recoveredManifestSha256':hashlib.sha256(b.encode()).hexdigest(),'captureSha256':proof['captureSha256'],'fullFileHashesVerified':True,'physicallyIndependentFiles':True,'originalsUnchanged':True,'originalVolumeMountedReadOnly':True,'durablePath':str(out),'runtimeDutiesStarted':False,'networkQualified':False,'productionQualified':False};(out/'preservation-proof.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
