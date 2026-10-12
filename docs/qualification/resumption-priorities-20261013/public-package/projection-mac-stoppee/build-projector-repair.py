from pathlib import Path
import shlex,json,hashlib,subprocess,time
E=Path(__file__).resolve().parent;Q=E.parent;R=Path('/Users/boris/Dev/blockchain-nano')
OLD='/build/coherent-e33ad89e-arm64';CORE='/build/publication-cancel-boundary-arm64-20261007/libnano_core.a';IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
SOURCE=E/'project-actual-stopped-store.cpp';assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()=='304946a4ca60f9eec4be65cbf0cf42818e90695060608f59d863dfb509a4c58b'
CMD=['docker','run','--rm','--pull=never','--platform','linux/arm64','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--cpus','1','--memory','2g','--env','TMPDIR=/outputs/tmp','--mount','type=volume,source=nano-clean-arm64-build-20261002,target=/build,readonly','--mount',f'type=bind,source={R},target=/work,readonly','--mount',f'type=bind,source={E},target=/draft,readonly','--mount',f'type=bind,source={E}/compile-type-repair/build,target=/outputs']
def run(name,args):
 cmd=CMD+['--entrypoint',args[0],IMAGE]+args[1:];(E/'compile-type-repair/receipts'/f'{name}.command.json').write_text(json.dumps({'args':cmd,'timeoutSeconds':120},indent=2)+'\n');start=time.monotonic()
 with (E/'compile-type-repair/receipts'/f'{name}.log').open('wb') as f:p=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,timeout=120)
 (E/'compile-type-repair/receipts'/f'{name}.exit.json').write_text(json.dumps({'exitCode':p.returncode,'elapsedSeconds':time.monotonic()-start},indent=2)+'\n');print(name,p.returncode,flush=True)
 if p.returncode:raise SystemExit(p.returncode)
paths=[CORE]+[OLD+'/'+x for x in ['third_party/mcl/bint64.o','third_party/mcl/base64.o','third_party/ckzg/libnano_ckzg.a','third_party/ckzg/libnano_blst.a','third_party/secp256k1/lib/libsecp256k1.a','lib/libmcl.a']]
run('parents-before',['/usr/bin/sha256sum']+paths)
F=Q/'isolated-recovery-tooling-cmake-20261012/current/CMakeFiles/nano-capella-isolated-stopped-capture.dir';flags={}
for line in (F/'flags.make').read_text().splitlines():
 if ' = ' in line:k,v=line.split(' = ',1);flags[k]=v
args=['/usr/bin/clang++']+shlex.split(flags['CXX_DEFINES'])+shlex.split(flags['CXX_INCLUDES'])+shlex.split(flags['CXX_FLAGS'])+['-c','/draft/project-actual-stopped-store.cpp','-o','/outputs/project-actual-stopped-store.o'];run('compile',args)
link=shlex.split((F/'link.txt').read_text());out=[]
for token in link:
 if token.startswith('CMakeFiles/nano-capella-isolated-stopped-capture.dir/') and token.endswith('.o'):token='/outputs/project-actual-stopped-store.o'
 elif token=='nano-capella-isolated-stopped-capture':token='/outputs/project-actual-stopped-store'
 elif token=='libnano_core.a':token=CORE
 elif token.endswith(('.a','.o')) and not token.startswith('/'):token=OLD+'/'+token
 out.append(token)
run('link',out);run('binary-sha256',['/usr/bin/sha256sum','/outputs/project-actual-stopped-store']);run('parents-after',['/usr/bin/sha256sum']+paths)
assert (E/'compile-type-repair/receipts/parents-before.log').read_bytes()==(E/'compile-type-repair/receipts/parents-after.log').read_bytes();assert (E/'compile-type-repair/receipts/parents-before.log').read_text().split()[0]=='53f4c33dcb68ca43fd054474b8cfa968f26b04f601f4a256996960bfa486a16b'
receipt={'format':'nano-pure-actual-stopped-store-projector-build-v1','passed':True,'sourceSha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),'binarySha256':(E/'compile-type-repair/receipts/binary-sha256.log').read_text().split()[0],'compileExitCode':0,'linkExitCode':0,'qualifiedCoreArchiveSha256':(E/'compile-type-repair/receipts/parents-before.log').read_text().split()[0],'reusedParentsUnchanged':True,'productionCoreOrDependenciesOrImageRebuilt':False,'compiledOnlyNoProjectionExecuted':True};(E/'compile-type-repair/receipts/build-terminal.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
