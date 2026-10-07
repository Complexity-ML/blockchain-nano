from pathlib import Path
import json,subprocess
r=Path(__file__).parent;old=r/'native-affected';out=r/'native-repair';out.mkdir()
(out/'reason.json').write_text(json.dumps({'failedOriginalSession':39084,'exitCode':1,'terminalChunk':'3bfd88','causes':{'test_capella_dependency_preparation':'missing NANO_TEST_DEPENDENCY_PREPARATION, compile refused','test_capella_export_resume':'missing NANO_TEST_EXPORT_CACHE; intentional counterexample path','test_capella_head_history_fairness':'missing NANO_HEAD_HISTORY_FIRST; intentional counterexample path','test_capella_catalog_busy_http':'sandbox loopback bind denied'},'productionSourcesChanged':False,'parentFailuresPreserved':True},indent=2)+'\n')
def run_case(program,d,mode=None):
 a=[str(program)]+([] if mode is None else [mode]);label=mode or 'default'
 with (d/f'{label}-run.log').open('w') as f:
  try:p=subprocess.run(a,stdout=f,stderr=subprocess.STDOUT,timeout=60);code=p.returncode
  except subprocess.TimeoutExpired:code='timeout60'
 (d/f'{label}-exit.json').write_text(json.dumps({'args':a,'exitCode':code})+'\n');print(d.name,label,code,flush=True);assert code==0
for stem,flag in [('test_capella_dependency_preparation','NANO_TEST_DEPENDENCY_PREPARATION'),('test_capella_export_resume','NANO_TEST_EXPORT_CACHE'),('test_capella_head_history_fairness','NANO_HEAD_HISTORY_FIRST')]:
 d=out/stem;d.mkdir()
 for stage in ['compile','link']:
  a=json.loads((old/stem/f'{stage}-command.json').read_text());a=[x.replace(str(old/stem),str(d)) for x in a]
  if stage=='compile':a.insert(1,'-D'+flag)
  (d/f'{stage}-command.json').write_text(json.dumps(a)+'\n')
  with (d/f'{stage}.log').open('w') as f:p=subprocess.run(a,stdout=f,stderr=subprocess.STDOUT)
  (d/f'{stage}-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,(stem,stage)
 modes=['normal','duplicate','unavailable','unavailable-anchor','refused','lost','cancel','slow','slow-duplicate','slow-one-step'] if stem=='test_capella_dependency_preparation' else [None]
 for mode in modes:run_case(d/'program',d,mode)
d=out/'test_capella_catalog_busy_http';d.mkdir();run_case(old/'test_capella_catalog_busy_http/program',d)
d=out/'test_capella_deep_preparation';d.mkdir()
for mode in ['anchor','moving','reset','refused']:run_case(old/'test_capella_deep_preparation/program',d,mode)
print('only affected harness failures repaired and additional configured modes executed',flush=True)
