from pathlib import Path
import subprocess,json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-v22-depth-continuation-draft-20261006')
t=json.loads(Path('/private/tmp/nano-restored-three-node-v22-network-status-20261006/campaign/terminal-exit.json').read_text());assert t['originalSession']==91325 and t['exitCode'] in [0,1]
r=json.loads(Path('/private/tmp/nano-restored-three-node-v22-network-status-20261006/campaign/result.json').read_text());assert r['stopReceipts']==3
out=root/'native-proof';out.mkdir(exist_ok=False)
base=repo/'docs/qualification/delivery-diagnostic-integrated-20261006/native'
compile_parent=json.loads((base/'capella_head_catalog_relay-compile-command.json').read_text());link_parent=json.loads((base/'capella_head_catalog_relay-link-command.json').read_text())
def run(name,args,expected):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode,'expectedExitCode':expected})+'\n');assert p.returncode==expected,name
for mode in ['baseline','draft']:
 directory=out/mode;directory.mkdir();source=directory/'test.cpp';shutil.copy2(root/'test_depth_continuation.cpp',source)
 obj=directory/'test.o';binary=directory/'test'
 args=compile_parent.copy();args[args.index('-c')+1]=str(source);args[args.index('-o')+1]=str(obj)
 args[1:1]=['-I'+str(root if mode=='draft' else repo/'network')]
 if mode=='draft':args.insert(1,'-DNANO_TEST_DEPENDENCY_PREPARATION')
 args.extend(['-MMD','-MF',str(directory/'test.d')]);run(mode+'-compile',args,0)
 args=link_parent.copy();args[args.index('-o')+1]=str(binary)
 args=[str(obj) if x.endswith('/capella_head_catalog_relay.o') else x for x in args];run(mode+'-link',args,0)
 if mode=='baseline':run(mode+'-normal',[str(binary),'normal'],1)
 else:
  for scenario in ['normal','duplicate','unavailable','refused','lost','cancel']:run(mode+'-'+scenario,[str(binary),scenario],0)
 (directory/'program.sha256').write_text(hashlib.sha256(binary.read_bytes()).hexdigest()+'  test\n')
print('Old implementation counterexample failed as expected; isolated draft six modes passed. Runtime/all architectures/WAN not qualified.')
