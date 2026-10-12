from pathlib import Path
import json
E=Path(__file__).resolve().parent

def load(stage):
 d=E/stage/'.cmake/api/v1/reply'
 idx=json.loads(next(d.glob('index-*.json')).read_text())
 m=json.loads((d/idx['reply']['codemodel-v2']['jsonFile']).read_text())
 c=m['configurations'][0]
 def strip(x):
  if isinstance(x,dict):return {k:strip(v) for k,v in x.items() if k not in ('backtrace','backtraceGraph')}
  if isinstance(x,list):return [strip(v) for v in x]
  return x
 return {t['name']:strip(json.loads((d/t['jsonFile']).read_text())) for t in c['targets']}

before=load('baseline');after=load('current')
added=sorted(after.keys()-before.keys());removed=sorted(before.keys()-after.keys())
changed=[k for k in sorted(before.keys()&after.keys()) if before[k]!=after[k]]
report={'format':'nano-isolated-recovery-tooling-cmake-codemodel-v1','configurationOnly':True,'sourceMount':'/work','buildMount':'/outputs','generator':'Unix Makefiles','baselineTargetCount':len(before),'currentTargetCount':len(after),'addedTargets':added,'removedTargets':removed,'changedExistingSemanticTargets':changed,'ignoredOnly':'CMake backtrace and backtraceGraph origin line metadata','nanoCoreUnchanged':before['nano_core']==after['nano_core'],'nanoExecutableUnchanged':before['nano']==after['nano'],'allExistingTargetsUnchanged':not changed and not removed}
assert added==['nano-capella-isolated-bootstrap-audit','nano-capella-isolated-stopped-capture'],added
assert not removed and not changed,changed
for n in ['nano_core','nano','nano-capella-isolated-bootstrap-audit','nano-capella-isolated-stopped-capture']:
 (E/'receipts'/f'{n}.codemodel.semantic.json').write_text(json.dumps(after[n],indent=2)+'\n')
(E/'receipts/codemodel-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
