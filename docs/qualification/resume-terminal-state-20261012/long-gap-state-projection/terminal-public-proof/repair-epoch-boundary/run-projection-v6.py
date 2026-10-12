#!/usr/bin/env python3
"""One bounded read-only diagnostic; private manifests remain on the VPS."""
from pathlib import Path
import hashlib,json,subprocess,time,os,sys,shlex
out=Path('/home/ubuntu/nanobees-qualification/long-gap-state-projection-20261012/repair-epoch-boundary')
source=Path('/home/ubuntu/nanobees-qualification/v37-source-preserved-20261012')
repo=Path('/home/ubuntu/blockchain-nano-git')
image='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
volume='nano-clean-rpc-build-20261002';name='nano-v37-long-gap-projection-epoch-20261012'
build='/diagnostic/build';core='/build/publication-cancel-boundary-amd64-20261007/libnano_core.a'
assert out.is_dir() and source.is_dir()
def save(name,value): (out/name).write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
def manifest():
 result={};groups={}
 for group in ['recovered-data','private-audit','config-origin']:
  count=size=0
  for p in sorted((source/group).rglob('*')):
   if p.is_symlink():raise RuntimeError('Source symlink refused')
   if p.is_file():result[str(p.relative_to(source))]=sha(p);count+=1;size+=p.stat().st_size
  groups[group]={'files':count,'bytes':size}
 return result,groups
before,groups=manifest();assert groups['recovered-data']['files']==3220 and groups['private-audit']['files']==16 and groups['config-origin']['files']==5
assert not (out/'original-result.json').exists(),'Do not rerun an original projection'
save('private-source-before.json',before);save('source-group-counts.json',groups)
save('input-bindings.json',{g:sha(source/'config-origin'/g) for g in ['genesis.json','profile.json','initial.json']})
save('controller-state.json',{'pid':os.getpid(),'status':'running','startedUnixMs':int(time.time()*1000),'container':name})
subprocess.run(['git','-C',str(repo),'diff','--quiet','85eb492f','--','src','include','third_party'],check=True)
active=subprocess.run(['sudo','-n','docker','ps','-q'],check=True,capture_output=True,text=True).stdout.split()
for ident in active:
 row=json.loads(subprocess.run(['sudo','-n','docker','inspect',ident],check=True,capture_output=True,text=True).stdout)[0]
 for mount in row['Mounts']:
  if str(mount.get('Source','')).startswith(str(source)) and mount.get('RW'):raise RuntimeError('Source mounted writable by another container')
