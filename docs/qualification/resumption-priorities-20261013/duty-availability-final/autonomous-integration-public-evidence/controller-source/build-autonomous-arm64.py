import pathlib,hashlib,json,subprocess,time,shlex,sys
base=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012');out=base/'new-duty-projection-availability-counterexample-20261013/candidate-preparation/integration-arm64';out.mkdir(exist_ok=True)
repo=pathlib.Path('/Users/boris/Dev/blockchain-nano');build=base/'duty-availability-coherent-arm64-20261013/build';image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';source=repo/'tests/test_capella_duty_availability.cpp'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(n,v):(out/n).write_text(json.dumps(v,indent=2)+'\n')
parents={'libnano_core.a':sha(build/'libnano_core.a'),'nano':sha(build/'nano')};assert parents=={'libnano_core.a':'161a3450c163aa324ca7c365beb028abb448fbd6912dfb34fcc26d8d230aae26','nano':'64ca2a4bf25e0d0bd82e6479f735070ca2cf2b43b756aa8b528d38169ebd0e19'}
core_pins=json.loads((out.parent/'postready-sync-extension/final-six-source-binding.json').read_text())['files'];assert all(sha(repo/row['path'])==row['sha256'] for row in core_pins)
fixture_root=repo/'tests/fixtures';asset_names=['consensus-capella-slots-official/manifest.json','consensus-slashing-official/manifest.json','consensus-altair-state-roots-official/manifest.json','consensus-altair-state-roots-official/minimal-altair.yaml','consensus-bellatrix-containers-official/minimal-bellatrix.yaml','consensus-capella-withdrawals-official/minimal-capella.yaml','consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml']
public_manifest=json.loads((fixture_root/'consensus-capella-slots-official/manifest.json').read_text());row=next(r for r in public_manifest['cases'] if r['name']=='slots_1');asset_names += ['consensus-capella-slots-official/slots_1/'+n for n in row['files']]
reference_manifest=json.loads((fixture_root/'consensus-slashing-official/manifest.json').read_text());asset_names += ['consensus-slashing-official/'+n for n in reference_manifest['referenceFiles']]
asset_pins={n:sha(fixture_root/n) for n in asset_names}
pins={'sourceSha256':sha(source),'driverSha256':sha(pathlib.Path(__file__)),'coreSources':core_pins,'parentsBefore':parents,'image':image,'publicFixtureAssetsBefore':asset_pins};
flags=(build/'CMakeFiles/nano-capella-transition-tests.dir/flags.make').read_text();variables={line.split('=',1)[0].strip():line.split('=',1)[1].strip() for line in flags.splitlines() if line.startswith('CXX_')};compile=['/usr/bin/clang++']+shlex.split(variables['CXX_DEFINES'])+shlex.split(variables['CXX_INCLUDES'])+shlex.split(variables['CXX_FLAGS'])+['-c','/work/tests/test_capella_duty_availability.cpp','-o','/testing/test.o']
link=shlex.split((build/'CMakeFiles/nano-capella-transition-tests.dir/link.txt').read_text());link[link.index('-o')+1]='/testing/test-duty-availability';old_object=next(i for i,v in enumerate(link) if v.endswith('test_capella_transition.cpp.o'));link[old_object]='/testing/test.o';link.append('-Wl,-Map,/testing/link.map')
def docker(args,memory='4g'):
 return ['docker','run','--rm','--pull=never','--platform','linux/arm64','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--cpus','2','--memory',memory,'--memory-swap',memory,'--pids-limit','128','--mount','type=bind,source='+str(build)+',target=/outputs,readonly','--mount','type=bind,source='+str(repo)+',target=/work,readonly','--mount','type=bind,source='+str(out)+',target=/testing','--workdir','/outputs',image]+args
