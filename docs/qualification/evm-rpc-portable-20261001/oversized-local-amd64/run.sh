#!/bin/bash
set -euo pipefail
build=/build/evm-rpc-portable-20261001
mkdir -p /reports/scratch; export TMPDIR=/reports/scratch
cd "$build"
sha256sum libnano_core.a lib/libmcl.a third_party/mcl/bint64.o third_party/mcl/base64.o > /reports/libraries-before.sha256
/usr/bin/clang++ -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 -I/work/include -I/work/third_party -I/work/third_party/mcl/include -O3 -DNDEBUG -std=gnu++20 -c /work/tests/test_oversized_block.cpp -o /reports/test.o > /reports/build.log 2>&1
sed -e 's@CMakeFiles/nano-oversized-block-tests.dir/tests/test_oversized_block.cpp.o@/reports/test.o@g' -e 's@-o nano-oversized-block-tests @-o /reports/nano-oversized-block-tests @' CMakeFiles/nano-oversized-block-tests.dir/link.txt > /reports/link-command.txt
bash /reports/link-command.txt

sha256sum /reports/nano-oversized-block-tests > /reports/executable.sha256
set +e
for fork in Paris Shanghai; do NANO_TEST_EVM_FORK=$fork timeout 120 /reports/nano-oversized-block-tests > /reports/$fork.log 2>&1; code=$?; printf '%s\n' "$code" > /reports/$fork.exit; done
set -e
sha256sum libnano_core.a lib/libmcl.a third_party/mcl/bint64.o third_party/mcl/base64.o > /reports/libraries-after.sha256
cmp /reports/libraries-before.sha256 /reports/libraries-after.sha256
