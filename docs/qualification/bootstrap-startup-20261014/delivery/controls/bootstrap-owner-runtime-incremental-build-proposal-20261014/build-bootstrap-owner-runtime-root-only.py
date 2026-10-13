#!/usr/bin/env python3
"""Root-only coherent public-source Docker build. Exact plan seals; no private DBs/runtime.
Reuse requires actual existing MD, same compiler argv, source/generated/SDK bytes.
New tests are run separately after the terminal build receipt, not by this driver.
"""
import argparse, hashlib, json, os, pathlib, posixpath, re, shlex, shutil, stat, subprocess, sys, time, uuid
IMAGES={'arm64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb',
        'amd64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'}
FIELDS=['st_dev','st_ino','st_uid','st_gid','st_mode','st_size','st_mtime_ns','st_ctime_ns']
BASE='6a4ef3d9be2f5649129415f1bc158e2a211ed4f6'
def require(ok,why):
 if not ok: raise ValueError(why)
def digest(b): return hashlib.sha256(b).hexdigest()
def canonical(p):
 p=pathlib.Path(p);require(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlaps(a,b):return a==b or a in b.parents or b in a.parents
def relative(p):
 require(isinstance(p,str) and p and not p.startswith('/') and pathlib.PurePosixPath(p).as_posix()==p and '..' not in pathlib.PurePosixPath(p).parts and '\n' not in p and '\0' not in p,'relative path');return p
def material(arch): return FIELDS[:-1] if arch=='arm64' else FIELDS
def observed(p,arch,limit=None,bytes_out=False):
 p=canonical(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW)
 try:
  a=os.fstat(fd);require(stat.S_ISREG(a.st_mode) and (limit is None or a.st_size<=limit),'bounded regular public artifact');h=hashlib.sha256();parts=[];size=0
  while True:
   b=os.read(fd,1048576)
   if not b:break
   size+=len(b);require(limit is None or size<=limit,'byte limit');h.update(b)
   if bytes_out:parts.append(b)
  z=os.fstat(fd);require(all(getattr(a,k)==getattr(z,k)==getattr(p.lstat(),k) for k in material(arch)),'public artifact changed');s={'sha256':h.hexdigest(),**{k:getattr(z,k) for k in FIELDS}}
  return (b''.join(parts),s) if bytes_out else s
 finally:os.close(fd)
def save(p,v):
 b=(json.dumps(v,indent=2)+'\n').encode();fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
 return digest(b)
def dep(b):
 s=b.decode();require(':' in s,'MD target');d=sorted(set(shlex.split(s.replace('\\\n',' ').split(':',1)[1])));require(d and all(v.startswith(('/work/','/outputs/','/usr/','/lib/')) for v in d),'MD namespace');return d
def unit(row):
 a=shlex.split(row['command']);require(row['directory']=='/outputs' and a[0]=='/usr/bin/clang++' and a.count('-o')==a.count('-c')==1,'compiler argv');return relative(a[a.index('-o')+1]),a
def dependency_argv(a,obj,probe):
 result=[];j=0
 while j<len(a):
  if a[j] in ('-MF','-MT','-MQ'):j+=2;continue
  if a[j] in ('-MD','-MMD','-MP'):j+=1;continue
  result.append(a[j]);j+=1
 if probe:result[result.index('-c')]='-M';result[result.index('-o')+1]='preflight/'+obj+'.d';result+=['-MT',obj]
 else:result+=['-MD','-MF',obj+'.d','-MT',obj]
 return result
def main():
 ap=argparse.ArgumentParser()
 for k in ('controller-sha','plan','plan-sha'):ap.add_argument('--'+k,required=True)
 a=ap.parse_args();planpath=canonical(a.plan)
 # Arch is read as a bounded hint, then the same held bytes are authenticated.
 raw,seal=observed(planpath,'arm64',16*1024*1024,True);require(digest(raw)==a.plan_sha,'external plan SHA');p=json.loads(raw);arch=p['arch'];require(arch in IMAGES,'architecture')
 selfpath=canonical(__file__);rawself,selfseal=observed(selfpath,arch,1024*1024,True);require(digest(rawself)==a.controller_sha,'controller external SHA')
 require(p['format']=='nano-bootstrap-owner-runtime-incremental-plan-v1' and p['status']=='READY_ROOT_DISPATCH_ONLY' and p['baselineGitCommit']==BASE and p['image']==IMAGES[arch],'exact qualified baseline/image')
 require(os.geteuid()==p['uid'] and os.getegid()==p['gid'] and not any(k.startswith(('NANO_','LD_','DYLD_')) for k in os.environ),'public owner/environment')
 root=canonical(p['outputRoot']);require(not root.exists(),'fresh build output');parent=canonical(p['coreParent']);origin=canonical(p['sourceOrigin']);parent_build=canonical(p['parentBuild']);require(not any(overlaps(root,q) for q in (parent,parent_build,origin,planpath.parent,selfpath.parent)), 'new output overlaps protected parent/source/control tree');require(len(p['core'])==141 and len(p['sourcePins'])==1314 and len(p['libraries'])==7 and len(p['consumers'])==2,'qualified parent closure dimensions')
 require({c['target'] for c in p['consumers']}=={'nano','nano-capella-bootstrap-archive-cas-tests'} and p['newCore']=='src/consensus_capella_bootstrap_input.cpp','specific core/runtime/regression batch')
 require(set(p['overlays'])=={'CMakeLists.txt','include/nano/chain.hpp','include/nano/storage.hpp','src/chain.cpp','src/storage.cpp','include/nano/consensus_capella_bootstrap_input.hpp','src/consensus_capella_bootstrap_input.cpp','src/consensus_capella_runtime.cpp','src/main.cpp','tests/test_capella_bootstrap_archive_cas.cpp',p['newTestSource']},'specific eleven-path owner/input/runtime/test batch')
 protected={};diagnostics=[]
 def pin(r):
  path=canonical(r['path']);b,s=observed(path,arch,None,True);require(s['sha256']==r['sha256'] and s['st_size']==r['bytes'],'public byte pin')
  protected[str(path)]=s;return b
 def same(x,y,label):
  require(x['sha256']==y['sha256'] and all(x[k]==y[k] for k in material(arch)),label)
  if arch=='arm64' and x['st_ctime_ns']!=y['st_ctime_ns']:diagnostics.append({'scope':label,'old':y['st_ctime_ns'],'new':x['st_ctime_ns'],'materialAndBytesEqual':True})
 def copy(r,dst):
  data=pin(r);dst.parent.mkdir(parents=True,exist_ok=True);require(not dst.exists(),'new output copy');fd=os.open(dst,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,stat.S_IMODE(protected[r['path']]['st_mode']))
  with os.fdopen(fd,'wb') as f:f.write(data)
  require(observed(dst,arch)['sha256']==r['sha256'],'copied bytes')
 for r in p['authorManifests']+p['references']:pin(r)
 archive=pin(p['parentArchive']);require(digest(archive)==p['parentArchive']['sha256'],'parent core')
 parent_terminal=json.loads(pin(p['parentTerminal']));raw_parent=json.loads(pin(p['parentRawMemberProof']))
 parent_members=raw_parent['members'] if isinstance(raw_parent,dict) else raw_parent
 require(parent_terminal['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and parent_terminal['archive']['sha256']==p['parentArchive']['sha256'] and parent_terminal['archiveMembers']==141,'actual qualified141 parent terminal')
 require(len(parent_members)==141 and len({r['member'] for r in parent_members})==141 and {r['object'] for r in parent_members}=={r['object'] for r in p['core']},'actual parent141 member identity closure')
 parent_member_map={r['object']:r for r in parent_members}
 for row in p['core']:
  member=parent_member_map[row['object']];require(member['sha256']==row['objectInput']['sha256'] and member['bytes']==row['objectInput']['bytes'] and member['sha256']==parent_terminal['coreObjects'][row['object']]['sha256'],'parent object compiled/member/terminal SHA binding')
 for row in p['core']:
  pin(row['objectInput']);require(dep(pin(row['mdInput']))==row['dependencies'],'actual MD parent binding')
 for c in p['consumers']:
  for k in ('objectInput','mdInput','flagsInput','linkInput'):pin(c[k])
  require(dep(pin(c['mdInput']))==c['dependencies'],'consumer actual MD')
 for r in p['libraries'].values():pin(r)
 for r in p['generated'].values():pin(r)
 for r in p['parentOtherProtected']:pin(r)
 os.mkdir(root,0o700);build=root/'build';source=root/'source';controls=root/'controls';receipts=root/'receipts'
 for d in (build,source,controls,receipts):d.mkdir(mode=0o700)
 save(receipts/'initial-empty-output.json',{'outputWasAbsentBeforeMkdir':True,'buildEntriesBeforeAnyCopy':sorted(x.name for x in build.iterdir()),'sourceEntriesBeforeAnyCopy':sorted(x.name for x in source.iterdir()),'qualifiedHistoricalEmptyDirInvented':False});require(not any(build.iterdir()) and not any(source.iterdir()),'fresh empty build/source before any copy');
 (build/'tmp').mkdir(mode=0o700);began=time.monotonic();env={'PATH':'/usr/bin:/bin','LC_ALL':'C'};failure=None;counts={'freshCore':0,'reusedCore':0,'newCore':0,'freshConsumers':0,'reusedConsumers':0,'newTest':0,'links':0};objects={};programs={};sourcebefore={};copies={};workers={};metadata={}
 def command(label,argv,seconds):
  require(time.monotonic()-began<3000,'3000s finite total');save(receipts/(label+'.command.json'),{'argv':argv,'timeoutSeconds':seconds});start=time.monotonic()
  with (receipts/(label+'.log')).open('xb',buffering=0) as f:
   try:r=subprocess.run(argv,stdout=f,stderr=subprocess.STDOUT,env=env,timeout=min(seconds,max(1,3000-(time.monotonic()-began))))
   except subprocess.TimeoutExpired:save(receipts/(label+'.exit.json'),{'exitCode':None,'timedOut':True});raise
  save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'timedOut':False,'elapsedSeconds':time.monotonic()-start});require(r.returncode==0,'phase failed '+label);print(json.dumps({'phase':label,'elapsedSeconds':time.monotonic()-began}),flush=True);return observed(receipts/(label+'.log'),arch,None,True)[0]
 def container(label,argv,seconds):
  token=uuid.uuid4().hex;name='nano-bootstrap-cas-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name]
  cmd=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.bootstrap.cas.owner='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/outputs/tmp','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={build},dst=/outputs','--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',argv[0],p['image']]+argv[1:]
  try:return command(label,cmd,seconds)
  finally:
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20)
   if r.returncode==0:
    require(json.loads(r.stdout).get('nano.bootstrap.cas.owner')==token,'owned cleanup');rr=subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20);require(rr.returncode==0,'cleanup removal')
   else:require(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'cleanup state')
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20);require(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'owned container remains')
 def parent_container(label,argv,seconds):
  token=uuid.uuid4().hex;name='nano-parent-capture-'+token[:12];docker=p['dockerArgv'];inspect=docker+['container','inspect','--format','{{json .Config.Labels}}',name]
  cmd=docker+['run','--rm','--pull=never','--platform','linux/'+arch,'--name',name,'--label','nano.bootstrap.cas.owner='+token,'--network','none','--read-only','--user',str(p['uid'])+':'+str(p['gid']),'--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/proof/tmp','--mount',f'type=bind,src={origin},dst=/work,readonly','--mount',f'type=bind,src={p["parentBuild"]},dst=/outputs,readonly','--mount',f'type=bind,src={build/"parent-proof"},dst=/proof','--mount',f'type=bind,src={controls},dst=/controls,readonly','--workdir','/outputs','--entrypoint',argv[0],p['image']]+argv[1:]
  try:return command(label,cmd,seconds)
  finally:
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20)
   if r.returncode==0:
    require(json.loads(r.stdout).get('nano.bootstrap.cas.owner')==token,'owned parent cleanup');rr=subprocess.run(docker+['container','rm','--force',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20);require(rr.returncode==0,'parent cleanup removal')
   else:require(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'parent cleanup state')
   r=subprocess.run(inspect,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=env,timeout=20);require(r.returncode==1 and (b'No such object' in r.stdout or b'No such container' in r.stdout),'owned parent container remains')
 def worker(label,rows,seconds):
  save(receipts/(label+'-actual-argv.json'),rows);body='#!/bin/sh\nset -eu\n'
  for n,argv,limit in rows:body+='printf '+shlex.quote('%s\\n')+' '+shlex.quote(n)+'\n'+'/usr/bin/timeout --signal=TERM --kill-after=5s '+str(limit)+'s '+shlex.join(argv)+'\n'
  file=controls/(label+'.sh');file.write_text(body);workers[str(file)]=observed(file,arch);return container(label,['/bin/sh','/controls/'+file.name],seconds)
 try:
  image=json.loads(command('image-inspect',p['dockerArgv']+['image','inspect','--format','{{json .}}',p['image']],25));require(image['Id']==p['image'] and image['Architecture']==arch and image['Os']=='linux','immutable existing image');require(not any(v.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for v in image.get('Config',{}).get('Env',[])),'no private image environment')
  # Host parent source readback is protected too, before any parent probe or source overlay.
  for r in p['sourcePins'].values():pin(r)
  parentcc=json.loads(pin(p['parentCompileCommands']));parent_units={r['source']:unit(next(c for c in parentcc if c['file']=='/work/'+r['source'] and unit(c)[0]==r['object'])) for r in p['core']}
  parent_units.update({r['source']:unit(next(c for c in parentcc if c['file']=='/work/'+r['source'] and unit(c)[0]==r['object'])) for r in p['consumers']})
  proofdir=build/'parent-proof';proofdir.mkdir(mode=0o700);(proofdir/'tmp').mkdir(mode=0o700)
  parentbody='#!/bin/sh\nset -eu\n'
  for rel,(obj,argv) in sorted(parent_units.items()):
   (proofdir/obj).parent.mkdir(parents=True,exist_ok=True);probe=dependency_argv(argv,obj,True);probe[probe.index('-o')+1]='/proof/'+obj+'.d';parentbody+='/usr/bin/timeout --signal=TERM --kill-after=5s 60s '+shlex.join(probe)+'\n'
  parent_script=controls/'parent-current-dependencies.sh';parent_script.write_text(parentbody);workers[str(parent_script)]=observed(parent_script,arch)
  parent_container('parent-current-dependencies',['/bin/sh','/controls/'+parent_script.name],700)
  current_parent_graph={}
  for r in [*p['core'],*p['consumers']]:
   actual=dep(observed(proofdir/(r['object']+'.d'),arch,None,True)[0]);require(actual==r['dependencies'],'fresh parent -M differs from actual compiled MD ancestry');current_parent_graph[r['object']]={'actualCompilationMdOrigin':r['mdInput'],'freshProbe':observed(proofdir/(r['object']+'.d'),arch),'dependencies':actual,'freshProbeIsNotCompilationMd':True}
  save(receipts/'parent-current-dependency-closure.json',current_parent_graph)
  names=parent_container('parent-archive-members',['/usr/bin/ar','t','libnano_core.a'],60).decode().splitlines();require(len(names)==141 and len(set(names))==141 and set(names)=={r['member'] for r in parent_members},'actual parent ar-t141')
  parentbody='#!/bin/bash\nset -euo pipefail\n'
  for name in names:
   require(re.fullmatch('[A-Za-z0-9_.+-]+',name),'safe parent member name');parentbody+='printf '+shlex.quote(name+'\n')+'\n/usr/bin/ar p /outputs/libnano_core.a '+shlex.quote(name)+' | /usr/bin/sha256sum\n/usr/bin/ar p /outputs/libnano_core.a '+shlex.quote(name)+' | /usr/bin/wc -c\n'
  parent_script=controls/'parent-archive-bytes.sh';parent_script.write_text(parentbody);workers[str(parent_script)]=observed(parent_script,arch);output=parent_container('parent-archive-byte-comparison',['/bin/bash','/controls/'+parent_script.name],180).decode().splitlines();require(len(output)==423,'parent282 ar-p complete output')
  byname={r['member']:r for r in parent_members};parent_proof=[]
  for i in range(0,len(output),3):
   name=output[i];actual_hash=output[i+1].split()[0];actual_bytes=int(output[i+2]);expected=byname[name];require(actual_hash==expected['sha256'] and actual_bytes==expected['bytes'],'actual parent raw member/object bytes');parent_proof.append({'member':name,'object':expected['object'],'sha256':actual_hash,'bytes':actual_bytes})
  save(receipts/'parent-141-actual-byte-proof.json',{'archive':p['parentArchive'],'members':parent_proof,'parentObjectsAndSourcesNeverMountedWritable':True})
  for path,state in protected.items():same(observed(path,arch),state,'parent capture input readback')
  sourcepins={**p['sourcePins'],**p['overlays']}
  for rel,r in sourcepins.items():relative(rel);copy(r,source/rel);sourcebefore[rel]=observed(source/rel,arch)
  require(len(sourcebefore)==1317,'exact current parent1314 + inputheader/cpp/startuptest');save(receipts/'source-before.json',sourcebefore);save(receipts/'parents-before.json',protected)
  container('configure',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],120)
  cc=json.loads(observed(build/'compile_commands.json',arch,None,True)[0]);old=parentcc;key=lambda r:(r['file'],r.get('output'),r['directory']);before={key(r):r for r in old};after={key(r):r for r in cc};require(len(after)==len(before)+2 and all(after.get(k)==r for k,r in before.items()),'only new core and test argv added')
  core={r['source']:unit(next(c for c in cc if c['file']=='/work/'+r['source'] and 'nano_core.dir/' in c['command'])) for r in p['core']}
  core[p['newCore']]=unit(next(c for c in cc if c['file']=='/work/'+p['newCore'] and 'nano_core.dir/' in c['command']))
  consumers={c['target']:unit(next(r for r in cc if r['file']=='/work/'+c['source'] and unit(r)[0]==c['object'])) for c in p['consumers']}
  tiny=unit(next(c for c in cc if c['file']=='/work/'+p['newTestSource']));changed={k for k in p['overlays'] if k!='CMakeLists.txt'}
  selected={r['source'] for r in p['core'] if any(posixpath.normpath(d[6:]) in changed for d in r['dependencies'] if d.startswith('/work/'))};selected.add(p['newCore'])
  selectedcons={c['target'] for c in p['consumers'] if any(posixpath.normpath(d[6:]) in changed for d in c['dependencies'] if d.startswith('/work/'))}
  require(selectedcons=={'nano','nano-capella-bootstrap-archive-cas-tests'},'new main and pending source must compile');
  save(receipts/'selection.json',{'freshCore':sorted(selected),'freshConsumers':sorted(selectedcons),'newTest':p['newTestTarget'],'actualMdBased':True})
  for rel,r in p['libraries'].items():copy(r,build/relative(rel));copies[rel]=observed(build/rel,arch)
  for rel,r in p['generated'].items():
   require(observed(build/rel,arch)['sha256']==r['sha256'],'generated CMake bytes');copies[rel]=observed(build/rel,arch)
  units={s:core[s] for s in selected};units.update({c['source']:consumers[c['target']] for c in p['consumers'] if c['target'] in selectedcons});units[p['newTestSource']]=tiny
  proberows=[]
  for rel,(obj,argv) in sorted(units.items()):(build/'preflight'/(obj+'.d')).parent.mkdir(parents=True,exist_ok=True);proberows.append((rel,dependency_argv(argv,obj,True),60))
  worker('dependency-preflight',proberows,300);probes={obj:dep(observed(build/'preflight'/(obj+'.d'),arch,None,True)[0]) for obj,_ in units.values()}
  all_deps={d for r in p['core'] if r['source'] not in selected for d in r['dependencies']}|{d for c in p['consumers'] if c['target'] not in selectedcons for d in c['dependencies']}|{d for ds in probes.values() for d in ds};sdk=set(p['sdkHashes']);sdk.update(d for d in all_deps if d.startswith(('/usr/','/lib/')));sdk.update(['/usr/bin/clang++','/usr/bin/ar','/usr/bin/ranlib'])
  for d in all_deps:
   if d.startswith('/work/'):rel=relative(posixpath.normpath(d[6:]));require(rel in sourcebefore and observed(source/rel,arch)['sha256']==sourcebefore[rel]['sha256'],'project dependency bytes')
   elif d.startswith('/outputs/'):rel=relative(posixpath.normpath(d[9:]));require(rel in copies,'presealed generated dependency')
  links={};meta=['compile_commands.json','CMakeFiles/nano_core.dir/flags.make','CMakeFiles/nano_core.dir/link.txt']
  for target in [*consumers,p['newTestTarget']]:
   for leaf in ('flags.make','link.txt'):meta.append('CMakeFiles/'+target+'.dir/'+leaf)
   text=observed(build/'CMakeFiles'/(target+'.dir')/'link.txt',arch,None,True)[0];argv=shlex.split(text.decode());require(argv[0]=='/usr/bin/clang++' and argv.count('libnano_core.a')==1 and argv[argv.index('-o')+1]==target,'exact linker argv');sdk.update(v for v in argv if v.startswith(('/usr/','/lib/')));links[target]=argv
   if target in consumers:
    old_consumer=next(c for c in p['consumers'] if c['target']==target)
    require(text==pin(old_consumer['linkInput']),'existing consumer linker bytes unchanged')
  metadata={r:observed(build/r,arch) for r in meta};require(metadata['CMakeFiles/nano_core.dir/flags.make']['sha256']==p['coreFlagsSha256'],'core compiler flags identical')
  sdkfile=controls/'sdk.sh';sdkfile.write_text('#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(v)+'\n' for v in sorted(sdk)));workers[str(sdkfile)]=observed(sdkfile,arch);sdkbefore=container('sdk-before',['/bin/sh','/controls/sdk.sh'],120);rows={n:h for h,n in (l.split('  ',1) for l in sdkbefore.decode().splitlines())};require(all(rows.get(n)==h for n,h in p['sdkHashes'].items()),'qualified SDK bytes')
  compilerows=[]
  for r in p['core']:
   obj,argv=core[r['source']];(build/obj).parent.mkdir(parents=True,exist_ok=True)
   if r['source'] not in selected:copy(r['objectInput'],build/obj);counts['reusedCore']+=1
  existing_consumer_sources={c['source'] for c in p['consumers']}
  for rel,(obj,argv) in sorted(units.items()):
   (build/obj).parent.mkdir(parents=True,exist_ok=True)
   limit=600 if rel in existing_consumer_sources else 180 if rel==p['newTestSource'] else 120
   compilerows.append((rel,dependency_argv(argv,obj,False),limit))
  worker('compile-selected',compilerows,1800)
  for rel,(obj,argv) in units.items():require(dep(observed(build/(obj+'.d'),arch,None,True)[0])==probes[obj],'actual MD equals preflight')
  counts['freshCore']=len(selected)-1;counts['newCore']=1;counts['freshConsumers']=len(selectedcons);counts['newTest']=1
  for c in p['consumers']:
   if c['target'] not in selectedcons:copy(c['objectInput'],build/c['object']);counts['reusedConsumers']+=1
  arlines=observed(build/'CMakeFiles/nano_core.dir/link.txt',arch,None,True)[0].decode().splitlines();require(len(arlines)==2,'archive two commands');ar,ran=[shlex.split(v) for v in arlines];require(ar[:3]==['/usr/bin/ar','qc','libnano_core.a'] and len(ar[3:])==142 and set(ar[3:])=={v[0] for v in core.values()} and ran==['/usr/bin/ranlib','libnano_core.a'],'exact142 result archive operands');worker('archive',[('ar',ar,120),('ranlib',ran,60)],200)
  archivepin=observed(build/'libnano_core.a',arch)
  names=container('archive-members',['/usr/bin/ar','t','libnano_core.a'],60).decode().splitlines();objects={v[0]:observed(build/v[0],arch) for v in core.values()};byname={pathlib.Path(obj).name:(obj,s) for obj,s in objects.items()};require(len(names)==142 and len(set(names))==142 and set(names)==set(byname),'actual142 unique members')
  members=controls/'members.sh';body='#!/bin/sh\nset -eu\n'
  for n in names:require(re.fullmatch('[A-Za-z0-9_.+-]+',n),'member name');body+='/usr/bin/ar p /outputs/libnano_core.a '+shlex.quote(n)+' > /outputs/tmp/member\nprintf '+shlex.quote(n+' ')+'\n/usr/bin/sha256sum /outputs/tmp/member\n/usr/bin/wc -c < /outputs/tmp/member\n'
  members.write_text(body);workers[str(members)]=observed(members,arch);rawmembers=container('archive-member-bytes',['/bin/sh','/controls/members.sh'],120);proof=[]
  memberlines=rawmembers.decode().splitlines();require(len(memberlines)==284,'142 hash/length result rows');
  for i in range(0,len(memberlines),2):n,h,_=memberlines[i].split(' ',2);obj,s=byname[n];require(h==s['sha256'] and int(memberlines[i+1])==s['st_size'],'archive raw member bytes and length match actual object');proof.append({'member':n,'object':obj,'sha256':h,'bytes':s['st_size']})
  require(len(proof)==142,'all142 result raw members checked');same(observed(build/'libnano_core.a',arch),archivepin,'archive after member proof');save(receipts/'archive-142-raw-member-proof.json',{'archiveBeforeProof':archivepin,'members':proof})
  consumer_objects={obj:observed(build/obj,arch) for obj,_ in [*consumers.values(),tiny]}
  save(receipts/'consumer-objects-before-links.json',consumer_objects)
  worker('link-selected',[(target,argv,120) for target,argv in links.items()],400);counts['links']=len(links);programs={t:observed(build/t,arch) for t in links};same(observed(build/'libnano_core.a',arch),archivepin,'archive after links')
  sdkafter=container('sdk-after',['/bin/sh','/controls/sdk.sh'],120);require(sdkbefore==sdkafter,'SDK byte equality');save(receipts/'sdk-binding.json',{'beforeSha256':digest(sdkbefore),'afterSha256':digest(sdkafter),'paths':len(sdk),'equal':True})
  for path,s in protected.items():same(observed(path,arch),s,'protected parent')
  for path,s in workers.items():same(observed(path,arch),s,'public worker')
  for path,s in sourcebefore.items():same(observed(source/path,arch),s,'source')
  for path,s in {**copies,**objects,**consumer_objects,**metadata}.items():same(observed(build/path,arch),s,'build input')
  for target,s in programs.items():same(observed(build/target,arch),s,'linked binary')
  same(observed(build/'libnano_core.a',arch),archivepin,'archive');same(observed(selfpath,arch),selfseal,'controller');require(observed(planpath,arch,16*1024*1024,True)[0]==raw,'plan stable');save(receipts/'parents-after.json',protected);save(receipts/'ctime-diagnostics.json',diagnostics)
  result={'format':'nano-bootstrap-owner-runtime-incremental-build-terminal-v1','status':'BUILD_PASS_TESTS_NOT_EXECUTED','arch':arch,'baselineGitCommit':BASE,'controllerSha256':a.controller_sha,'planSha256':a.plan_sha,'counts':counts,'sourceFiles':len(sourcebefore),'archive':archivepin,'archiveMembers':142,'coreObjects':objects,'consumerObjects':consumer_objects,'programs':programs,'allActualMdEqualPreflight':True,'publicParentsAndSdkUnchanged':True,'imageBuiltOrPulled':False,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
 except BaseException as error:
  failure=type(error).__name__+': '+str(error);result={'format':'nano-bootstrap-owner-runtime-incremental-build-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','arch':arch,'failure':failure,'completedCounters':counts,'testsExecuted':0,'privateInputsRead':False,'runtimeStarted':False,'productionQualified':False}
  try:
   for path,s in protected.items():same(observed(path,arch),s,'failure parent readback')
   result['publicParentsUnchangedAfterFailure']=True
  except BaseException:result['publicParentsUnchangedAfterFailure']=False
 save(receipts/'terminal-build.json',result);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
