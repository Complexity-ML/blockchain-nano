set -euo pipefail
original=/build/production-evm-c10426b2-20261001
mkdir -p /reports/scratch /reports/ctest
export TMPDIR=/reports/scratch
cd "$original"
python3 - <<'PROBE_BUILD'
from pathlib import Path
import shlex,subprocess,hashlib,json
root=Path('/build/network/CMakeFiles/nano-altair-probe.dir')
flags=(root/'flags.make').read_text();args=[]
for key in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']:
 value=next(line.split('=',1)[1].strip() for line in flags.splitlines() if line.startswith(key+' ='))
 args.extend(shlex.split(value))
args=['/usr/bin/clang++',*args,'-I/work/network','-c','/control/altair_probe.cpp','-o','/reports/probe.o']
link=shlex.split((root/'link.txt').read_text());objects=[i for i,x in enumerate(link) if x.endswith('/altair_probe.cpp.o')];assert len(objects)==1
link[objects[0]]='/reports/probe.o';link[link.index('-o')+1]='/reports/nano-altair-probe-diagnostic'
libs=[x for x in link if x.startswith('/') and Path(x).is_file() and x!=link[0] and x!='/reports/probe.o']
Path('/reports/probe-build-commands.json').write_text(json.dumps(dict(compile=args,link=link),indent=2)+'\n')
Path('/reports/probe-link-input-paths.json').write_text(json.dumps(libs)+'\n')
with open('/reports/probe-compile.log','w') as f:subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,check=True)
with open('/reports/probe-link.log','w') as f:subprocess.run(link,stdout=f,stderr=subprocess.STDOUT,check=True)
PROBE_BUILD
/usr/bin/clang++ -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 '-DNANO_TEST_SOURCE="/work"' -I/work/tests -I/work/include -I/work/third_party -I/work/third_party/mcl/include -O3 -DNDEBUG -std=gnu++20 -c /control/test_altair_p2p.cpp -o /reports/diagnostic.o > /reports/compile.log 2>&1
/usr/bin/clang++ -O3 -DNDEBUG /reports/diagnostic.o -o /reports/nano-altair-p2p-diagnostic third_party/mcl/bint64.o third_party/mcl/base64.o libnano_core.a third_party/ckzg/libnano_ckzg.a third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a /usr/lib/x86_64-linux-gnu/libgmpxx.so /usr/lib/x86_64-linux-gnu/libgmp.so /usr/lib/x86_64-linux-gnu/libssl.so /usr/lib/x86_64-linux-gnu/libcrypto.so /usr/lib/x86_64-linux-gnu/libicuuc.so -ldl /usr/lib/x86_64-linux-gnu/librocksdb.so lib/libmcl.a > /reports/link.log 2>&1
cat > /reports/ctest/CTestTestfile.cmake <<'CMAKE'
add_test(consensus_altair_libp2p /reports/nano-altair-p2p-diagnostic /build/network/nano-p2p /reports/nano-altair-probe-diagnostic)
set_tests_properties(consensus_altair_libp2p PROPERTIES TIMEOUT 120 LABELS "qualification_consensus_ethereum")
CMAKE
ctest --test-dir /reports/ctest --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$original/nano" -P /control/manifest-tool.cmake
sha256sum libnano_core.a third_party/mcl/bint64.o third_party/mcl/base64.o third_party/ckzg/libnano_ckzg.a third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a lib/libmcl.a > /reports/link-inputs-before.txt
python3 - <<'PROBE_HASH'
from pathlib import Path
import hashlib,json
paths=json.loads(Path('/reports/probe-link-input-paths.json').read_text())
Path('/reports/probe-link-inputs-before.json').write_text(json.dumps({x:hashlib.sha256(Path(x).read_bytes()).hexdigest() for x in paths},sort_keys=True)+'\n')
PROBE_HASH
set +e
ctest --test-dir /reports/ctest --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
test_exit=$?
printf '%s\n' "$test_exit" > /reports/ctest-exit.txt
cp /reports/ctest/Testing/Temporary/LastTest.log /reports/LastTest.log
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-after-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$original/nano" -P /control/manifest-tool.cmake
manifest_exit=$?
printf '%s\n' "$manifest_exit" > /reports/post-test-manifest-exit.txt
set -e
sha256sum libnano_core.a third_party/mcl/bint64.o third_party/mcl/base64.o third_party/ckzg/libnano_ckzg.a third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a lib/libmcl.a > /reports/link-inputs-after.txt
python3 - <<'PROBE_HASH'
from pathlib import Path
import hashlib,json
paths=json.loads(Path('/reports/probe-link-input-paths.json').read_text())
Path('/reports/probe-link-inputs-after.json').write_text(json.dumps({x:hashlib.sha256(Path(x).read_bytes()).hexdigest() for x in paths},sort_keys=True)+'\n')
PROBE_HASH
cmp /reports/probe-link-inputs-before.json /reports/probe-link-inputs-after.json
cmp /reports/link-inputs-before.txt /reports/link-inputs-after.txt
cmp /reports/executables-before-tests.json /reports/executables-after-tests.json
test "$manifest_exit" = 0
exit "$test_exit"