mode=sys.argv[1] if len(sys.argv)>1 else '--build-only'
if mode=='--build-only':
 save('before-build-source-binding.json',pins)
 receipt={'format':'nano-duty-availability-autonomous-arm64-build-v1','beforeBindings':pins,'source':'tests/test_capella_duty_availability.cpp','cmakeGeneratedFlagsSha256':sha(build/'CMakeFiles/nano-capella-transition-tests.dir/flags.make'),'cmakeGeneratedLinkSha256':sha(build/'CMakeFiles/nano-capella-transition-tests.dir/link.txt'),'onlyNewTestObjectLinked':True,'commands':{}}
 for name,args in [('compile',compile),('link',link)]:
  command=docker(args);receipt['commands'][name]=command;save(name+'-command.json',{'args':command});started=time.monotonic()
  with (out/(name+'.log')).open('wb') as log:r=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=120)
  receipt[name]={'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-started};save('build-receipt.json',receipt)
  if r.returncode:raise SystemExit(r.returncode)
 receipt['sourceSha256After']=sha(source);receipt['parentsAfter']={n:sha(build/n) for n in parents};assert receipt['sourceSha256After']==pins['sourceSha256'] and receipt['parentsAfter']==parents
 receipt['helperBinarySha256']=sha(out/'test-duty-availability');receipt['linkMapSha256']=sha(out/'link.map');save('build-receipt.json',receipt);print(json.dumps({'result':'BUILT_ONLY','sourceSha256':pins['sourceSha256'],'helperSha256':receipt['helperBinarySha256'],'parentsUnchanged':True}))
else:
 receipt=json.loads((out/'build-receipt.json').read_text());assert sha(source)==receipt['sourceSha256After'] and sha(out/'test-duty-availability')==receipt['helperBinarySha256']
 if mode=='--run-self-test':name='self-test';arguments=['/testing/test-duty-availability','--self-test']
 elif mode=='--run-pure63':name='pure63';arguments=['/testing/test-duty-availability','--pure63']
 elif mode=='--run-reactor63':name='reactor63';arguments=['/testing/test-duty-availability','--reactor63','/testing/reactor63-data'];assert not (out/'reactor63-data').exists()
 elif mode=='--run-reactor':name='reactor';arguments=['/testing/test-duty-availability','--reactor','/testing/reactor-data'];assert not (out/'reactor-data').exists()
 elif mode=='--run-reactor-repaired':name='reactor-bounded-admission';arguments=['/testing/test-duty-availability','--reactor','/testing/reactor-bounded-admission-data'];assert not (out/'reactor-bounded-admission-data').exists()
 elif mode=='--run-reactor-cold':name='reactor-cold';arguments=['/testing/test-duty-availability','--reactor','/testing/reactor-cold-data'];assert not (out/'reactor-cold-data').exists()
 else:raise ValueError('unknown mode')
 command=docker(arguments,'128m');command[command.index('--pids-limit'):command.index('--pids-limit')]=['--ulimit','cpu=60:60']
 if mode=='--run-self-test':
  (out/'self-test-tmp').mkdir(mode=0o700)
  command[command.index(image):command.index(image)]=['--env','TMPDIR=/testing/self-test-tmp']
 save(name+'-command.json',{'args':command});start=time.monotonic()
 with (out/(name+'.log')).open('wb') as log:r=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=65)
 run={'format':'nano-duty-availability-autonomous-arm64-run-v1','mode':mode,'command':command,'exitCode':r.returncode,'elapsedSeconds':time.monotonic()-start,'sourceSha256Before':pins['sourceSha256'],'sourceSha256After':sha(source),'helperSha256':sha(out/'test-duty-availability'),'parentsBefore':parents,'parentsAfter':{n:sha(build/n) for n in parents},'coreSources':core_pins,'publicFixtureAssetsBefore':asset_pins,'publicFixtureAssetsAfter':{n:sha(fixture_root/n) for n in asset_pins},'memoryLimitBytes':134217728,'cpuLimitSeconds':60,'collectorWallLimitSeconds':65,'networkDisabled':True,'productionQualified':False};save(name+'-receipt.json',run);assert run['parentsAfter']==parents and run['publicFixtureAssetsAfter']==asset_pins and run['sourceSha256Before']==run['sourceSha256After'];print(json.dumps({'mode':mode,'exitCode':r.returncode,'elapsedSeconds':run['elapsedSeconds'],'sourceSha256':run['sourceSha256After'],'runReceiptSha256':sha(out/(name+'-receipt.json'))}));raise SystemExit(r.returncode)
