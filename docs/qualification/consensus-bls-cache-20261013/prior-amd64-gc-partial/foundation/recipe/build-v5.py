#!/usr/bin/env python3
"""ROOT-dispatched build only, on the VPS host. No SSH, tests or campaign inputs.
Sources must first have arrived through reviewed Git push/pull. Python is used
on the host only; the pinned existing image runs compiler/CMake/shell tools.
"""
import argparse,hashlib,io,json,os,pathlib,re,shlex,shutil,stat,subprocess,sys,tarfile,time,uuid
IMAGE='sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
INPUT_SHA='ac1dc45ceaadac8f6ff42f78bb5b80e6ea2fd71a6fbbb6b435a9a2cf51437da3'
def require(ok,why):
 if not ok:raise ValueError(why)
def sha(data):return hashlib.sha256(data).hexdigest()
def canonical(p):
 p=pathlib.Path(p);require(p.is_absolute() and str(p)==str(p.resolve()),'canonical absolute path required');return p
def bounded(p,limit=16*1024*1024):
 p=canonical(p);s=p.lstat();require(stat.S_ISREG(s.st_mode),'regular public artifact required')
 with p.open('rb') as f:b=f.read(limit+1)
 require(len(b)<=limit,'public artifact bound');return b
def pinned(p,h,limit=16*1024*1024):
 require(isinstance(h,str) and re.fullmatch('[0-9a-f]{64}',h),'external SHA required');b=bounded(p,limit);require(sha(b)==h,'public pin mismatch '+str(p));return b
def hashstat(p):
 p=canonical(p);a=p.stat();h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 z=p.stat();fields=['st_dev','st_ino','st_uid','st_gid','st_mode','st_size','st_mtime_ns','st_ctime_ns'];require(all(getattr(a,k)==getattr(z,k) for k in fields),'artifact changed during hash')
 return {'sha256':h.hexdigest(),**{k:getattr(z,k) for k in fields}}
def save(p,value):
 b=(json.dumps(value,indent=2)+'\n').encode();fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
 with os.fdopen(fd,'wb') as f:f.write(b);f.flush();os.fsync(f.fileno())
 return sha(b)
def object_path(command):
 args=shlex.split(command['command']);require(args[0]=='/usr/bin/clang++' and '-c' in args and '-o' in args,'exact C++ compiler command required');rel=args[args.index('-o')+1];require(not pathlib.PurePosixPath(rel).is_absolute() and '..' not in pathlib.PurePosixPath(rel).parts,'owned object path');return rel,args


def dependency_set(path):
 data=bounded(path,4*1024*1024).decode('utf-8','strict');require(':' in data,'raw dependency target missing')
 values=shlex.split(data.replace('\\\n',' ').split(':',1)[1]);require(values and all(x.startswith(('/work/','/outputs/','/usr/','/lib/')) and '\n' not in x for x in values),'compiler dependency scope')
 return sorted(set(values))
