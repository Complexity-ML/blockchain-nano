#!/usr/bin/env python3
"""ROOT host-only public provenance capture and plan; no Docker or compiler here.
Every CLI --*-sha is supplied literally from existing qualification by ROOT.
Missing actual freshly compiled MD fails; inherited MD stays explicitly ancestral.
"""
import argparse, hashlib, json, os, pathlib, posixpath, shlex, stat
BASE='6a4ef3d9be2f5649129415f1bc158e2a211ed4f6'
IMAGES={'arm64':'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','amd64':'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'}
def need(ok,why):
 if not ok:raise ValueError(why)
def path(p):
 p=pathlib.Path(p);need(p.is_absolute() and p.resolve()==p and not p.is_symlink(),'canonical public path');return p
def overlaps(a,b):return a==b or a in b.parents or b in a.parents
def rel(p):need(p and not p.startswith('/') and '..' not in pathlib.PurePosixPath(p).parts and '\n' not in p and '\0' not in p,'safe source path');return p
def observe(p,retain=False):
 p=path(p);fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW);h=hashlib.sha256();chunks=[]
 try:
  before=os.fstat(fd);need(stat.S_ISREG(before.st_mode),'regular public artifact')
  while True:
   b=os.read(fd,1048576)
   if not b:break
   h.update(b)
   if retain:chunks.append(b)
  after=os.fstat(fd);current=p.lstat();fields=('st_dev','st_ino','st_uid','st_gid','st_mode','st_size','st_mtime_ns')
  need(all(getattr(before,k)==getattr(after,k)==getattr(current,k) for k in fields),'artifact changed during read')
  seal={'path':str(p),'sha256':h.hexdigest(),'bytes':after.st_size,'stat':{k:getattr(after,k) for k in (*fields,'st_ctime_ns')}}
  return (b''.join(chunks),seal) if retain else seal
 finally:os.close(fd)
def load(p,expected):
 b,s=observe(p,True);need(s['sha256']==expected,'external public input SHA');return json.loads(b),s

def md(p):
 b,s=observe(p,True);text=b.decode();need(':' in text,'actual compiler MD target');deps=sorted(set(shlex.split(text.replace('\\\n',' ').split(':',1)[1])));need(deps and all(x.startswith(('/work/','/outputs/','/usr/','/lib/')) for x in deps),'actual MD namespace');return deps,s

def argv(row):
 a=shlex.split(row['command']);need(row['directory']=='/outputs' and a[0]=='/usr/bin/clang++' and a.count('-o')==a.count('-c')==1,'actual current parent compiler argv');return a[a.index('-o')+1],a

