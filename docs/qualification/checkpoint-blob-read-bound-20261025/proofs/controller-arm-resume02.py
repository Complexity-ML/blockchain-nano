import pathlib,json,hashlib,subprocess,time,sys,xml.etree.ElementTree as ET
P=pathlib.Path
cfg=json.loads(P(sys.argv[1]).read_text()); original=P(cfg['directory']); root=original.parent/(original.name+'-tests02');root.mkdir(exist_ok=False)
out=original/'outputs';source=original/'source';old=P(cfg['previousOutputs'])
result={'format':'nano-checkpoint-blob-focused-resume-v1','passed':False,'cppCandidate':cfg['revision'],'priorEnvelopeRemainsFailed':True,'imageBuilds':0,'networkCampaignStarted':False,'productionQualified':False,'phases':[]}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(n,argv,limit=600):
 (root/(n+'-argv.json')).write_text(json.dumps(argv,indent=2)+'\n');start=time.monotonic()
 with open(root/(n+'.stdout.log'),'wb') as o,open(root/(n+'.stderr.log'),'wb') as e:p=subprocess.run(argv,stdout=o,stderr=e,timeout=limit)
 result['phases'].append({'name':n,'exitCode':p.returncode,'elapsedMs':round(1000*(time.monotonic()-start))});print(n,p.returncode,flush=True)
 if p.returncode:raise RuntimeError(n+' failed')
try:
 prior=json.loads((original/'result-public.json').read_text());assert not prior['passed'] and prior['phases'][-1]['name']=='build' and prior['phases'][-1]['exitCode']==0
 assert prior['failure']['type']=='AssertionError'
 for rel,pin in cfg['sourcePins'].items():assert sha(source/rel)==pin['sha256'],rel
 changed=[str(f.relative_to(out)) for f in (out/'CMakeFiles/nano_core.dir/src').glob('*.o') if sha(f)!=sha(old/f.relative_to(out))]
 assert changed==['CMakeFiles/nano_core.dir/src/storage.cpp.o'],changed
 for rel in ['CMakeFiles/nano_core.dir/src/kzg.cpp.o','third_party/ckzg/include/nano_kzg_setup.hpp']:
  assert sha(out/rel)==sha(old/rel),rel
 compiled=[x for x in prior['compileCommands'] if ' -c /work/src/' in x]
 assert len(compiled)==2 and {x.rsplit(' -c ',1)[1] for x in compiled}=={'/work/src/storage.cpp','/work/src/kzg.cpp'}
 result['actualCoreCompiles']=2;result['changedCoreObjectPayloads']=changed;result['kzgObjectAndGeneratedSetupByteIdentical']=True
 artifacts={n:sha(out/n) for n in ['libnano_core.a',*cfg['targets']]}
 base=cfg['docker']+['run','--rm','--pull=never','--network=none','--cpus=2','--memory=3g','--pids-limit=256','--user',cfg['user'],'--read-only','--tmpfs','/tmp:rw,nosuid,nodev,size=512m','--mount','type=bind,src='+str(source)+',dst=/work,readonly','--mount','type=bind,src='+str(out)+',dst=/out','--workdir','/out',cfg['image']]
 run('archive',base+['ar','t','/out/libnano_core.a'])
 members=(root/'archive.stdout.log').read_text().splitlines();assert len(members)==145 and sorted(members)==sorted(f.name for f in (out/'CMakeFiles/nano_core.dir/src').glob('*.o'))
 run('registry',base+['ctest','--test-dir','/out','-R',cfg['pattern'],'--show-only=json-v1'])
 names=sorted(x['name'] for x in json.loads((root/'registry.stdout.log').read_text())['tests']);assert names==sorted(cfg['tests'])
 run('tests',base+['ctest','--test-dir','/out','-R',cfg['pattern'],'--output-on-failure','--no-tests=error','--parallel','1','--output-junit','/out/focused.xml'],900)
 cases=ET.parse(out/'focused.xml').findall('.//testcase');assert sorted(c.attrib['name'] for c in cases)==names
 assert all(c.find('failure') is None and c.find('error') is None and c.find('skipped') is None for c in cases)
 assert all(sha(out/n)==h for n,h in artifacts.items())
 result.update(passed=True,tests=names,artifacts=artifacts)
 for rel in ['focused.xml','Testing/Temporary/LastTest.log']:(root/P(rel).name).write_bytes((out/rel).read_bytes())
except Exception as e:result['failure']={'type':type(e).__name__,'reason':str(e)}
finally:(root/'result-public.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
sys.exit(0 if result['passed'] else 1)