def dependency_command(argv,obj,probe=False):
 args=[];n=0
 while n<len(argv):
  value=argv[n]
  if value in ['-MF','-MT','-MQ']:require(n+1<len(argv),'dependency option missing argument');n+=2;continue
  if value in ['-MD','-MMD','-MP']:n+=1;continue
  args.append(value);n+=1
 if probe:
  at=args.index('-o');args[at+1]='/outputs/preflight-dependencies/'+obj+'.d';args[args.index('-c')]='-M';args+=['-MT',obj]
 else:args+=['-MD','-MF',obj+'.d','-MT',obj]
 return args

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for k in ['controller-sha','inputs','git-commit','repository','parent-build','output-root']:p.add_argument('--'+k,required=True)
 a=p.parse_args();self_path=canonical(__file__);pinned(self_path,a.controller_sha,1024*1024);i=json.loads(pinned(a.inputs,INPUT_SHA));require(i['format']=='nano-gc-amd64-public-incremental-inputs-v2','specific public input schema')
 require(re.fullmatch('[0-9a-f]{40}',a.git_commit) and a.git_commit==i['deliveredGitCommit'],'reviewed delivered Git commit required');repo=canonical(a.repository);parent=canonical(a.parent_build);root=canonical(a.output_root);require(os.geteuid()==1000 and os.getegid()==1000,'VPS user1000 required');require(not root.exists(),'fresh output root required')
 env={k:v for k,v in os.environ.items() if not k.startswith(('NANO_','LD_','DYLD_'))};env['LC_ALL']='C'
 def git(args):
  r=subprocess.run(['/usr/bin/git','-C',str(repo)]+args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60,env=env);require(r.returncode==0,'Git public source command failed');return r.stdout
 require(git(['rev-parse','HEAD']).decode().strip()==a.git_commit,'Git pull must match expected delivered commit');git(['diff-index','--quiet','HEAD','--'])
 os.mkdir(root,0o700);source=root/'source';build=root/'build';receipts=root/'receipts';controls=root/'controls'
 for d in [source,build,receipts,controls]:d.mkdir(mode=0o700)
 (build/'tmp').mkdir(mode=0o700);(receipts/'tmp').mkdir(mode=0o700);started=time.monotonic();actions=0;owned=None;dependency_copies_before={};parent_metadata_before={};probe_sets={};all_sdk=[]
 def host(label,argv,timeout=60):
  nonlocal actions
  require(time.monotonic()-started<1800,'total build wall budget exhausted');actions+=1;save(receipts/(label+'.command.json'),{'argv':argv,'timeoutSeconds':timeout});begin=time.monotonic()
  try:
   with (receipts/(label+'.log')).open('wb',buffering=0) as log:r=subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,timeout=min(timeout,max(1,1800-(time.monotonic()-started))),env=env)
  except subprocess.TimeoutExpired:
   save(receipts/(label+'.exit.json'),{'exitCode':None,'elapsedSeconds':time.monotonic()-begin,'processCompleted':False,'timedOut':True});raise
  save(receipts/(label+'.exit.json'),{'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-begin,'processCompleted':True,'timedOut':False});require(r.returncode==0,'actual command failed: '+label);print(json.dumps({'phase':label,'elapsedSeconds':time.monotonic()-started}),flush=True);return (receipts/(label+'.log')).read_bytes()
 def container(label,argv,timeout=300):
  nonlocal owned
  token=uuid.uuid4().hex;name='nano-gc-amd-'+token[:16];owned=(name,token)
  args=['/usr/bin/sudo','-n','/usr/bin/docker','run','--rm','--pull=never','--platform','linux/amd64','--name',name,'--label','nano.gc.build.owner='+token,'--network','none','--read-only','--user','1000:1000','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','4g','--pids-limit','256','--env','TMPDIR=/outputs/tmp','--mount','type=bind,source='+str(source)+',target=/work,readonly','--mount','type=bind,source='+str(parent)+',target=/parents,readonly','--mount','type=bind,source='+str(build)+',target=/outputs','--mount','type=bind,source='+str(receipts)+',target=/receipts','--mount','type=bind,source='+str(controls)+',target=/controls,readonly','--workdir','/outputs','--entrypoint',argv[0],IMAGE]+argv[1:]
  try:return host(label,args,timeout)
  finally:
   q=subprocess.run(['/usr/bin/sudo','-n','/usr/bin/docker','container','inspect','--format','{{json .Config.Labels}}',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20,env=env)
   if q.returncode==0:
    labels=json.loads(q.stdout);require(labels.get('nano.gc.build.owner')==token,'refuse unowned cleanup');args=['/usr/bin/sudo','-n','/usr/bin/docker','container','rm','--force',name];save(receipts/(label+'-cleanup.command.json'),{'argv':args,'timeoutSeconds':20,'outsideExpiredBuildBudget':True});removed=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20,env=env);(receipts/(label+'-cleanup.log')).write_bytes(removed.stdout);save(receipts/(label+'-cleanup.exit.json'),{'exitCode':removed.returncode});require(removed.returncode==0,'owned cleanup failed')
   else:require(q.returncode==1 and (b'No such object' in q.stdout or b'No such container' in q.stdout),'owned container terminal absence ambiguous')
   q=subprocess.run(['/usr/bin/sudo','-n','/usr/bin/docker','container','inspect',name],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20,env=env);require(q.returncode==1 and (b'No such object' in q.stdout or b'No such container' in q.stdout),'owned container not absent');owned=None
 failure=None;parent_before={};source_before={};terminal=None
 try:
  image=json.loads(host('image-inspect',['/usr/bin/sudo','-n','/usr/bin/docker','image','inspect','--format','{{json .}}',IMAGE]));require(image['Id']==IMAGE and image['Architecture']=='amd64' and image['Os']=='linux','existing pinned image platform');require(not any(x.split('=',1)[0].startswith(('NANO_','LD_','DYLD_')) for x in image.get('Config',{}).get('Env',[])),'injected image runtime environment')
  expected={r['path']:r['sha256'] for r in i['consolidatedSources']};expected[i['portableAvailabilitySource']['path']]=i['portableAvailabilitySource']['sha256']
  extra=i['additionalPublicSourceInputs'];require(len(extra)==1 and extra[0]['path']=='tools/storage-maintenance/rocksdb-owned-copy.hpp' and extra[0]['gitCommit']==a.git_commit and extra[0]['historicallyIncludedInParentSourceMap'] is False,'exact current public helper header required')
  for entry in extra:require(entry['path'] not in expected and entry['path'] not in i['parentSourceHashes'],'additional header must not overwrite historical/source pins');expected[entry['path']]=entry['sha256']
  save(receipts/'additional-current-public-source-pins.json',{'files':extra,'historicalParentSourceMapUnchanged':True})
  paths=sorted(set(i['parentSourceHashes'])|set(expected));require(len(paths)<=4096 and all(not x.startswith('/') and '..' not in pathlib.PurePosixPath(x).parts and '\n' not in x and not x.endswith('.env') for x in paths),'public source allowlist')
  for n in range(0,len(paths),32):
   raw=git(['archive','--format=tar',a.git_commit,'--']+paths[n:n+32]);require(len(raw)<=64*1024*1024,'bounded public Git archive chunk')
   with tarfile.open(fileobj=io.BytesIO(raw),mode='r:') as t:
    for m in t:
     if m.isdir():continue
     require(m.isfile() and m.name in paths and m.size<=16*1024*1024,'only bounded public regular source files');dest=source/m.name;require(not dest.exists(),'duplicate archived source');dest.parent.mkdir(parents=True,exist_ok=True);b=t.extractfile(m).read();require(len(b)==m.size,'Git source size');dest.write_bytes(b);source_before[m.name]=sha(b)
  require(set(source_before)==set(paths),'all public source inputs from exact Git archive required')
  for rel,h in expected.items():require(source_before[rel]==h,'delivered consolidated/portable source mismatch: '+rel)
  project_aliases={}
  for r in i['core']:
   q=parent/r['object'];s=hashstat(q);require(s['sha256']==r['sha256'] and s['st_size']==r['bytes'],'actual parent139 object changed');parent_before[r['object']]=s
   for raw_rel in r['repositoryDependencies']:
    require(isinstance(raw_rel,str) and raw_rel and not raw_rel.startswith('/') and '\n' not in raw_rel,'relative public project dependency required')
    rel=os.path.normpath(raw_rel)
    require(not pathlib.PurePosixPath(rel).is_absolute() and '..' not in pathlib.PurePosixPath(rel).parts and (source/rel).resolve()==source/rel,'normalized project dependency must remain within owned source tree')
    if raw_rel!=rel:project_aliases[raw_rel]=rel
    require(rel in source_before,'compiler dependency absent from Git archive: '+rel);wanted=expected.get(rel,i['parentSourceHashes'].get(rel));require(wanted is not None and source_before[rel]==wanted,'unexpected compiler dependency change: '+rel)
  save(receipts/'relative-project-dependency-aliases.json',{'rawToCanonical':project_aliases,'normalizedProjectPathsOnly':True,'canonicalContentPinsRequired':True,'systemAndGeneratedSdkNamesUnchanged':True})
  for rel,h in [('libnano_core.a',i['parentArchiveSha256']),('nano',i['parentNanoSha256']),('libnano-test-sync-fault.so',i['faultSha256'])]:
   s=hashstat(parent/rel);require(s['sha256']==h,'qualified parent program/archive mismatch');parent_before[rel]=s
  save(receipts/'source-before.json',{'commit':a.git_commit,'sourceHashes':source_before,'publicAllowlistOnly':True});save(receipts/'parent-before.json',parent_before)
  container('configure',['/usr/bin/cmake','-S','/work','-B','/outputs','-G','Unix Makefiles','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DMCL_PORTABLE=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],180)
  cc=bounded(build/'compile_commands.json');save(receipts/'original-compile-commands-binding.json',{'sha256':sha(cc),'bytes':len(cc),'capturedBeforeCompilation':True});(receipts/'original-compile_commands.json').write_bytes(cc);commands=json.loads(cc);newcore={};consumers={}
  for row in commands:
   obj,argv=object_path(row) if row['file'].endswith('.cpp') and row['command'].startswith('/usr/bin/clang++ ') else (None,None)
   if obj and obj.startswith('CMakeFiles/nano_core.dir/'):
    require(row['file'].startswith('/work/'),'core source mount');newcore[row['file'][6:]]=(obj,argv)
   for target,rel in i['consumers'].items():
    if obj and obj=='CMakeFiles/'+target+'.dir/'+rel+'.o':require(target not in consumers,'duplicate consumer');consumers[target]=(obj,argv)
  require(set(newcore)=={r['source'] for r in i['core']}|{i['newCoreSource']} and len(newcore)==140 and set(consumers)==set(i['consumers']),'exact140core/eightconsumers graph')
  oldcommands={r['file']:object_path(r)[1] for r in i['parentCompileCommands'] if r['file'].startswith('/work/src/') and 'CMakeFiles/nano_core.dir/' in r['command']}
  for r in i['core']:
   if not r['fresh']:require(newcore[r['source']][1]==oldcommands['/work/'+r['source']],'reused compiler argv changed')
  # Copy current dependency artifacts from the immutable parent, with no build.
  links={target:bounded(build/'CMakeFiles'/ (target+'.dir')/'link.txt',1024*1024).decode() for target in consumers};library_paths=set()
  for link in links.values():
   for token in shlex.split(link):
    if not token.startswith('/') and not token.startswith('CMakeFiles/') and token!='libnano_core.a' and (token.endswith(('.a','.so','.o'))):library_paths.add(token)
  library_paths.add('libnano-test-sync-fault.so')
  for rel in sorted(library_paths):
   require('..' not in pathlib.PurePosixPath(rel).parts,'owned dependency path');s=hashstat(parent/rel);pin=i['expectedDependencyArtifacts'].get(rel);require(pin is not None and s['sha256']==pin['sha256'] and s['st_size']==pin['bytes'],'historical dependency artifact differs: '+rel);parent_before[rel]=s;dest=build/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(parent/rel,dest);copy=hashstat(dest);require(copy['sha256']==pin['sha256'] and copy['st_size']==pin['bytes'],'dependency byte copy');dependency_copies_before[rel]=copy
  # Snapshot parent generated metadata and any /outputs dependencies consumed
  # by old objects. The old standalone system-SDK snapshot remains absent.
  parent_metadata_paths={'compile_commands.json','CMakeFiles/nano_core.dir/flags.make','CMakeFiles/nano_core.dir/link.txt'}
  for target in i['consumers']:
   for leaf in ['flags.make','link.txt']:
    rel='CMakeFiles/'+target+'.dir/'+leaf
    if (parent/rel).exists():parent_metadata_paths.add(rel)
  for r in i['core']:
   parent_metadata_paths.add(r['object']+'.d')
   parent_metadata_paths.update(x[len('/outputs/'):] for x in r['dependencies'] if x.startswith('/outputs/'))
  for rel in sorted(parent_metadata_paths):parent_metadata_before[rel]=hashstat(parent/rel)
  save(receipts/'parent-generated-metadata-before.json',parent_metadata_before)
  probe_units={r['source']:newcore[r['source']] for r in i['core'] if r['fresh']};probe_units[i['newCoreSource']]=newcore[i['newCoreSource']]
  probe_units.update({i['consumers'][target]:pair for target,pair in consumers.items()});require(len(probe_units)==49,'actual41core/eightconsumer dependency-only probes')
  for rel,(obj,argv) in probe_units.items():
   path=build/'preflight-dependencies'/(obj+'.d');path.parent.mkdir(parents=True,exist_ok=True);container('probe-'+pathlib.Path(rel).stem,dependency_command(argv,obj,True),120);dependencies=dependency_set(path);probe_sets[obj]=dependencies
   for dependency in dependencies:
    if dependency.startswith('/work/'):
     raw_name=dependency[len('/work/'):]
     require(raw_name and not raw_name.startswith('/') and '\n' not in raw_name,'relative fresh public project dependency required')
     name=os.path.normpath(raw_name)
     require(not pathlib.PurePosixPath(name).is_absolute() and '..' not in pathlib.PurePosixPath(name).parts and (source/name).resolve()==source/name,'normalized fresh project dependency must remain within owned source tree')
     if raw_name!=name:project_aliases[raw_name]=name
     require(name in source_before,'fresh compiler project dependency not archived: '+name);wanted=expected.get(name,i['parentSourceHashes'].get(name));require(wanted is not None and source_before[name]==wanted,'unexpected fresh project dependency change: '+name)
  save(receipts/'fresh-dependency-only-probes.json',{'probeCount':49,'codeGenerationExecuted':False,'sets':probe_sets,'rawFiles':{obj:hashstat(build/'preflight-dependencies'/(obj+'.d')) for obj in probe_sets}})
  all_dependencies={x for r in i['core'] if not r['fresh'] for x in r['dependencies']};all_dependencies.update(x for rows in probe_sets.values() for x in rows)
  save(receipts/'old-and-fresh-project-dependency-aliases.json',{'rawToCanonical':project_aliases,'normalizedProjectPathsOnly':True,'canonicalContentPinsRequired':True,'systemAndGeneratedSdkNamesUnchanged':True,'rawMAndMdDependencySetsUnchanged':True})
  sdk=sorted(x for x in all_dependencies if not x.startswith('/work/'));all_sdk=sdk;require(all(x.startswith(('/usr/','/lib/','/outputs/')) and '\n' not in x for x in sdk),'complete fresh+reuse SDK dependency scope')
  for path in sdk:
   if path.startswith('/outputs/'):
    rel=path[len('/outputs/'):];dest=build/rel
    reused=any(path in r['dependencies'] for r in i['core'] if not r['fresh'])
    if reused:
     original=hashstat(parent/rel);parent_before[rel]=original
     if not dest.exists():dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(parent/rel,dest)
     require(hashstat(dest)['sha256']==original['sha256'],'reused generated SDK dependency differs')
    else:require(dest.is_file(),'fresh generated SDK dependency absent')
  sdk_script='#!/bin/sh\nset -eu\n'+''.join('/usr/bin/sha256sum -- '+shlex.quote(x)+'\n' for x in sdk);(controls/'sdk-hashes.sh').write_text(sdk_script);sdk_before=container('sdk-before',['/bin/sh','/controls/sdk-hashes.sh'],180)
  reused=0;fresh=0
  for r in i['core']:
   obj,argv=newcore[r['source']];dest=build/obj;dest.parent.mkdir(parents=True,exist_ok=True)
   if r['fresh']:container('compile-core-'+pathlib.Path(r['source']).stem,dependency_command(argv,obj));require(dependency_set(build/(obj+'.d'))==probe_sets[obj],'fresh core actual compiled dependencies differ from preflight');fresh+=1
   else:shutil.copyfile(parent/r['object'],dest);require(hashstat(dest)['sha256']==r['sha256'],'reused object byte copy');reused+=1
  obj,argv=newcore[i['newCoreSource']];(build/obj).parent.mkdir(parents=True,exist_ok=True);container('compile-core-new-gc',dependency_command(argv,obj));require(dependency_set(build/(obj+'.d'))==probe_sets[obj],'new GC actual compiled dependencies differ from preflight');fresh+=1;require((fresh,reused)==(41,99),'exact incremental closure')
  link=bounded(build/'CMakeFiles/nano_core.dir/link.txt',1024*1024).decode().splitlines();require(len(link)==2,'exact archive+ranlib commands')
  ar=shlex.split(link[0]);require(ar[:3]==['/usr/bin/ar','qc','libnano_core.a'] and len(ar[3:])==140 and set(ar[3:])=={x[0] for x in newcore.values()},'exact140 archive inputs');container('archive',ar,120);ran=shlex.split(link[1]);require(ran==['/usr/bin/ranlib','libnano_core.a'],'exact archive index');container('ranlib',ran,60);member_names=container('archive-members',['/usr/bin/ar','t','libnano_core.a'],60).decode().splitlines();require(len(member_names)==140 and len(set(member_names))==140,'actual140member archive')
  archive_before=hashstat(build/'libnano_core.a');objects_before={obj:hashstat(build/obj) for obj,_ in newcore.values()};require({pathlib.Path(x).name for x in objects_before}==set(member_names),'exact object/member names')
  members_file=controls/'core-members.txt';members_file.write_text(''.join(x+'\n' for x in member_names));require(all(re.fullmatch('[A-Za-z0-9_.+-]+',x) for x in member_names),'public flat member names')
  member_script='''#!/bin/sh
set -eu
while IFS= read -r member; do
 /usr/bin/ar p /outputs/libnano_core.a "$member" > /outputs/tmp/gc-member-first.bin
 first=$(/usr/bin/sha256sum /outputs/tmp/gc-member-first.bin | cut -d ' ' -f1)
 firstsize=$(wc -c < /outputs/tmp/gc-member-first.bin)
 /usr/bin/ar p /outputs/libnano_core.a "$member" > /outputs/tmp/gc-member-second.bin
 second=$(/usr/bin/sha256sum /outputs/tmp/gc-member-second.bin | cut -d ' ' -f1)
 secondsize=$(wc -c < /outputs/tmp/gc-member-second.bin)
 printf '%s\t%s\t%s\t%s\t%s\n' "$member" "$first" "$firstsize" "$second" "$secondsize"
 rm -- /outputs/tmp/gc-member-first.bin /outputs/tmp/gc-member-second.bin
done < /controls/core-members.txt
'''
  (controls/'archive-member-proof.sh').write_text(member_script);raw=container('archive-member-raw-byte-proof',['/bin/sh','/controls/archive-member-proof.sh'],180).decode();proof=[];byname={pathlib.Path(obj).name:(obj,s) for obj,s in objects_before.items()}
  for line in raw.splitlines():
   name,first,nfirst,second,nsecond=line.split('\t');require(name in byname,'unknown archive member');obj,stats=byname[name];require(first==second==stats['sha256'] and int(nfirst)==int(nsecond)==stats['st_size'],'archive member actual bytes differ');proof.append({'name':name,'object':obj,'sha256':first,'bytes':int(nfirst),'twoIndependentArExtractionsEqual':True})
  require(len(proof)==140 and len({r['name'] for r in proof})==140 and hashstat(build/'libnano_core.a')==archive_before and {obj:hashstat(build/obj) for obj in objects_before}==objects_before,'archive/object/member proof changed');save(receipts/'archive-140-raw-member-proof.json',{'archive':archive_before,'members':proof,'arPInvocations':280,'rawMemberBytesRetained':False})
  for target,(obj,argv) in consumers.items():
   (build/obj).parent.mkdir(parents=True,exist_ok=True);container('compile-'+target,dependency_command(argv,obj));require(dependency_set(build/(obj+'.d'))==probe_sets[obj],'consumer actual compiled dependencies differ from preflight');lines=links[target].splitlines();require(len(lines)==1,'one consumer link command');args=shlex.split(lines[0]);require(args[0]=='/usr/bin/clang++' and 'libnano_core.a' in args,'consumer must link exact candidate core');container('link-'+target,args,120)
  sdk_after=container('sdk-after',['/bin/sh','/controls/sdk-hashes.sh'],180);require(sdk_after==sdk_before,'complete fresh+reuse SDK/generated input changed during compilation');save(receipts/'complete-sdk-before-after-binding.json',{'currentPaths':all_sdk,'currentObservedSdkCount':len(all_sdk),'beforeSha256':sha(sdk_before),'afterSha256':sha(sdk_after),'currentBeforeAfterEqual':True,'historicalStandaloneSdkSnapshotAvailable':False,'historicalSdkHashesInvented':False,'immutableOriginalCompilerImage':IMAGE,'noMountOverlayOnSystemSdk':True})
  require(hashstat(build/'libnano_core.a')==archive_before and {obj:hashstat(build/obj) for obj in objects_before}==objects_before,'proved archive/core objects changed after consumer links');save(receipts/'archive-core-objects-after-links.json',{'archive':hashstat(build/'libnano_core.a'),'objects':{obj:hashstat(build/obj) for obj in objects_before},'equalToMemberProofInputs':True}); require({rel:hashstat(build/rel) for rel in dependency_copies_before}==dependency_copies_before,'dependency copies changed after link');require({rel:hashstat(parent/rel) for rel in parent_metadata_before}==parent_metadata_before,'parent SDK/generated/flag metadata changed');save(receipts/'dependency-copies-before-after.json',{'before':dependency_copies_before,'after':{rel:hashstat(build/rel) for rel in dependency_copies_before},'historicalPins':i['expectedDependencyArtifacts'],'allEqual':True});save(receipts/'parent-generated-metadata-after.json',parent_metadata_before)
  require({r:hashstat(parent/r) for r in parent_before}==parent_before,'qualified parent inputs changed');require({rel:sha(bounded(source/rel)) for rel in source_before}==source_before,'public source changed');pinned(self_path,a.controller_sha,1024*1024);pinned(a.inputs,INPUT_SHA)
  save(receipts/'parent-after.json',parent_before);save(receipts/'source-after.json',{'commit':a.git_commit,'sourceHashes':source_before});archive=hashstat(build/'libnano_core.a');programs=[{'target':t,'object':hashstat(build/consumers[t][0]),'binary':hashstat(build/t),'coreArchiveSha256':archive['sha256']} for t in consumers]
  terminal={'format':'nano-gc-amd64-incremental-build-terminal-v1','status':'BUILD_PASS_TESTS_NOT_EXECUTED','gitCommit':a.git_commit,'controllerSha256':a.controller_sha,'publicInputsSha256':INPUT_SHA,'coreFresh':fresh,'coreReused':reused,'archiveMembers':140,'freshConsumers':8,'archive':archive,'programs':programs,'image':IMAGE,'parentSourceLibraryImageInputsUnchanged':True,'sameImageSdkBeforeAfterObserved':True,'oldSdkHashesRetroactivelyInvented':False,'dependencyOnlyProbes':49,'actualCompiledDependenciesEqualPreflight':True,'archiveRawBytesComparedTo140Objects':True,'historicallyPinnedDependencyCopiesBeforeAfterEqual':True,'parentGeneratedMetadataBeforeAfterEqual':True,'testsExecuted':0,'runtimeStarted':False,'privateInputsRead':False,'imageBuiltOrPulled':False,'productionQualified':False}
 except BaseException as error:failure=type(error).__name__+': '+str(error);terminal={'format':'nano-gc-amd64-incremental-build-terminal-v1','status':'BUILD_FAILED_NO_AUTOMATIC_RETRY','failure':failure,'gitCommit':a.git_commit,'controllerSha256':a.controller_sha,'privateInputsRead':False,'testsExecuted':0,'runtimeStarted':False,'productionQualified':False}
 if failure:
  try:
   unchanged=all(hashstat(parent/rel)==value for rel,value in parent_before.items()) and all(sha(bounded(source/rel))==h for rel,h in source_before.items());terminal['observedParentAndPublicSourceUnchangedAfterFailure']=unchanged;terminal['observedCopiedDependenciesUnchangedAfterFailure']=all(hashstat(build/rel)==value for rel,value in dependency_copies_before.items());terminal['observedParentGeneratedMetadataUnchangedAfterFailure']=all(hashstat(parent/rel)==value for rel,value in parent_metadata_before.items())
  except BaseException as error:terminal['observedParentAndPublicSourceUnchangedAfterFailure']=False;terminal['failureBindingObservation']=str(error)
 save(receipts/'terminal-build-receipt.json',terminal);return 1 if failure else 0
if __name__=='__main__':sys.exit(main())
