#!/bin/bash
set -eu
b=/build/full-coherent-0f53b42d-amd64-20261002
cd "$b"
inputs=(third_party/mcl/bint64.o third_party/mcl/base64.o libnano_core.a third_party/ckzg/libnano_ckzg.a third_party/ckzg/libnano_blst.a third_party/secp256k1/lib/libsecp256k1.a /usr/lib/x86_64-linux-gnu/libgmpxx.so /usr/lib/x86_64-linux-gnu/libgmp.so /usr/lib/x86_64-linux-gnu/libssl.so /usr/lib/x86_64-linux-gnu/libcrypto.so /usr/lib/x86_64-linux-gnu/libicuuc.so /usr/lib/x86_64-linux-gnu/librocksdb.so lib/libmcl.a /build/network/nano-p2p /build/network/nano-altair-probe)
sha256sum "${inputs[@]}" > /reports/inputs-before.sha256
bash /reports/network-build.sh
if [ "$(cat /reports/network-build-exit.txt)" != 0 ]; then exit 1; fi
set +e
timeout -s KILL 180 /usr/bin/clang++ -O3 -DNDEBUG -std=gnu++20 -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 '-DNANO_TEST_SOURCE="/diag"' -I/diag/include -I/diag/third_party -I/diag/third_party/mcl/include /diag/tests/test_altair_p2p.cpp -o /reports/nano-altair-p2p-diagnostic "${inputs[@]:0:13}" -ldl > /reports/build.log 2>&1
code=$?
set -e
printf '%s\n' "$code" > /reports/build-exit.txt
if [ "$code" -ne 0 ]; then exit "$code"; fi
sha256sum /reports/nano-altair-p2p-diagnostic > /reports/diagnostic-before.sha256
set +e
start=$(date +%s)
timeout -s KILL 120 /reports/nano-altair-p2p-diagnostic /reports/nano-p2p-diagnostic /build/network/nano-altair-probe > /reports/test.stdout 2> /reports/test.stderr
code=$?
set -e
printf '%s\n' "$code" > /reports/test-exit.txt
printf '%s\n' "$(($(date +%s)-start))" > /reports/test-seconds.txt
sha256sum /reports/nano-altair-p2p-diagnostic > /reports/diagnostic-after.sha256
sha256sum "${inputs[@]}" > /reports/inputs-after.sha256
cmp /reports/inputs-before.sha256 /reports/inputs-after.sha256
cmp /reports/diagnostic-before.sha256 /reports/diagnostic-after.sha256
printf '0\n' > /reports/input-compare-exit.txt
exit "$code"
