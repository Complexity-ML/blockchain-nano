set -euo pipefail
build=/base/snapshot-cache-row-2db1840c-arm64-20261002
flags=(-O2 -DNDEBUG -std=gnu++20 -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 -I/work/include -I/work/third_party -I/work/third_party/ckzg/src -I/work/third_party/ckzg/blst/bindings -I"$build/third_party/ckzg/include" -I/work/third_party/secp256k1/include -I/work/third_party/mcl/include)
libs=("$build/third_party/mcl/bint64.o" "$build/third_party/mcl/base64.o" "$build/libnano_core.a" "$build/third_party/ckzg/libnano_ckzg.a" "$build/third_party/ckzg/libnano_blst.a" "$build/third_party/secp256k1/lib/libsecp256k1.a" -lgmpxx -lgmp -lssl -lcrypto -licuuc -ldl -lrocksdb "$build/lib/libmcl.a")
sha256sum "${libs[@]:0:6}" > /reports/inputs-before.sha256
clang++ "${flags[@]}" /work/tests/test_capella_audit_ancestry.cpp -o /scratch/ancestry-tests "${libs[@]}"
/scratch/ancestry-tests > /reports/helper-tests.log
clang++ "${flags[@]}" -c /work/src/consensus_capella_audit.cpp -o /scratch/audit.o
clang++ "${flags[@]}" /control/driver.cpp /scratch/audit.o -o /scratch/audit-driver "${libs[@]}"
sha256sum /scratch/ancestry-tests /scratch/audit-driver > /reports/programs.sha256
cp /scratch/ancestry-tests /scratch/audit-driver /reports/
sha256sum "${libs[@]:0:6}" > /reports/inputs-after.sha256
cmp /reports/inputs-before.sha256 /reports/inputs-after.sha256
printf '0\n' > /reports/compile-exit.txt
