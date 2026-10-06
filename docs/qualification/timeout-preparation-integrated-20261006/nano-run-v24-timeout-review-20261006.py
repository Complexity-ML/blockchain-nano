from pathlib import Path
import subprocess,json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-v24-timeout-continuation-draft-20261006');out=root/'native-review-proof';out.mkdir(exist_ok=False)
base=repo/'docs/qualification/delivery-diagnostic-integrated-20261006/native'
compile_parent=json.loads((base/'capella_head_catalog_relay-compile-command.json').read_text());link_parent=json.loads((base/'capella_head_catalog_relay-link-command.json').read_text())
frozen={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for kind in ['baseline-review-source','draft-review-source'] for p in sorted((root/kind).rglob('*')) if p.is_file()};(out/'source-before.json').write_text(json.dumps(frozen,indent=2)+'\n')
def run(name,args,expected):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode,'expectedExitCode':expected})+'\n');assert p.returncode==expected,name
for mode in ['baseline','draft']:
 directory=out/mode;directory.mkdir();source=root/(mode+'-review-source')/'test_timeout_continuation.cpp';obj=directory/'test.o';binary=directory/'test'
 args=compile_parent.copy();args[args.index('-c')+1]=str(source);args[args.index('-o')+1]=str(obj);args[1:1]=['-DNANO_TEST_DEPENDENCY_PREPARATION','-I'+str(root/(mode+'-review-source'))];args.extend(['-MMD','-MF',str(directory/'test.d')]);run(mode+'-compile',args,0)
 args=link_parent.copy();args[args.index('-o')+1]=str(binary);args=[str(obj) if x.endswith('/capella_head_catalog_relay.o') else x for x in args];run(mode+'-link',args,0)
 for scenario in ['slow','slow-duplicate'] if mode=='baseline' else ['normal','duplicate','unavailable','unavailable-anchor','refused','lost','cancel','slow','slow-duplicate']:run(mode+'-'+scenario,[str(binary),scenario],1 if mode=='baseline' else 0)
 (directory/'program.sha256').write_text(hashlib.sha256(binary.read_bytes()).hexdigest()+'  test\n')
assert frozen=={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for kind in ['baseline-review-source','draft-review-source'] for p in sorted((root/kind).rglob('*')) if p.is_file()}
(out/'source-immutability.json').write_text(json.dumps({'sourceFiles':len(frozen),'hashesUnchanged':True,'runtimeCompiled':False,'wanQualified':False})+'\n')
print('Current relay slow-export counterexamples failed as expected; isolated draft nine modes passed. Runtime, Docker and WAN remain unqualified.')
