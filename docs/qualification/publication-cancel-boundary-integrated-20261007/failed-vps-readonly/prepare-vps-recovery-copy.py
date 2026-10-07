from pathlib import Path
import subprocess,json,hashlib,shutil
out=Path('/tmp/nano-v37-offline-recovery-copy-20261007');out.mkdir(mode=0o700)
source='nano-restored-three-node-vps-20261005-data';image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';docker=['sudo','-n','docker'];audit=Path('/tmp/nano-v37-runtime-refusal-readonly-20261007')
def run(args):return subprocess.check_output(args,text=True)
def stopped():
 s=json.loads(run(docker+['inspect','nano-restored-three-node-vps-muxgxadj','--format','{{json .State}}']));assert not s['Running'] and s['ExitCode']==1 and not s['OOMKilled'];assert not run(docker+['ps','-q','--filter','volume='+source]).strip()
stopped();assert not (out/'data').exists()
base=docker+['run','--rm','--network','none','--read-only','--tmpfs','/tmp','-v',source+':/source:ro','-v',str(out)+':/recovery']
# Only the recovery directory is writable. Never mount the original data RW.
script='test -z "$(find /source -type l -print -quit)" && du -sb /source && cp -a /source /recovery/data && find /source -type f -exec sha256sum {} + | sort > /recovery/source-before.sha256 && find /recovery/data -type f -exec sha256sum {} + | sort > /recovery/copy-before.sha256 && chown -R 1000:1000 /recovery'
free=shutil.disk_usage('/tmp').free;size=int(run(docker+['run','--rm','--network','none','-v',source+':/source:ro','--entrypoint','du',image,'-sb','/source']).split()[0]);assert free>2*size+2*1024**3,(free,size)
with (out/'copy-original.log').open('w') as log:p=subprocess.run(base+['--entrypoint','sh',image,'-c',script],stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,p.returncode
before=(out/'source-before.sha256').read_text();copied=(out/'copy-before.sha256').read_text();assert before.replace('  /source/','  /recovery/data/')==copied
assert before.replace('  /source/','  /var/lib/nano/')==(audit/'data-before.sha256').read_text()
assert before==run(base+['--entrypoint','sh',image,'-c','find /source -type f -exec sha256sum {} + | sort'])
stopped()
# Remap every signer/witness path to the isolated copy, including inactive keys.
def remap(v):
 if isinstance(v,str):return v.replace('/var/lib/nano/','/recovery/data/')
 if isinstance(v,list):return [remap(x) for x in v]
 if isinstance(v,dict):return {k:remap(x) for k,x in v.items()}
 return v
for name in ['config.json','request.json']:(out/name).write_text(json.dumps(remap(json.loads((audit/name).read_text())),indent=2)+'\n')
proof={'independentCopyVerified':True,'sourceAllWritersStopped':True,'sourceVolume':source,'copyVolume':str(out/'data'),'files':len(before.splitlines()),'copiedBytes':size,'freeBytesBefore':free,'sourceHashesUnchanged':True,'sourceManifestSha256':hashlib.sha256(before.encode()).hexdigest(),'noSignersStarted':True,'originalSourceMountedReadOnly':True,'networkQualified':False,'productionQualified':False}
(out/'copy-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof),flush=True)
