from pathlib import Path
import json,hashlib,shutil
root=Path('/private/tmp/nano-catalog-selection-docker-20261006');dest=root/'native-proof';dest.mkdir(exist_ok=False)
for directory,label in [('nano-catalog-selection-native-20261006','core'),('nano-catalog-selection-native-sdk-20261006','sdk'),('nano-catalog-selection-native-network-20261006','network')]:
 src=Path('/private/tmp')/directory
 for f in src.rglob('*'):
  if f.is_file() and f.suffix in ['.json','.log','.txt','.d','.sha256']:
   t=dest/label/f.relative_to(src);t.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,t)
core=dest/'core';sdk=dest/'sdk';network=dest/'network'
for mode in ['configured-runtime','automatic-recovery','http','wire']:assert json.loads((core/f'candidate-{mode}-exit.json').read_text())['exitCode']==0
for name in ['capella_catalog_selection','capella_catalog_relay','capella_head_catalog_relay','capella_catalog_termination']:assert json.loads((sdk/(name+'-run-exit.json')).read_text())['exitCode']==0
assert json.loads((sdk/'capella_head_history_fairness-run-exit.json').read_text())['exitCode']==1
assert json.loads((sdk/'fairness-cmake-definitions/run-exit.json').read_text())['exitCode']==0
assert json.loads((network/'runtime-link-exit.json').read_text())['exitCode']==0
summary={'sourceBase':'19b722bd47d1cfea679a5235dab117499ba2b4ec','frozenSourceFiles':345,'nativeCoreOriginalSession':70693,'nativeCoreExit':0,'nativeNetworkOriginalSession':7036,'nativeNetworkExit':0,'coreRebuiltUnits':18,'coreScenarios':['configured-runtime','automatic-recovery','http','wire'],'sdkCases':{'selection':9,'catalog':46,'head':19,'termination':7,'historyFirst':10},'failuresRetained':['first helper wrong source filename; exit 2','fairness without required NANO_HEAD_HISTORY_FIRST definition; exit 1','corrected helper nonexistent cwd; no linker executed; exit 1'],'dockerPending':True,'wanQualified':False,'productionQualified':False,'faultInjectionEnabled':True}
(dest/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
manifest={str(f.relative_to(dest)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(dest.rglob('*')) if f.is_file()};(dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(len(manifest),'native proof files collected; failures preserved; Docker pending')
