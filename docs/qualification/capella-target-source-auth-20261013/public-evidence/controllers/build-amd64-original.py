#!/usr/bin/env python3
"""ROOT-dispatched targeted AMD64 build from actual27/113/6/3 MD footprint.
Exact external SHA pins bind8be+four coreV2/two testV2 source overlays.
No tests, keys, databases, private campaigns, image build/pull or installations.
"""
import argparse,hashlib,json,os,pathlib,posixpath,re,shlex,shutil,stat,subprocess,sys,time,uuid
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
BASE='8be93c69e144ed6e7f8b8a33ecb308c6fe3d4fdd'
OLD_HEADER='bf8668a4e5119529ec091e7fe5616d93767da243bf63c9a34b3ae4b384706cbb'
CHANGES={'CMakeLists.txt','include/nano/consensus_capella_fork_store.hpp','src/consensus_capella_fork_store.cpp','src/consensus_capella_fork_state.cpp','src/consensus_capella_target_source_auth.hpp','tests/test_capella_target_source_authentication.cpp'}
FIELDS=('st_dev','st_ino','st_uid','st_gid','st_mode','st_size','st_mtime_ns','st_ctime_ns')
def require(ok,why):
 if not ok: raise ValueError(why)
def sha(b): return hashlib.sha256(b).hexdigest()
def canonical(p):
 p=pathlib.Path(p);require(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical nonsymlink path');return p
def relative(p):
 require(isinstance(p,str) and p and not p.startswith('/') and '..' not in pathlib.PurePosixPath(p).parts and '\n' not in p and '\x00' not in p,'bounded relative path');return p
def read(p,limit=16*1024*1024):
 p=canonical(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);require(stat.S_ISREG(a.st_mode) and a.st_size<=limit,'bounded regular public input');out=[];n=0
  while True:
   b=os.read(fd,1024*1024)
   if not b: break
   out.append(b);n+=len(b);require(n<=limit,'public byte bound')
  z=os.fstat(fd);require(all(getattr(a,k)==getattr(z,k) for k in FIELDS),'input changed during read');require(all(getattr(z,k)==getattr(p.lstat(),k) for k in FIELDS),'input replaced during read');return b''.join(out)
 finally:os.close(fd)
def pinned(p,h,limit=16*1024*1024):
 require(isinstance(h,str) and re.fullmatch('[0-9a-f]{64}',h),'literal external SHA required');b=read(p,limit);require(sha(b)==h,'external input SHA mismatch');return b
def hashstat(p):
 p=canonical(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);require(stat.S_ISREG(a.st_mode),'public build regular artifact');h=hashlib.sha256()
  while True:
   b=os.read(fd,1024*1024)
   if not b:break
   h.update(b)
  z=os.fstat(fd);require(all(getattr(a,k)==getattr(z,k) for k in FIELDS),'artifact changed while hashing');require(all(getattr(z,k)==getattr(p.lstat(),k) for k in FIELDS),'artifact replaced');return {'sha256':h.hexdigest(),**{k:getattr(z,k) for k in FIELDS}}
 finally:os.close(fd)
def save(p,v):
 b=(json.dumps(v,indent=2)+'\n').encode();fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
 return sha(b)
def copy_bytes(src,dst,pin):
 before=hashstat(src);require(before['sha256']==pin['sha256'] and before['st_size']==pin.get('bytes',pin.get('st_size')),'source byte pin');dst.parent.mkdir(parents=True,exist_ok=True);require(not dst.exists(),'no output overwrite');shutil.copyfile(src,dst);os.chmod(dst,stat.S_IMODE(before['st_mode']));after=hashstat(dst);require(after['sha256']==before['sha256'] and after['st_size']==before['st_size'] and stat.S_IMODE(after['st_mode'])==stat.S_IMODE(before['st_mode']) and (after['st_dev'],after['st_ino'])!=(before['st_dev'],before['st_ino']),'independent mode-preserving copy');require(hashstat(src)==before,'source changed during copy');return after

def compiler(row):
 a=shlex.split(row['command']);require(a[0]=='/usr/bin/clang++' and a.count('-o')==1 and a.count('-c')==1 and row['directory']=='/outputs','exact compiler namespace');o=relative(a[a.index('-o')+1]);require(o==row.get('output',o),'object output disagreement');return o,a

def deps_bytes(b):
 s=b.decode();require(':' in s,'dependency target');d=shlex.split(s.replace('\\\n',' ').split(':',1)[1]);require(d and all(x.startswith(('/work/','/outputs/','/usr/','/lib/')) and '\n' not in x for x in d),'dependency namespace');return sorted(set(d))
def dependency_argv(argv,obj,probe=False):
 a=[];j=0
 while j<len(argv):
  v=argv[j]
  if v in ('-MF','-MT','-MQ'):require(j+1<len(argv),'dependency argument');j+=2;continue
  if v in ('-MD','-MMD','-MP'):j+=1;continue
  a.append(v);j+=1
 if probe:a[a.index('-o')+1]='/outputs/preflight/'+obj+'.d';a[a.index('-c')]='-M';a+=['-MT',obj]
 else:a+=['-MD','-MF',obj+'.d','-MT',obj]
 return a

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for k in ('controller-sha','plan','plan-sha'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();self_path=canonical(__file__);pinned(self_path,a.controller_sha,1024*1024);plan_path=canonical(a.plan);i=json.loads(pinned(plan_path,a.plan_sha))
 require(i['format']=='nano-target-source-auth-amd64-incremental-plan-v1' and i['status']=='READY_FROZEN_OVERLAYS' and i['baseRevision']==BASE and i['image']==IMAGE,'specific ready8be target-source-auth plan')
 require(set(i['overlays'])==CHANGES and len(i['ownerReferences'])==2,'exact4core+2test overlays')
 require(i['limits']=={'totalWallSeconds':1800,'coreCompileSeconds':120,'consumerCompileSeconds':600,'linkSeconds':120,'probeSeconds':60,'configureSeconds':120,'sdkSeconds':120,'cpus':2,'memory':'4g','pids':256},'finite build-only budgets')
 require(i['expectedCounts']=={'coreFresh':27,'coreReused':113,'archiveMembers':140,'existingConsumersFresh':6,'existingConsumersReused':3,'newTinyConsumerFresh':1,'newLinks':10},'dependency-calculated27/113/6/3/1/10 counts')
 require(os.geteuid()==1000 and os.getegid()==1000 and not any(n.startswith(('NANO_','LD_','DYLD_')) for n in os.environ),'VPS owner1000 and no private/interposition environment')
 qroot=pathlib.Path('/home/ubuntu/nanobees-qualification');parent=canonical(i['parentRoot']);root=canonical(i['outputRoot']);source_parent=parent/'source';parent_build=parent/'build'
 require(parent==qroot/'checkpoint-bls-cache-amd64-20261013/build-pass-01' and root==qroot/'target-source-auth-amd64-20261013/build-pass-01' and not root.exists(),'explicit passed public parent and fresh candidate output')
 hashes={**i['sourceHashes'],**{r:v['sha256'] for r,v in i['overlays'].items()}};require(len(hashes)==i['sourceCountAfterOverlays']==1304,'exact1302public-source plus2new-file scope')
 manifests={};expected_overlays={};protected={}
 def track(r):
  p=canonical(r['path']);require(qroot in p.parents and root not in p.parents and p!=root,'public immutable input namespace');s=hashstat(p);require(s['sha256']==r['sha256'] and s['st_size']==r.get('bytes',r.get('st_size')),'actual public input byte pin');protected[str(p)]=s;return p
 for ref in i['ownerReferences']:
  label=ref['label'];require(label in ('core','test') and label not in manifests,'distinct owner core/test seals');mp=track(ref['manifest']);m=json.loads(pinned(mp,ref['manifest']['sha256']));track(ref['patch']);manifests[label]=m;require(m['baselineGitCommit']==BASE,'owner source baseline8be')
  for row in m['sourceFiles']:
   require(row['path'] not in expected_overlays,'overlapping owner files');expected_overlays[row['path']]=row['sha256'];require(i['overlays'][row['path']]['path']==str(mp.parent/'source'/row['path']),'owner source exact path')
   if row.get('baselineSha256') is not None:require(i['sourceHashes'][row['path']]==row['baselineSha256'],'overlay baseline source pin')
 require(expected_overlays=={p:r['sha256'] for p,r in i['overlays'].items()},'owner6source hashes differ')
 require(i['ownerReferences'][0]['manifest']['sha256']=='da39fde2008dcbe960e67b10e0d538f2456aab9d19c86756d291533f99113b06' and i['ownerReferences'][0]['patch']['sha256']=='d343c652213df5f1c9187b0f7b7794f518560d293466b2da02011c02847bf5fd' and i['ownerReferences'][1]['manifest']['sha256']=='ab076fcb70078ea18d44184cd84f47ec526a9a2ae37260ea2eccd58f32d2aead' and i['ownerReferences'][1]['patch']['sha256']=='beb5fccd7cea03722c9a08efd5c12dc46dd58daf439ce2094481187079fac8d2','literal frozen coreV2/testV2 proposals')
 for m in manifests.values():
  for r in m['protectedUnchangedSourceBindings']:require(i['sourceHashes'][r['path']]==r['sha256'],'protected source/API/scenario changed')
 terminal=json.loads(pinned(track(i['parentTerminal']),i['parentTerminal']['sha256']));require(terminal['format']=='nano-bls-cache-amd64-incremental-terminal-v1' and terminal['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and terminal['archive']['sha256']==i['parentArchive']['sha256'],'actual passed BLS140 parent, not earlier archive')
 if not root.parent.exists():os.mkdir(root.parent,0o700)
 parent_dir=root.parent.lstat();require(stat.S_ISDIR(parent_dir.st_mode) and parent_dir.st_uid==1000 and stat.S_IMODE(parent_dir.st_mode)==0o700,'owned700 output parent')
 os.mkdir(root,0o700);require(not list(root.iterdir()),'atomic fresh output');source=root/'source';build=root/'build';receipts=root/'receipts';controls=root/'controls'
 for p in (source,build,receipts,controls):p.mkdir(mode=0o700)
 (build/'tmp').mkdir(mode=0o700);save(receipts/'initial-empty-output.json',{'path':str(root),'atomicMkdir':True,'initialEntries':[],'mode':'0700'})
 start=time.monotonic();env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};phase=0;source_before={};copies={};metadata={};failure=None;fresh=0;reused=0;programs=[]
 def tick():require(time.monotonic()-start<1800,'finite1800s build budget exceeded')
 def host(label,argv,seconds):
  nonlocal phase
  tick();phase+=1;save(receipts/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds});began=time.monotonic();fd=os.open(receipts/(label+'.log'),os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
  try:
   with os.fdopen(fd,'wb',buffering=0) as out:r=subprocess.run(argv,stdout=out,stderr=subprocess.STDOUT,timeout=min(seconds,max(1,1800-(time.monotonic()-start))),env=env)
  except subprocess.TimeoutExpired:save(receipts/(label+'.exit.json'),{'exitCode':None,'timedOut':True,'elapsedSeconds':time.monotonic()-began});raise
  save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-began});require(r.returncode==0,'actual command refused: '+label);print(json.dumps({'phase':label,'elapsedSeconds':time.monotonic()-start}),flush=True);return read(receipts/(label+'.log'))
 def container(label,argv,seconds):
  token=uuid.uuid4().hex;name='nano-target-auth-'+token[:16];inspect=['/usr/bin/sudo','-n','/usr/bin/docker','container','inspect','--format','{{json .Config.Labels}}',name]
  cmd=['/usr/bin/sudo','-n','/usr/bin/docker','run','--rm','--pull=never','--platform','linux/amd64','--name',name,'--label','nano.source.auth.build.owner='+token,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/outputs/tmp','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(build)+',dst=/outputs','--mount','type=bind,src='+str(controls)+',dst=/controls,readonly','--workdir','/outputs','--entrypoint',argv[0],IMAGE]+argv[1:]
  try:return host(label,cmd,seconds)
  finally:
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20)
   if r.returncode==0:
    require(json.loads(r.stdout).get('nano.source.auth.build.owner')==token,'owned-only cleanup');rm=['/usr/bin/sudo','-n','/usr/bin/docker','container','rm','--force',name];save(receipts/(label+'-cleanup.command.json'),{'argv':rm,'outsideExpiredWallBudget':True});r=subprocess.run(rm,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20);require(r.returncode==0,'owned cleanup failed')
   else:require(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'ambiguous absence')
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20);require(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'owned container remains')
 def bind_project(d):
  for x in d:
   if x.startswith('/work/'):
    rel=posixpath.normpath(x[6:]);relative(rel);require(rel in hashes and hashstat(source/rel)['sha256']==hashes[rel],'unsealed/changed project input: '+rel)
 def control(name,body):
  p=controls/name;fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
  with os.fdopen(fd,'wb') as out:out.write(body);out.flush();os.fsync(out.fileno())
  return hashstat(p)
 try:
  image=json.loads(host('image-inspect',['/usr/bin/sudo','-n','/usr/bin/docker','image','inspect','--format','{{json .}}',IMAGE],30));require(image['Id']==IMAGE and image['Architecture']=='amd64' and image['Os']=='linux','existing pinned AMD image only');require(not any(x.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for x in image.get('Config',{}).get('Env',[])),'private/interposition image environment')
  for rel,h in hashes.items():
   tick();relative(rel);ref=i['overlays'][rel] if rel in i['overlays'] else {'path':i['sourceOrigins'][rel],'sha256':h,'bytes':i['sourceBytes'][rel]};origin=track(ref);source_before[rel]=copy_bytes(origin,source/rel,ref)
  actual=[]
  for base,dirs,files in os.walk(source,followlinks=False):
   require(not any((pathlib.Path(base)/n).is_symlink() for n in dirs+files),'copied source symlink');actual.extend(str((pathlib.Path(base)/n).relative_to(source)) for n in files)
  require(set(actual)==set(hashes),'exact copied public source allowlist')
  track(i['parentArchive']);selected=set();consumer_selected=set();core_changes={r['path'] for r in manifests['core']['sourceFiles']}
  for row in i['core']+i['consumers']:
   track(row['objectInput']);md=track(row['mdInput']);d=deps_bytes(read(md));require(d==row['dependencies'],'actual inherited MD differs');bind_project(d);project={posixpath.normpath(x[6:]) for x in d if x.startswith('/work/')};want=bool(core_changes.intersection(project));require(want is row['fresh'],'derived dependency footprint differs')
   if want:(consumer_selected if 'target' in row else selected).add(row.get('target',row['source']))
  require(len(i['core'])==140 and len({r['object'] for r in i['core']})==140 and len(i['consumers'])==9 and len({r['target'] for r in i['consumers']})==9 and len(selected)==27 and len(consumer_selected)==6,'actual140/9 selected27/6 graph')
  for ref in i['compilerMetadata'].values():track(ref)
  save(receipts/'parents-before.json',protected);save(receipts/'source-before.json',source_before);save(receipts/'selection.json',{'freshCore':sorted(selected),'reusedCore':113,'freshExistingConsumers':sorted(consumer_selected),'reusedExistingConsumers':3,'freshNewTiny':1,'oldMdFabricated':False})
  container('configure',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],120)
  cc=json.loads(read(build/'compile_commands.json'));key=lambda r:(r['file'],r.get('output'),r['directory']);old={key(r):r for r in i['compileCommands']};new={key(r):r for r in cc};require(len(old)==len(i['compileCommands']) and len(new)==len(cc),'duplicate compile graph')
  for k,r in old.items():require(new.get(k)==r,'old compiler argv changed; ABI reuse forbidden')
  extra=[r for k,r in new.items() if k not in old];require(len(extra)==1 and extra[0]['file']=='/work/'+i['newSource'],'only new tiny unit added')
  core={r['source']:compiler(next(c for c in cc if c['file']=='/work/'+r['source'] and 'nano_core.dir/' in c['command'])) for r in i['core']};consumers={r['target']:compiler(next(c for c in cc if c['file']=='/work/'+r['source'] and compiler(c)[0]==r['object'])) for r in i['consumers']};tiny=compiler(extra[0]);links={}
  for rel,ref in i['compilerMetadata'].items():
   if rel=='compile_commands.json':continue
   require(hashstat(build/rel)['sha256']==ref['sha256'],'generated old flags/link changed');metadata[rel]=hashstat(build/rel)
  for target in list(consumers)+[i['newTarget']]:
   for leaf in ('flags.make','link.txt'):
    rel='CMakeFiles/'+target+'.dir/'+leaf;metadata[rel]=hashstat(build/rel)
   links[target]=read(build/'CMakeFiles'/(target+'.dir')/'link.txt').decode()
  metadata['compile_commands.json']=hashstat(build/'compile_commands.json')
  for rel,ref in i['libraries'].items():copies[rel]=copy_bytes(track(ref),build/rel,ref)
  for rel,ref in i['generated'].items():
   track(ref);require(hashstat(build/rel)['sha256']==ref['sha256'],'configure generated KZG differs');copies[rel]=hashstat(build/rel)
  save(receipts/'generated-metadata-before.json',metadata);units={r['source']:core[r['source']] for r in i['core'] if r['fresh']};units.update({r['source']:consumers[r['target']] for r in i['consumers'] if r['fresh']});units[i['newSource']]=tiny;probes={}
  require(len(units)==34,'27core+6oldconsumer+1tiny preflights required')
  for rel,(obj,argv) in units.items():
   (build/'preflight'/(obj+'.d')).parent.mkdir(parents=True,exist_ok=True);container('probe-'+pathlib.Path(rel).stem,dependency_argv(argv,obj,True),60);d=deps_bytes(read(build/'preflight'/(obj+'.d')));bind_project(d);probes[obj]=d
  deps={x for r in i['core']+i['consumers'] if not r['fresh'] for x in r['dependencies']}|{x for d in probes.values() for x in d};sdk=set(i['oldSdkHashes'])|{x for x in deps if not x.startswith('/work/')}
  for text in links.values():
   argv=shlex.split(text);require(argv[0]=='/usr/bin/clang++' and argv.count('-o')==1 and argv.count('libnano_core.a')==1,'exact compiler linker recipe');sdk.update(x for x in argv if x.startswith(('/usr/','/lib/')));libnames={x for x in argv if not x.startswith('/') and x.endswith(('.a','.so','.o')) and not x.startswith('CMakeFiles/') and x!='libnano_core.a'};require(libnames<=set(i['libraries']),'unsealed direct link dependency')
  require(all(x.startswith(('/outputs/','/usr/','/lib/')) and '\n' not in x for x in sdk),'bounded SDK namespaces');generated={x[9:]:hashstat(build/relative(x[9:])) for x in sdk if x.startswith('/outputs/')};require(set(generated)<=set(copies),'unsealed generated dependency')
  body=('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(x)+'\n' for x in sorted(sdk))).encode();worker=control('sdk-hashes.sh',body);sdk_before=container('sdk-before',['/bin/sh','/controls/sdk-hashes.sh'],120);sb={n:h for h,n in (l.split('  ',1) for l in sdk_before.decode().splitlines())};require(len(sb)==len(sdk) and all(sb[n]==h for n,h in i['oldSdkHashes'].items()),'same actual2193 SDK/image bindings required')
  save(receipts/'sdk-before-binding.json',{'qualifiedParentPaths':len(i['oldSdkHashes']),'currentPaths':len(sdk),'beforeSha256':sha(sdk_before),'historicalSdkInvented':False});save(receipts/'parents-before-first-compile.json',protected)
  for row in i['core']:
   obj,argv=core[row['source']];require(obj==row['object'],'core object mapping');(build/obj).parent.mkdir(parents=True,exist_ok=True)
   if row['fresh']:container('compile-core-'+pathlib.Path(row['source']).stem,dependency_argv(argv,obj),120);require(deps_bytes(read(build/(obj+'.d')))==probes[obj],'actual core MD differs from preflight');fresh+=1
   else:copy_bytes(row['objectInput']['path'],build/obj,row['objectInput']);reused+=1
  require((fresh,reused)==(27,113),'only27fresh113reusedcore')
  arch=read(build/'CMakeFiles/nano_core.dir/link.txt').decode().splitlines();require(len(arch)==2,'archive two-command recipe');ar=shlex.split(arch[0]);ran=shlex.split(arch[1]);require(ar[:3]==['/usr/bin/ar','qc','libnano_core.a'] and len(ar[3:])==140 and set(ar[3:])=={v[0] for v in core.values()} and ran==['/usr/bin/ranlib','libnano_core.a'],'archive140exactoperands');container('archive',ar,120);container('ranlib',ran,60);names=container('archive-members',['/usr/bin/ar','t','libnano_core.a'],60).decode().splitlines();archive=hashstat(build/'libnano_core.a');objects={obj:hashstat(build/obj) for obj,_ in core.values()};byname={pathlib.Path(obj).name:(obj,s) for obj,s in objects.items()};require(len(names)==140 and len(set(names))==140 and set(names)==set(byname) and all(re.fullmatch('[A-Za-z0-9_.+-]+',n) for n in names),'flat unique140ar members')
  control('members.txt',(''.join(n+'\n' for n in names)).encode());member_body=b'''#!/bin/sh
set -eu
while IFS= read -r member; do
 /usr/bin/ar p /outputs/libnano_core.a "$member" > /outputs/tmp/member1
 h1=$(/usr/bin/sha256sum /outputs/tmp/member1 | cut -d ' ' -f1)
 n1=$(wc -c < /outputs/tmp/member1)
 /usr/bin/ar p /outputs/libnano_core.a "$member" > /outputs/tmp/member2
 h2=$(/usr/bin/sha256sum /outputs/tmp/member2 | cut -d ' ' -f1)
 n2=$(wc -c < /outputs/tmp/member2)
 printf '%s\t%s\t%s\t%s\t%s\n' "$member" "$h1" "$n1" "$h2" "$n2"
 rm -- /outputs/tmp/member1 /outputs/tmp/member2
done < /controls/members.txt
''';member_worker=control('member-proof.sh',member_body);raw=container('archive-raw-members',['/bin/sh','/controls/member-proof.sh'],120);proof=[]
  for line in raw.decode().splitlines():
   n,h1,n1,h2,n2=line.split('\t');require(n in byname,'known member');obj,s=byname[n];require(h1==h2==s['sha256'] and int(n1)==int(n2)==s['st_size'],'actual raw member bytes differ');proof.append({'name':n,'object':obj,'sha256':h1,'bytes':int(n1)})
  require(len(proof)==140 and len({r['name'] for r in proof})==140,'actual140raw member proofs');save(receipts/'archive-140-raw-member-proof.json',{'archive':archive,'members':proof,'arPInvocations':280,'objectPayloadExported':False})
  for row in i['consumers']:
   obj,argv=consumers[row['target']];(build/obj).parent.mkdir(parents=True,exist_ok=True)
   if row['fresh']:container('compile-consumer-'+row['target'],dependency_argv(argv,obj),600);require(deps_bytes(read(build/(obj+'.d')))==probes[obj],'actual consumer MD differs from preflight')
   else:copy_bytes(row['objectInput']['path'],build/obj,row['objectInput'])
  obj,argv=tiny;(build/obj).parent.mkdir(parents=True,exist_ok=True);container('compile-new-tiny',dependency_argv(argv,obj),600);require(deps_bytes(read(build/(obj+'.d')))==probes[obj],'actual new tiny MD differs from preflight')
  for target,text in links.items():
   argv=shlex.split(text);obj=tiny[0] if target==i['newTarget'] else consumers[target][0];require(argv[argv.index('-o')+1]==target and [x for x in argv if x.endswith('.o')]==[obj,'third_party/mcl/bint64.o','third_party/mcl/base64.o'],'exact direct consumer/MCL objects');container('link-'+target,argv,120);programs.append({'target':target,'object':hashstat(build/obj),'binary':hashstat(build/target),'coreArchiveSha256':archive['sha256'],'objectReused':target in consumers and target not in consumer_selected,'binaryReused':False})
  sdk_after=container('sdk-after',['/bin/sh','/controls/sdk-hashes.sh'],120);require(sdk_before==sdk_after and hashstat(controls/'sdk-hashes.sh')==worker and hashstat(controls/'member-proof.sh')==member_worker,'SDK/worker changed')
  require({p:hashstat(p) for p in protected}==protected and {p:hashstat(source/p) for p in source_before}==source_before,'parent/source input changed');require({p:hashstat(build/p) for p in copies}==copies and {p:hashstat(build/p) for p in generated}==generated,'library/generated copy changed');require({p:hashstat(build/p) for p in metadata}==metadata and {p:hashstat(build/p) for p in objects}==objects and hashstat(build/'libnano_core.a')==archive,'metadata/core/archive changed after links');pinned(self_path,a.controller_sha);pinned(plan_path,a.plan_sha)
  save(receipts/'parents-after.json',protected);save(receipts/'source-after.json',source_before);save(receipts/'sdk-before-after-binding.json',{'beforeSha256':sha(sdk_before),'afterSha256':sha(sdk_after),'equal':True,'currentPaths':len(sdk),'parentQualifiedPaths':2193,'noHistoricalSdkInvented':True})
  terminal={'format':'nano-target-source-auth-amd64-incremental-terminal-v1','status':'BUILD_PASS_TESTS_NOT_EXECUTED','baseRevision':BASE,'controllerSha256':a.controller_sha,'planSha256':a.plan_sha,'coreManifestSha256':i['ownerReferences'][0]['manifest']['sha256'],'testManifestSha256':i['ownerReferences'][1]['manifest']['sha256'],**i['expectedCounts'],'archive':archive,'programs':programs,'actualDependenciesEqualPreflight':True,'allParentSourceLibraryGeneratedSdkBindingsUnchanged':True,'oldMdFabricated':False,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'imageBuiltOrPulled':False,'productionQualified':False}
 except BaseException as error:
  failure=type(error).__name__+': '+str(error);terminal={'format':'nano-target-source-auth-amd64-incremental-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','failure':failure,'coreFreshCompleted':fresh,'coreReusedCopied':reused,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
  try:terminal['protectedInputsUnchangedAfterFailure']=({p:hashstat(p) for p in protected}==protected)
  except BaseException as diag:terminal['failureReadbackUnavailable']=type(diag).__name__
 save(receipts/'terminal-build-receipt.json',terminal);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
