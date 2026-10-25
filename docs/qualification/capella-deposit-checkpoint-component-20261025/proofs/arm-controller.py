#!/usr/bin/env python3
"""Bounded component build: reuse byte-identified Core; no image build/node."""
import hashlib,json,os,pathlib,shlex,subprocess,sys,time,xml.etree.ElementTree as ET
P=pathlib.Path
cfg=json.loads(P(sys.argv[1]).read_text()); root=P(cfg['directory']); out=root/'outputs'; source=root/'source'
root.mkdir(parents=True,exist_ok=True)
result={'format':'nano-deposit-checkpoint-component-build-v1','sourceCandidate':cfg['revision'],'architecture':cfg['architecture'],'image':cfg['image'],'imageBuilds':0,'networkCampaignStarted':False,'productionQualified':False,'phases':[],'passed':False}
def sha(p):
 h=hashlib.sha256()
 with open(p,'rb') as f:
  for c in iter(lambda:f.read(1024*1024),b''):h.update(c)
 return h.hexdigest()
def run(name,argv,timeout=600):
 (root/(name+'-argv.json')).write_text(json.dumps(argv,indent=2)+'\n');start=time.monotonic()
 with open(root/(name+'.stdout.log'),'wb') as o,open(root/(name+'.stderr.log'),'wb') as e:
  p=subprocess.run(argv,stdout=o,stderr=e,timeout=timeout)
 result['phases'].append({'name':name,'exitCode':p.returncode,'elapsedMs':round((time.monotonic()-start)*1000)})
 print(name,p.returncode,flush=True)
 if p.returncode:raise RuntimeError(name+' failed; logs retained')
try:
 old=P(cfg['previousOutputs']); pin=cfg['corePin']
 assert (old/'libnano_core.a').stat().st_size==pin['bytes'] and sha(old/'libnano_core.a')==pin['sha256'],'inherited Core differs'
 assert not out.exists() and source.is_dir()
 # Existing evidence stays untouched. The incremental workspace is a separate copy.
 copy=['cp','-cR'] if sys.platform=='darwin' else ['cp','-a','--reflink=auto']
 run('clone-output',copy+[str(old),str(out)])
 result['inheritedCore']=pin
 assert sha(out/'libnano_core.a')==pin['sha256']
 for rel,pin in cfg['sourcePins'].items():
  f=source/rel; assert f.stat().st_size==pin['bytes'] and sha(f)==pin['sha256'],rel
 # Incremental cache authority is payload equality, not timestamps. Explicitly
 # keep unchanged sources older, with changed inputs newer than copied objects.
 changed=set(cfg['changedBuildInputs'])
 for rel in cfg['sourcePins']:
  f=source/rel;os.utime(f,(1,1))
 for rel in changed:
  assert rel in cfg['sourcePins'];os.utime(source/rel,None)
 before={str(f.relative_to(out)):sha(f) for f in (out/'CMakeFiles/nano_core.dir/src').glob('*.o')}
 base=cfg['docker']+['run','--rm','--pull=never','--network=none','--cpus=2','--memory=3g','--pids-limit=256','--user',cfg['user'],'--read-only','--tmpfs','/tmp:rw,nosuid,nodev,size=512m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(out)+',dst=/out','--workdir','/out',cfg['image']]
 run('configure',base+['cmake','-S','/work','-B','/out','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 run('build',base+['cmake','--build','/out','--verbose','--parallel','2','--target',*cfg['targets']],1200)
 compiled=[line for line in (root/'build.stdout.log').read_text().splitlines() if ' -c /work/' in line and 'clang++' in line]
 result['compileCommands']=compiled
 core_compiles=[line for line in compiled if ' -c /work/src/' in line]
 compiled_core_sources={line.rsplit(' -c ',1)[1] for line in core_compiles}
 assert set(cfg['requiredCoreCompiles'])<=compiled_core_sources,core_compiles
 if '/work/src/kzg.cpp' in compiled_core_sources:
  for rel in ['CMakeFiles/nano_core.dir/src/kzg.cpp.o','third_party/ckzg/include/nano_kzg_setup.hpp']:
   assert sha(out/rel)==sha(old/rel),rel
  result['kzgObjectAndGeneratedSetupByteIdentical']=True
 after={str(f.relative_to(out)):sha(f) for f in (out/'CMakeFiles/nano_core.dir/src').glob('*.o')}
 changed_objects=sorted(k for k,v in after.items() if before.get(k)!=v)
 result['coreObjectsChanged']=changed_objects
 result['coreObjectCount']=len(after)
 assert len(after)==145
 run('archive-members',base+['ar','t','/out/libnano_core.a'])
 members=(root/'archive-members.stdout.log').read_text().splitlines()
 assert len(members)==145 and sorted(members)==sorted(P(k).name for k in after)
 run('registry',base+['ctest','--test-dir','/out','-R',cfg['pattern'],'--show-only=json-v1'])
 registry=json.loads((root/'registry.stdout.log').read_text());names=sorted(x['name'] for x in registry['tests']);assert names==sorted(cfg['tests']),names
 run('tests',base+['ctest','--test-dir','/out','-R',cfg['pattern'],'--output-on-failure','--no-tests=error','--parallel','1','--output-junit','/out/focused.xml'],1200)
 tree=ET.parse(out/'focused.xml');cases=tree.findall('.//testcase');assert sorted(c.attrib['name'] for c in cases)==sorted(cfg['tests'])
 assert all(c.find('failure') is None and c.find('skipped') is None and c.find('error') is None for c in cases)
 result['tests']=names;result['passed']=True
 result['artifacts']={n:{'bytes':(out/n).stat().st_size,'sha256':sha(out/n)} for n in ['libnano_core.a',*cfg['targets'],'focused.xml','Testing/Temporary/LastTest.log']}
except Exception as e:result['failure']={'type':type(e).__name__,'reason':str(e)}
finally:
 result['finishedAtUnix']=time.time();(root/'result-public.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result.get(k) for k in ['passed','failure','coreObjectsChanged','tests']}),flush=True)
sys.exit(0 if result['passed'] else 1)
