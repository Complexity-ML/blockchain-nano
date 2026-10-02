set -euo pipefail
original=/build/corrective-1ac57e25-20261002
mkdir -p /reports/scratch /reports/ctest
export TMPDIR=/reports/scratch
cd "$original"
/usr/bin/clang++ -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 -I/work/src -I/work/include -I/work/third_party -I/work/third_party/ckzg/src -I/work/third_party/ckzg/blst/bindings -I/build/corrective-1ac57e25-20261002/third_party/ckzg/include -I/work/third_party/secp256k1/include -I/work/third_party/mcl/include -O3 -DNDEBUG -std=gnu++20 -Wall -Wextra -Wpedantic -c /work/src/consensus_bellatrix_reactor.cpp -o /reports/reactor.o > /reports/reactor-compile.log 2>&1
/usr/bin/clang++ -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 -I/work/src -I/work/include -I/work/third_party -I/work/third_party/ckzg/src -I/work/third_party/ckzg/blst/bindings -I/build/corrective-1ac57e25-20261002/third_party/ckzg/include -I/work/third_party/secp256k1/include -I/work/third_party/mcl/include -O3 -DNDEBUG -std=gnu++20 -Wall -Wextra -Wpedantic -c /work/src/consensus_bellatrix_store_snapshot.cpp -o /reports/store-snapshot.o > /reports/store-snapshot-compile.log 2>&1
/usr/bin/clang++ -O3 -DNDEBUG CMakeFiles/nano-bellatrix-transition-tests.dir/tests/test_bellatrix_transition.cpp.o /reports/reactor.o /reports/store-snapshot.o -o /reports/nano-bellatrix-candidate -Wl,-rpath,/build/corrective-1ac57e25-20261002 third_party/mcl/bint64.o third_party/mcl/base64.o libnano_core.a third_party/ckzg/libnano_ckzg.a libnano-test-sync-fault.so third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a /usr/lib/x86_64-linux-gnu/libgmpxx.so /usr/lib/x86_64-linux-gnu/libgmp.so /usr/lib/x86_64-linux-gnu/libssl.so /usr/lib/x86_64-linux-gnu/libcrypto.so /usr/lib/x86_64-linux-gnu/libicuuc.so -ldl /usr/lib/x86_64-linux-gnu/librocksdb.so lib/libmcl.a > /reports/link.log 2>&1
/usr/bin/clang++ -O3 -DNDEBUG -std=gnu++20 -I/work/src -I/work/third_party /work/tests/test_checkpoint_json.cpp -o /reports/nano-checkpoint-json-tests > /reports/canonical-compile.log 2>&1
cat > /reports/ctest/CTestTestfile.cmake <<'CMAKE'
add_test(consensus_checkpoint_json_canonical /reports/nano-checkpoint-json-tests)
set_tests_properties(consensus_checkpoint_json_canonical PROPERTIES TIMEOUT 30 LABELS "qualification_consensus_ethereum")
add_test(consensus_bellatrix_witness_runtime /reports/nano-bellatrix-candidate --runtime-recovery)
set_tests_properties(consensus_bellatrix_witness_runtime PROPERTIES TIMEOUT 180 LABELS "qualification_consensus_ethereum")
add_test(consensus_bellatrix_local_slashing /reports/nano-bellatrix-candidate --runtime-slashing)
set_tests_properties(consensus_bellatrix_local_slashing PROPERTIES TIMEOUT 180 LABELS "qualification_consensus_ethereum")
add_test(consensus_bellatrix_voluntary_exit /reports/nano-bellatrix-candidate --exit-loop)
set_tests_properties(consensus_bellatrix_voluntary_exit PROPERTIES TIMEOUT 180 LABELS "qualification_consensus_ethereum")
add_test(consensus_bellatrix_deposit_store /reports/nano-bellatrix-candidate --deposit-store)
set_tests_properties(consensus_bellatrix_deposit_store PROPERTIES TIMEOUT 180 LABELS "qualification_consensus_ethereum")
add_test(consensus_bellatrix_clock_coalescing /reports/nano-bellatrix-candidate --clock-coalescing)
set_tests_properties(consensus_bellatrix_clock_coalescing PROPERTIES TIMEOUT 90 LABELS "qualification_consensus_ethereum")
add_test(consensus_bellatrix_beacon_headers_reorg /reports/nano-bellatrix-candidate --beacon-headers-reorg)
set_tests_properties(consensus_bellatrix_beacon_headers_reorg PROPERTIES TIMEOUT 90 LABELS "qualification_consensus_ethereum")
add_test(consensus_bellatrix_partition_simulation /reports/nano-bellatrix-candidate --partition-simulation)
set_tests_properties(consensus_bellatrix_partition_simulation PROPERTIES TIMEOUT 240 LABELS "qualification_consensus_ethereum")
CMAKE
ctest --test-dir /reports/ctest --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-before-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$original/nano" -P /control/manifest-tool.cmake
sha256sum CMakeFiles/nano-bellatrix-transition-tests.dir/tests/test_bellatrix_transition.cpp.o libnano_core.a third_party/mcl/bint64.o third_party/mcl/base64.o third_party/ckzg/libnano_ckzg.a libnano-test-sync-fault.so third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a lib/libmcl.a > /reports/link-inputs-before.txt
set +e
ctest --test-dir /reports/ctest --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
test_exit=$?
printf '%s\n' "$test_exit" > /reports/ctest-exit.txt
cp /reports/ctest/Testing/Temporary/LastTest.log /reports/LastTest.log
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/executables-after-tests.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$original/nano" -P /control/manifest-tool.cmake
manifest_exit=$?
printf '%s\n' "$manifest_exit" > /reports/post-test-manifest-exit.txt
set -e
sha256sum CMakeFiles/nano-bellatrix-transition-tests.dir/tests/test_bellatrix_transition.cpp.o libnano_core.a third_party/mcl/bint64.o third_party/mcl/base64.o third_party/ckzg/libnano_ckzg.a libnano-test-sync-fault.so third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a lib/libmcl.a > /reports/link-inputs-after.txt
cmp /reports/link-inputs-before.txt /reports/link-inputs-after.txt
cmp /reports/executables-before-tests.json /reports/executables-after-tests.json
test "$manifest_exit" = 0
exit "$test_exit"
