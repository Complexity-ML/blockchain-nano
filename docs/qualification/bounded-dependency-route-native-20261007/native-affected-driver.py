from pathlib import Path
import json,shutil,subprocess,hashlib
r=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');out=r/'native-affected';out.mkdir();source=out/'source/network';shutil.copytree(repo/'network',source)
for name in ['capella_block_delivery.hpp','capella_head_catalog_relay.hpp']:shutil.copy2(r/'network'/name,source/name)
(out/'sources.json').write_text(json.dumps({str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(source.rglob('*')) if p.is_file()},indent=2)+'\n')
base=Path('/private/tmp/nano-long-dependency-profile-20261007');compile_args=json.loads((base/'actual-compile-command.json').read_text());link_args=json.loads((base/'actual-link-command.json').read_text());results=[]
for name in sorted(json.loads((r/'affected-source-consumers.json').read_text())):
 stem=Path(name).stem;d=out/stem;d.mkdir();row={'source':name,'stages':{}}
 if name=='test_capella_head_catalog_relay.cpp':
  row['reusedScopedQualification']={'session':26061,'exitCode':0,'terminalChunk':'6e92b9','unchangedSource':hashlib.sha256((source/name).read_bytes()).hexdigest()==hashlib.sha256((r/'head-regression/test.cpp').read_bytes()).hexdigest()};assert row['reusedScopedQualification']['unchangedSource'];results.append(row);continue
 for stage,template in [('compile',compile_args),('link',link_args)]:
  a=[str(source/name) if x==str(base/'profile-actual.cpp') else str(d/'program.o') if x==str(base/'profile-actual.o') else str(d/'program') if x==str(base/'profile-actual') else x for x in template]
  if stage=='compile':a[1:1]=['-I'+str(source),'-MMD','-MF',str(d/'program.d')]
  (d/f'{stage}-command.json').write_text(json.dumps(a)+'\n')
  with (d/f'{stage}.log').open('w') as f:p=subprocess.run(a,stdout=f,stderr=subprocess.STDOUT)
  row['stages'][stage]=p.returncode;(d/f'{stage}-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n')
  if p.returncode:break
 if row['stages'].get('link')==0 and name!='main.cpp':
  with (d/'run.log').open('w') as f:
   try:p=subprocess.run([str(d/'program')],stdout=f,stderr=subprocess.STDOUT,timeout=120);code=p.returncode
   except subprocess.TimeoutExpired:code='timeout120'
  row['stages']['run']=code;(d/'run-exit.json').write_text(json.dumps({'exitCode':code})+'\n')
 results.append(row);(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(row),flush=True)
assert len(results)==13
assert all(all(code==0 for code in x['stages'].values()) for x in results)
print('13 consumers covered, one scoped head regression reused; main compiled and linked only',flush=True)
