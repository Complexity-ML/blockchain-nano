set -euo pipefail
build=/base/snapshot-cache-row-2db1840c-arm64-20261002
flags=(-O2 -DNDEBUG -std=gnu++20 -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 -I/work/include -I/work/third_party -I/work/third_party/ckzg/src -I/work/third_party/ckzg/blst/bindings -I"$build/third_party/ckzg/include" -I/work/third_party/secp256k1/include -I/work/third_party/mcl/include)
libs=("$build/third_party/mcl/bint64.o" "$build/third_party/mcl/base64.o" "$build/libnano_core.a" "$build/third_party/ckzg/libnano_ckzg.a" "$build/third_party/ckzg/libnano_blst.a" "$build/third_party/secp256k1/lib/libsecp256k1.a" -lgmpxx -lgmp -lssl -lcrypto -licuuc -ldl -lrocksdb "$build/lib/libmcl.a")
inputs=("$build/libnano_core.a" "$build/third_party/mcl/bint64.o" "$build/third_party/mcl/base64.o" "$build/third_party/ckzg/libnano_ckzg.a" "$build/third_party/ckzg/libnano_blst.a" "$build/third_party/secp256k1/lib/libsecp256k1.a" "$build/lib/libmcl.a")
sha256sum "${inputs[@]}" > /reports/inputs-before.sha256
clang++ "${flags[@]}" -c /work/src/consensus_capella_audit.cpp -o /scratch/audit.o
clang++ "${flags[@]}" -c /work/src/consensus_capella_store_snapshot.cpp -o /scratch/snapshot.o
clang++ "${flags[@]}" '-DNANO_TEST_SOURCE="/work"' /work/tests/test_capella_transition.cpp /scratch/snapshot.o /scratch/audit.o -o /scratch/transition-tests "${libs[@]}"
sha256sum /scratch/transition-tests > /reports/programs-before.sha256
set +e
timeout 120 /scratch/transition-tests > /reports/transition-tests.log 2>&1
run_exit=$?
set -e
printf '%s\n' "$run_exit" > /reports/transition-exit.txt
sha256sum /scratch/transition-tests > /reports/programs-after.sha256
sha256sum "${inputs[@]}" > /reports/inputs-after.sha256
cmp /reports/inputs-before.sha256 /reports/inputs-after.sha256
cmp /reports/programs-before.sha256 /reports/programs-after.sha256
exit "$run_exit"
