set -u
cd /build/finalized-retention-4734f285-amd64
mkdir -p /build/pruning-recovery-94535766-amd64/fixture-scratch
export TMPDIR=/build/pruning-recovery-94535766-amd64/fixture-scratch
clang++ -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 -I/work/include -I/work/third_party -I/work/third_party/mcl/include -O3 -DNDEBUG -std=gnu++20 -c /work/tests/test_execution_checkpoint.cpp -o /build/pruning-recovery-94535766-amd64/test_execution_checkpoint.cpp.o > /report/storage-compile.log 2>&1
status=$?
printf "%s\n" "$status" > /report/storage-compile-exit.txt
if [ "$status" -ne 0 ]; then exit "$status"; fi
/usr/bin/clang++ -O3 -DNDEBUG /build/pruning-recovery-94535766-amd64/test_execution_checkpoint.cpp.o -o /build/pruning-recovery-94535766-amd64/nano-execution-checkpoint-tests third_party/mcl/bint64.o third_party/mcl/base64.o libnano_core.a third_party/ckzg/libnano_ckzg.a third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a /usr/lib/x86_64-linux-gnu/libgmpxx.so /usr/lib/x86_64-linux-gnu/libgmp.so /usr/lib/x86_64-linux-gnu/libssl.so /usr/lib/x86_64-linux-gnu/libcrypto.so /usr/lib/x86_64-linux-gnu/libicuuc.so -ldl /usr/lib/x86_64-linux-gnu/librocksdb.so lib/libmcl.a > /report/storage-link.log 2>&1
status=$?
printf "%s\n" "$status" > /report/storage-link-exit.txt
if [ "$status" -ne 0 ]; then exit "$status"; fi
timeout 120 /build/pruning-recovery-94535766-amd64/nano-execution-checkpoint-tests --pruning-restore > /report/storage-test.log 2>&1
status=$?
printf "%s\n" "$status" > /report/storage-test-exit.txt
if [ "$status" -ne 0 ]; then exit "$status"; fi
sha256sum /build/pruning-recovery-94535766-amd64/nano-execution-checkpoint-tests > /report/storage-program.txt
