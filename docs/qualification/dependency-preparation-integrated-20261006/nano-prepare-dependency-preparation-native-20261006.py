from pathlib import Path
import shutil
base=Path('/private/tmp');source=(base/'nano-qualify-delivery-diagnostic-native-20261006.py').read_text();source=source.replace('nano-delivery-diagnostic-docker-20261006/source','nano-dependency-preparation-docker-20261006/source').replace('nano-delivery-diagnostic-native-20261006','nano-dependency-preparation-native-20261006');source=source.replace("for case in ['core_rpc_diagnostic'", "for case in ['capella_dependency_preparation','core_rpc_diagnostic'")
source=source.replace("if case=='capella_export_resume':", "if case=='capella_dependency_preparation':c.insert(1,'-DNANO_TEST_DEPENDENCY_PREPARATION')\n if case=='capella_export_resume':")
source=source.replace("run(case+'-link',l);run(case+'-run',[str(out/case)]);print", "run(case+'-link',l);run(case+'-run',[str(out/case)]);\n if case=='capella_dependency_preparation':\n  for mode in ['duplicate','unavailable','unavailable-anchor','refused','lost','cancel']:run(case+'-'+mode,[str(out/case),mode])\n print")
start=source.index("t=base/'nano-catalog-selection-native-20261006';c=read(t/'test-compile-command.json')")
source=source[:start]+'''t=base/'nano-network-status-coherent-native-20261006';c=read(t/'test-compile-command.json')
support=out/'fixture-support';shutil.copytree(repo/'tests/support',support/'tests/support');(support/'network').symlink_to(frozen/'network',target_is_directory=True)
c=[x.replace(str(t/'fixture-support'),str(support)).replace('/private/tmp/nano-network-status-coherent-20261006/source',str(frozen)) for x in c]
c[c.index('-o')+1]=str(out/'candidate.o');run('wire-compile',c)
l=read(t/'test-link-command.json');l[l.index('-o')+1]=str(out/'candidate');l=[str(out/'candidate.o') if x==str(t/'candidate.o') else x for x in l];run('wire-link',l,'/tmp/nano-pruning-wan-native-build-20261004');run('wire-run',[str(out/'candidate'),'--wire']);print((out/'wire-run.log').read_text()[-1200:],flush=True)
assert frozen_hashes=={str(p.relative_to(frozen)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(frozen.rglob('*')) if p.is_file()}
(out/'source-immutability.json').write_text(json.dumps({'files':len(frozen_hashes),'hashesUnchanged':True})+'\\n')
'''
source=source.replace('import json,subprocess,hashlib','import json,subprocess,hashlib,shutil');source=source.replace('def read(p):', "frozen_hashes={str(p.relative_to(frozen)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(frozen.rglob('*')) if p.is_file()};(out/'source-before.json').write_text(json.dumps(frozen_hashes,indent=2)+'\\n')\ndef read(p):")
p=base/'nano-qualify-dependency-preparation-native-20261006.py';p.write_text(source);compile(source,str(p),'exec');print(p)