def main():
 ap=argparse.ArgumentParser()
 for n in ('arch','parent-build','parent-source','parent-plan','parent-plan-sha','parent-terminal','parent-terminal-sha','parent-source-map','parent-source-map-sha','parent-raw-proof','parent-raw-proof-sha','parent-sdk-before','parent-sdk-before-sha','parent-sdk-after','parent-sdk-after-sha','parent-compile-commands','parent-compile-commands-sha','candidate-manifest','candidate-sha','output-root','control-root'):ap.add_argument('--'+n,required=True)
 a=ap.parse_args();need(a.arch in IMAGES,'arch');B=path(a.parent_build);S=path(a.parent_source);D=path(a.control_root);O=path(a.output_root);C=path(a.candidate_manifest).parent;need(not D.exists() and not O.exists(),'fresh control/build outputs');need(not any(overlaps(x,y) for x in (D,O) for y in (B.parent,B,S,C)) and not overlaps(D,O),'output/control overlaps protected parent/source/candidate tree')
 old,oldseal=load(a.parent_plan,a.parent_plan_sha);terminal,tseal=load(a.parent_terminal,a.parent_terminal_sha);inventory,iseal=load(a.parent_source_map,a.parent_source_map_sha);proof,pseal=load(a.parent_raw_proof,a.parent_raw_proof_sha);cc,ccseal=load(a.parent_compile_commands,a.parent_compile_commands_sha);candidate,cseal=load(a.candidate_manifest,a.candidate_sha)
 need(terminal['status']=='BUILD_PASS_TESTS_NOT_EXECUTED' and terminal['arch']==a.arch and terminal['archiveMembers']==141 and len(terminal['coreObjects'])==141,'actual completed141 qualification, not failure/prepared plan')
 need(len(old['core'])==140 and old['newCore']=='src/consensus_capella_bootstrap_archive.cpp' and old['newTestTarget']=='nano-capella-bootstrap-archive-cas-tests','explicit qualified old141 recipe:140 prior +1 actual new, not generic count relaxation')
 need(candidate['baselineRevision']==BASE and candidate['overlayCount']==11 and candidate['status'].startswith('FROZEN'),'frozen combined11-path source candidate')
 members=proof['members'] if isinstance(proof,dict) else proof;need(len(members)==141,'actual raw141 proof');membermap={r['object']:r for r in members};need(len(membermap)==141 and len({r['member'] for r in members})==141,'unique141 member identities')
 archive=observe(B/'libnano_core.a');need(archive['sha256']==terminal['archive']['sha256'] and archive['bytes']==terminal['archive']['st_size'],'actual parent archive matches final qualification')
 sourcepins={}
 for n,r in inventory.items():
  pin=observe(S/rel(n));need(pin['sha256']==r['sha256'] and pin['bytes']==r['st_size'],'current actual qualified source pin');sourcepins[n]=pin
 need(len(sourcepins)==1314,'actual parent1314 current source files')
 parent_changed=set(old['overlays'])-{'CMakeLists.txt'}
 actual_fresh={r['source'] for r in old['core'] if any(posixpath.normpath(d[6:]) in parent_changed for d in r['dependencies'] if d.startswith('/work/'))}
 need(len(actual_fresh)==terminal['counts']['freshCore']==16 and terminal['counts']['newCore']==1,'derive17 actual fresh parent units from recorded old MD/old overlays and final terminal')
 ancestry_refs=[oldseal,tseal,iseal,pseal,ccseal];rows=[]
 for r in old['core']:
  obj=r['object'];current=observe(B/obj);need(current['sha256']==membermap[obj]['sha256']==terminal['coreObjects'][obj]['sha256'] and current['bytes']==membermap[obj]['bytes'],'current parent object/raw member/terminal binding')
  if r['source'] in actual_fresh:
   depfile=B/(obj+'.d');need(depfile.is_file(),'MISSING_ACTUAL_FRESH_PARENT_MD '+str(depfile));ds,dp=md(depfile);origin='actual141-parent-compilation'
  else:
   # This is the exact historically compiled MD, not a synthetic/current-parent MD.
   need(current['sha256']==r['objectInput']['sha256'],'reused124 object actual old identity');depfile=path(r['mdInput']['path']);ds,dp=md(depfile);need(dp['sha256']==r['mdInput']['sha256'] and dp['bytes']==r['mdInput']['bytes'] and ds==r['dependencies'],'exact124 inherited compiler MD');origin='qualified-inherited-compilation-md'
   for d in ds:
    if d.startswith('/work/'):
     name=rel(posixpath.normpath(d[6:]));need(name in old['sourcePins'] and sourcepins[name]['sha256']==old['sourcePins'][name]['sha256'],'reused object historical/current project dependency equality')
  rowcc=next(c for c in cc if c['file']=='/work/'+r['source'] and argv(c)[0]==obj);rows.append({'source':r['source'],'object':obj,'objectInput':current,'mdInput':dp,'dependencies':ds,'mdProvenance':origin,'mdPhysicallyCurrent':origin=='actual141-parent-compilation','parentCompilerCommand':rowcc})
 new=old['newCore'];obj='CMakeFiles/nano_core.dir/'+new+'.o';current=observe(B/obj);need(current['sha256']==membermap[obj]['sha256']==terminal['coreObjects'][obj]['sha256'],'actual bootstrap141st object');ds,dp=md(B/(obj+'.d'));rows.append({'source':new,'object':obj,'objectInput':current,'mdInput':dp,'dependencies':ds,'mdProvenance':'actual141-parent-compilation','mdPhysicallyCurrent':True,'parentCompilerCommand':next(c for c in cc if c['file']=='/work/'+new and argv(c)[0]==obj)})
 need(len(rows)==141 and {r['object'] for r in rows}==set(membermap),'exact141 current parent topology')
 consumers=[]
 for target,src in [('nano','src/main.cpp'),('nano-capella-bootstrap-archive-cas-tests','tests/test_capella_bootstrap_archive_cas.cpp')]:
  obj='CMakeFiles/'+target+'.dir/'+src+'.o';pin=observe(B/obj);need(pin['sha256']==terminal['consumerObjects'][obj]['sha256'],'actual consumer object');ds,dp=md(B/(obj+'.d'));consumers.append({'target':target,'source':src,'object':obj,'objectInput':pin,'mdInput':dp,'dependencies':ds,'mdProvenance':'actual141-parent-compilation','flagsInput':observe(B/'CMakeFiles'/(target+'.dir')/'flags.make'),'linkInput':observe(B/'CMakeFiles'/(target+'.dir')/'link.txt')})
 overlays={}
 for r in candidate['sourceFiles']:
  n=rel(r['path']);pin=observe(path(a.candidate_manifest).parent/r['sourcePath']);need(pin['sha256']==r['sha256'] and pin['bytes']==r['bytes'],'candidate source seal')
  if r['newFile']:need(n not in sourcepins,'new path must actually be new')
  else:need(sourcepins[n]['sha256']==r['baselineSha256'],'candidate modification matches actual qualified baseline source')
  overlays[n]=pin
 need(set(overlays)=={'CMakeLists.txt','include/nano/chain.hpp','include/nano/storage.hpp','src/chain.cpp','src/storage.cpp','include/nano/consensus_capella_bootstrap_input.hpp','src/consensus_capella_bootstrap_input.cpp','src/consensus_capella_runtime.cpp','src/main.cpp','tests/test_capella_bootstrap_archive_cas.cpp','tests/test_capella_bootstrap_startup.cpp'},'exact11 path overlay closure')
 libraries={n:observe(B/rel(n)) for n in old['libraries']};need(len(libraries)==7 and all(r['sha256']==old['libraries'][n]['sha256'] for n,r in libraries.items()),'current7 libraries qualified byte equality');generated={n:observe(B/rel(n)) for n in old['generated']};need(all(r['sha256']==old['generated'][n]['sha256'] for n,r in generated.items()),'current generated parent input equality')
 sdk_before,sbefore=observe(a.parent_sdk_before,True);sdk_after,safter=observe(a.parent_sdk_after,True);need(sbefore['sha256']==a.parent_sdk_before_sha and safter['sha256']==a.parent_sdk_after_sha and sdk_before==sdk_after,'actual parent SDK before/after snapshots');sdk={n:h for h,n in (l.split('  ',1) for l in sdk_before.decode().splitlines())};need(sdk and all(n.startswith(('/usr/','/lib/')) for n in sdk),'actual parent SDK paths');ancestry_refs += [sbefore,safter,cseal]
 for r in rows:
  for d in r['dependencies']:
   if d.startswith('/work/'):need(rel(posixpath.normpath(d[6:])) in sourcepins,'whole project MD closure')
   elif d.startswith('/outputs/'):need(rel(posixpath.normpath(d[9:])) in generated,'parent generated MD closure')
   else:need(d in sdk,'parent actual compilation SDK closure')
 coreflags=observe(B/'CMakeFiles/nano_core.dir/flags.make');need(coreflags['sha256']==old['coreFlagsSha256'],'qualified actual core flags')
 other=[]
 for name,row in terminal['programs'].items():
  pin=observe(B/name);need(pin['sha256']==row['sha256'],'all three qualified parent binaries');other.append(pin)
 for name,row in terminal['consumerObjects'].items():
  pin=observe(B/name);need(pin['sha256']==row['sha256'],'all qualified parent consumer objects');other.append(pin)
 other += [coreflags,observe(B/'CMakeFiles/nano_core.dir/link.txt')]
 for target in terminal['programs']:
  other += [observe(B/'CMakeFiles'/(target+'.dir')/'flags.make'),observe(B/'CMakeFiles'/(target+'.dir')/'link.txt')]

 for r in [*sourcepins.values(),*libraries.values(),*generated.values(),archive,*ancestry_refs,*other,*[r['objectInput'] for r in rows],*[r['mdInput'] for r in rows]]:need(observe(r['path'])['sha256']==r['sha256'],'public input final readback')
 p={'format':'nano-bootstrap-owner-runtime-incremental-plan-v1','status':'READY_ROOT_DISPATCH_ONLY','baselineGitCommit':BASE,'arch':a.arch,'image':IMAGES[a.arch],'uid':501 if a.arch=='arm64' else 1000,'gid':20 if a.arch=='arm64' else 1000,'dockerArgv':['/Users/boris/.docker/bin/docker'] if a.arch=='arm64' else ['/usr/bin/sudo','/usr/bin/docker'],'coreParent':str(B.parent),'parentBuild':str(B),'sourceOrigin':str(S),'outputRoot':a.output_root,'authorManifests':[cseal],'references':ancestry_refs,'parentTerminal':tseal,'parentRawMemberProof':pseal,'parentArchive':archive,'parentCompileCommands':ccseal,'parentOtherProtected':other,'coreFlagsSha256':coreflags['sha256'],'sourcePins':sourcepins,'overlays':overlays,'core':rows,'consumers':consumers,'newCore':'src/consensus_capella_bootstrap_input.cpp','newTestSource':'tests/test_capella_bootstrap_startup.cpp','newTestTarget':'nano-capella-bootstrap-startup-tests','libraries':libraries,'generated':generated,'sdkHashes':sdk,'parentCurrentFreshProbesRequired':143,'resultCoreExpected':142,'resultSourceExpected':1317,'testsExecuted':0,'privateInputsRead':False,'productionQualified':False}
 os.mkdir(D,0o700);data=(json.dumps(p,indent=2)+'\n').encode();fd=os.open(D/'plan.json',os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
 with os.fdopen(fd,'wb') as f:f.write(data);f.flush();os.fsync(f.fileno())
 print(json.dumps({'planSha256':hashlib.sha256(data).hexdigest(),'parentActualCore':141,'inheritedActualMd':124,'physicalActualParentMd':17,'currentSource':1314,'resultSource':1317,'testsExecuted':0}))
if __name__=='__main__':main()
