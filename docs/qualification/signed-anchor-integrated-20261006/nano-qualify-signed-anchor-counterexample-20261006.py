from pathlib import Path
import json,subprocess,sys
r=Path('/private/tmp/nano-signed-anchor-counterexample-proof-20261006');r.mkdir(exist_ok=True);t=Path('/private/tmp/nano-catalog-selection-native-sdk-20261006');source=Path('/private/tmp/nano-signed-anchor-counterexample-20261006.cpp');kind=sys.argv[1]
def run(name,args,expected=0):
 (r/(name+'-command.json')).write_text(json.dumps(args,indent=2))
 with (r/(name+'.log')).open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
 (r/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode,'expectedExitCode':expected}));assert p.returncode==expected,name
c=json.loads((t/'capella_catalog_selection-compile-command.json').read_text());c[c.index('-c')+1]=str(source);c[c.index('-o')+1]=str(r/(kind+'.o'))
if kind=='draft':c.insert(1,'-I/private/tmp/nano-signed-anchor-draft-20261006')
run(kind+'-compile',c);l=json.loads((t/'capella_catalog_selection-link-command.json').read_text());l=[str(r/(kind+'.o')) if x==str(t/'capella_catalog_selection.o') else str(r/kind) if x==str(t/'capella_catalog_selection') else x for x in l];run(kind+'-link',l);run(kind+'-run',[str(r/kind)],1 if kind=='baseline' else 0);print((r/(kind+'-run.log')).read_text(),flush=True)
