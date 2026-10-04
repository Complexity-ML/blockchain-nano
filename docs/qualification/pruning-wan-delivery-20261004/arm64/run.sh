set -u
cd /build/finalized-retention-4734f285-arm64
clang++ -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 -I/work/include -I/work/third_party -I/work/third_party/mcl/include -O3 -DNDEBUG -std=gnu++20 -c /work/src/main.cpp -o /build/pruning-wan-092a153c-arm64/main.cpp.o > /report/compile.log 2>&1
status=$?
printf "%s\n" "$status" > /report/compile-exit.txt
if [ "$status" -ne 0 ]; then exit "$status"; fi
/usr/bin/clang++ -O3 -DNDEBUG /build/pruning-wan-092a153c-arm64/main.cpp.o -o /build/pruning-wan-092a153c-arm64/nano third_party/mcl/bint64.o third_party/mcl/base64.o libnano_core.a third_party/ckzg/libnano_ckzg.a third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a /usr/lib/aarch64-linux-gnu/libgmpxx.so /usr/lib/aarch64-linux-gnu/libgmp.so /usr/lib/aarch64-linux-gnu/libssl.so /usr/lib/aarch64-linux-gnu/libcrypto.so /usr/lib/aarch64-linux-gnu/libicuuc.so -ldl /usr/lib/aarch64-linux-gnu/librocksdb.so lib/libmcl.a > /report/link.log 2>&1
status=$?
printf "%s\n" "$status" > /report/link-exit.txt
if [ "$status" -ne 0 ]; then exit "$status"; fi
set -e
mkdir /build/pruning-wan-092a153c-arm64/network
cp /build/coherent-efbeb9bc-arm64/network/nano-p2p /build/pruning-wan-092a153c-arm64/network/nano-p2p
printf "%s\n" 092a153ca1d203434beb14fe2102e3a092dc454c > /build/pruning-wan-092a153c-arm64/production-source-revision.txt
sha256sum /build/pruning-wan-092a153c-arm64/nano /build/pruning-wan-092a153c-arm64/network/nano-p2p > /report/programs.txt
