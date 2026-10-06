from pathlib import Path
import json,subprocess,hashlib,shutil
base=Path('/private/tmp');repo=Path('/Users/boris/Dev/blockchain-nano');frozen=base/'nano-timeout-closed-guard-docker-20261006/source';out=base/'nano-timeout-closed-guard-native-20261006';out.mkdir(exist_ok=False);template=base/'nano-catalog-selection-native-sdk-20261006'
frozen_hashes={str(p.relative_to(frozen)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(frozen.rglob('*')) if p.is_file()};(out/'source-before.json').write_text(json.dumps(frozen_hashes,indent=2)+'\n')
def read(p):return json.loads(p.read_text())
def run(name,args,cwd=None,expected=0):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,cwd=cwd)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode,'expectedExitCode':expected}));assert p.returncode==expected,name
for case in ['capella_dependency_preparation','capella_head_catalog_relay','capella_catalog_termination','capella_head_history_fairness']:
 c=read(template/'capella_catalog_selection-compile-command.json');c.insert(1,'-I'+str(frozen/'network'))
 if case=='capella_dependency_preparation':c.insert(1,'-DNANO_TEST_DEPENDENCY_PREPARATION')
 if case=='capella_export_resume':c.insert(1,'-DNANO_TEST_EXPORT_CACHE')
 if case=='capella_head_history_fairness':c.insert(1,'-DNANO_HEAD_HISTORY_FIRST')
 c[c.index('-c')+1]=str(frozen/'network'/('test_'+case+'.cpp'));c[c.index('-o')+1]=str(out/(case+'.o'));run(case+'-compile',c)
 l=read(template/'capella_catalog_selection-link-command.json');l=[str(out/(case+'.o')) if x==str(template/'capella_catalog_selection.o') else str(out/case) if x==str(template/'capella_catalog_selection') else x for x in l];run(case+'-link',l);run(case+'-run',[str(out/case)]);
 if case=='capella_dependency_preparation':
  for mode in ['duplicate','unavailable','unavailable-anchor','refused','lost','cancel','slow','slow-duplicate']:run(case+'-'+mode,[str(out/case),mode])
 print(case,(out/(case+'-run.log')).read_text().strip(),flush=True)
t=base/'nano-dispatch-recovery-p2p-native-20261006';c=read(t/'compile-command.json');c.insert(1,'-I'+str(frozen/'network'));c[c.index('-c')+1]=str(frozen/'network/main.cpp');c[c.index('-o')+1]=str(out/'runtime.o');run('runtime-compile',c)
l=read(t/'link-command.json');l=[str(out/'runtime.o') if x==str(t/'test.o') else str(out/'runtime') if x==str(t/'test') else x for x in l];run('runtime-link',l);run('runtime-usage',[str(out/'runtime')],expected=1)
(out/'runtime-program.json').write_text(json.dumps({'sha256':hashlib.sha256((out/'runtime').read_bytes()).hexdigest(),'sourceSHA256':hashlib.sha256((frozen/'network/main.cpp').read_bytes()).hexdigest()},indent=2)+'\n')
assert frozen_hashes=={str(p.relative_to(frozen)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(frozen.rglob('*')) if p.is_file()}
(out/'source-immutability.json').write_text(json.dumps({'files':len(frozen_hashes),'hashesUnchanged':True})+'\n')
