from pathlib import Path
import subprocess,shlex,json,os,time
root=Path(__file__).parent;out=root/'native-build';old=Path('/private/tmp/nano-coherent-e33ad89e-20261006/native-build');parent=Path('/private/tmp/nano-runtime-catalog-preflight-20261006/native-build')
args=shlex.split((old/'CMakeFiles/nano-capella-transition-tests.dir/link.txt').read_text());args=[str(parent/'libnano_core.a') if a=='libnano_core.a' else str(out/'fixture.o') if a.endswith('/tests/test_capella_transition.cpp.o') else a for a in args];args[args.index('-o')+1]=str(out/'baseline-test')
subprocess.run(args,cwd=old,check=True,stdout=(out/'link-baseline.log').open('w'),stderr=subprocess.STDOUT)
env=dict(os.environ,NANO_TEST_COLD_RUNTIME='1',NANO_TEST_RUNTIME_REFERENCE_MEMO='1',NANO_TEST_DURABLE_CANONICAL_MEMO='1')
for name,binary,expected in [('baseline',out/'baseline-test',1),('fixed',out/'test',0)]:
 start=time.monotonic()
 with (out/(name+'-runtime.log')).open('w') as log:code=subprocess.run([str(binary),'--configured-runtime'],env=env,stdout=log,stderr=subprocess.STDOUT).returncode
 receipt={'exitCode':code,'elapsedSeconds':time.monotonic()-start,'expectedExitCode':expected};(out/(name+'-runtime-exit.json')).write_text(json.dumps(receipt)+'\n');print(name,receipt,flush=True)
 text=(out/(name+'-runtime.log')).read_text()
 if name=='baseline':assert code!=0 and 'durable wrapper canonical check revalidated immutable cold rows' in text
 else:assert code==0 and 'durable-wrapper-canonical-memo' in text
