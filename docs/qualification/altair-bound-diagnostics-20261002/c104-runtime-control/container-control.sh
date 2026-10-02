set -euo pipefail
original=/build/production-evm-c10426b2-20261001
mkdir -p /reports/scratch /reports/ctest
export TMPDIR=/reports/scratch
cd "$original"
/usr/bin/clang++ -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 '-DNANO_TEST_SOURCE="/work"' -I/work/tests -I/work/include -I/work/third_party -I/work/third_party/mcl/include -O3 -DNDEBUG -std=gnu++20 -c /control/test_altair_p2p.cpp -o /reports/diagnostic.o > /reports/compile.log 2>&1
/usr/bin/clang++ -O3 -DNDEBUG /reports/diagnostic.o -o /reports/nano-altair-p2p-diagnostic third_party/mcl/bint64.o third_party/mcl/base64.o libnano_core.a third_party/ckzg/libnano_ckzg.a third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a /usr/lib/x86_64-linux-gnu/libgmpxx.so /usr/lib/x86_64-linux-gnu/libgmp.so /usr/lib/x86_64-linux-gnu/libssl.so /usr/lib/x86_64-linux-gnu/libcrypto.so /usr/lib/x86_64-linux-gnu/libicuuc.so -ldl /usr/lib/x86_64-linux-gnu/librocksdb.so lib/libmcl.a > /reports/link.log 2>&1
cat > /reports/ctest/CTestTestfile.cmake <<'CMAKE'
add_test(consensus_altair_libp2p /reports/nano-altair-p2p-diagnostic /build/network/nano-p2p /build/network/nano-altair-probe)
set_tests_properties(consensus_altair_libp2p PROPERTIES TIMEOUT 120 LABELS "qualification_consensus_ethereum")
CMAKE
ctest --test-dir /reports/ctest --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$original/nano" -P /control/manifest-tool.cmake
sha256sum libnano_core.a third_party/mcl/bint64.o third_party/mcl/base64.o third_party/ckzg/libnano_ckzg.a third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a lib/libmcl.a > /reports/link-inputs-before.txt
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
cmp /reports/link-inputs-before.txt /reports/link-inputs-after.txt
cmp /reports/executables-before-tests.json /reports/executables-after-tests.json
test "$manifest_exit" = 0
exit "$test_exit"