assert subprocess.run(['sudo','-n','docker','ps','-aq','--filter','name=^/'+name+'$'],check=True,capture_output=True,text=True).stdout.strip()==''
compile_args=['/usr/bin/clang++','-DMCL_BINT_ASM_X64=0','-DMCL_DONT_USE_XBYAK','-DMCL_FP_BIT=384','-DMCL_FR_BIT=256','-DMCL_MSM=0','-DMCL_PORTABLE=1','-DMCL_USE_LLVM=1','-I/work/third_party/ckzg/blst/bindings','-I/work/include','-I/work/src','-I/work/third_party','-I/work/third_party/mcl/include','-O3','-DNDEBUG','-std=gnu++20','-c','/diagnostic/project-long-gap-v3.cpp','-o',build+'/project-long-gap.o']
link_args=['/usr/bin/clang++','-O3','-DNDEBUG',build+'/project-long-gap.o','-o',build+'/project-long-gap','third_party/mcl/bint64.o','third_party/mcl/base64.o',core,'third_party/ckzg/libnano_ckzg.a','third_party/ckzg/libnano_blst.a','third_party/secp256k1/lib/libsecp256k1.a','/usr/lib/x86_64-linux-gnu/libgmpxx.so','/usr/lib/x86_64-linux-gnu/libgmp.so','/usr/lib/x86_64-linux-gnu/libssl.so','/usr/lib/x86_64-linux-gnu/libcrypto.so','/usr/lib/x86_64-linux-gnu/libicuuc.so','-ldl','/usr/lib/x86_64-linux-gnu/librocksdb.so','lib/libmcl.a']
save('commands.json',{'compile':compile_args,'link':link_args,'run':[build+'/project-long-gap','--project-preserved-synthetic-v37'],'cwd':'/build/coherent-e33ad89e-amd64','coreRevision':'85eb492f','containerLimits':{'memory':'2g','cpus':'2','network':'none','nofile':65536}})
# The in-container driver has no source-writing API. Source/read-only-work bind mounts are kernel enforced.
inside='\n'.join([
 'mkdir -p '+shlex.quote(build),
 'cd /build/coherent-e33ad89e-amd64 || exit 1',
 'sha256sum '+shlex.quote(core)+' > /diagnostic/core-before.sha256',
 shlex.join(compile_args)+' > /diagnostic/compile.log 2>&1',
 'compile_status=$?',
 "printf '{\"exitCode\":%s}\\n' \"$compile_status\" > /diagnostic/compile-exit.json",
 'if [ "$compile_status" -ne 0 ]; then exit "$compile_status"; fi',
 shlex.join(link_args)+' > /diagnostic/link.log 2>&1',
 'link_status=$?',
 "printf '{\"exitCode\":%s}\\n' \"$link_status\" > /diagnostic/link-exit.json",
 'if [ "$link_status" -ne 0 ]; then exit "$link_status"; fi',
 'sha256sum '+shlex.quote(build+'/project-long-gap')+' '+shlex.quote(core)+' > /diagnostic/binaries.sha256',
 shlex.join([build+'/project-long-gap','--project-preserved-synthetic-v37'])+' > /diagnostic/projection.jsonl 2>&1',
 'projection_status=$?',
 "printf '{\"exitCode\":%s,\"controllerTimeout\":false}\\n' \"$projection_status\" > /diagnostic/projection-exit.json",
 'sha256sum '+shlex.quote(core)+' > /diagnostic/core-after.sha256',
 'exit "$projection_status"',
])
args=['sudo','-n','docker','run','--name',name,'--network','none','--user','1000:1000','--read-only','--tmpfs','/tmp','--cap-drop','ALL','--security-opt','no-new-privileges','--ulimit','nofile=65536:65536','--memory','2g','--cpus','2','-v',volume+':/build:ro','-v',str(repo)+':/work:ro','-v',str(source)+':/source:ro','-v',str(out)+':/diagnostic','--entrypoint','/bin/sh',image,'-c',inside]
save('docker-command.json',{'args':args})
try:
 with (out/'container-controller.log').open('w') as log:r=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,timeout=750)
 code=r.returncode;timed=False
except subprocess.TimeoutExpired:
 code=124;timed=True
 subprocess.run(['sudo','-n','docker','stop','--time','10',name],capture_output=True,text=True)
state=json.loads(subprocess.run(['sudo','-n','docker','inspect',name,'--format','{{json .State}}'],check=True,capture_output=True,text=True).stdout)
save('container-state.json',state)
after,after_groups=manifest();save('private-source-after.json',after)
unchanged=before==after
save('source-immutability-proof.json',{'fullFileHashesBeforeAfterEqual':unchanged,'groups':groups,'afterGroups':after_groups,'files':len(before),'beforeManifestSha256':hashlib.sha256(json.dumps(before,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'afterManifestSha256':hashlib.sha256(json.dumps(after,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'sourceMountedReadOnly':True,'sourceOpenedReadOnly':True,'privateManifestsExported':False})
terminal=None
if (out/'projection.jsonl').exists():
 for line in (out/'projection.jsonl').read_text().splitlines():
  try:row=json.loads(line)
  except ValueError:continue
  if row.get('event')=='projection-terminal':terminal=row
result={'controllerExitCode':code,'containerExitCode':state['ExitCode'],'containerOomKilled':state['OOMKilled'],'controllerTimeout':timed,'sourceFullHashComparison':unchanged,'terminal':terminal,'source':str(source),'artifacts':str(out),'networkQualified':False,'productionQualified':False}
save('original-result.json',result);save('controller-state.json',{'pid':os.getpid(),'status':'terminal','exitCode':code,'finishedUnixMs':int(time.time()*1000),'container':name})
print(json.dumps(result));sys.exit(code if unchanged else 2)
