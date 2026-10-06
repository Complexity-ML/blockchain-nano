from pathlib import Path
import json,subprocess,hashlib
r=Path('/private/tmp/nano-catalog-generation-rotation-counterexample-proof-20261006');r.mkdir(exist_ok=False);template=Path('/private/tmp/nano-catalog-selection-native-sdk-20261006');source=Path('/private/tmp/nano-catalog-generation-rotation-counterexample-20261006.cpp');draft=Path('/private/tmp/nano-catalog-generation-rotation-draft-20261006')
def run(cmd,name,expected=0):
 (r/(name+'-command.json')).write_text(json.dumps(cmd,indent=2)+'\n')
 with (r/(name+'.log')).open('w') as f:p=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT)
 (r/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode,'expectedExitCode':expected}));assert p.returncode==expected,(name,p.returncode)
for kind in ['baseline','draft']:
 c=json.loads((template/'capella_catalog_selection-compile-command.json').read_text());c[c.index('-c')+1]=str(source);c[c.index('-o')+1]=str(r/(kind+'.o'))
 if kind=='draft':c.insert(1,'-I'+str(draft))
 run(c,kind+'-compile');link=json.loads((template/'capella_catalog_selection-link-command.json').read_text());link=[str(r/(kind+'.o')) if x==str(template/'capella_catalog_selection.o') else str(r/kind) if x==str(template/'capella_catalog_selection') else x for x in link];run(link,kind+'-link');run([str(r/kind)],kind+'-run',1 if kind=='baseline' else 0);print(kind,(r/(kind+'-run.log')).read_text().strip(),flush=True)
(r/'scope.json').write_text(json.dumps({'counterexampleSourceSHA256':hashlib.sha256(source.read_bytes()).hexdigest(),'baselineExpectedExit':1,'draftExit':0,'mockedTransportAndNamespace':True,'actualConsensusOrWanQualified':False,'repositorySourceChanged':False,'compiledAfterCampaignTerminal':True},indent=2)+'\n')
