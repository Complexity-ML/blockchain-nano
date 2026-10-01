#!/bin/bash
set -euo pipefail
cd /build/evm-rpc-portable-20261001
mkdir -p /reports/scratch; export TMPDIR=/reports/scratch
sha256sum libnano_core.a lib/libmcl.a > /reports/libraries-before.sha256
flags='-DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 -I/work/include -I/work/third_party -I/work/third_party/mcl/include -O3 -DNDEBUG -std=gnu++20'
/usr/bin/clang++ $flags -c /work/src/types.cpp -o /reports/types.o > /reports/build.log 2>&1
/usr/bin/clang++ $flags -c /work/tests/test_core.cpp -o /reports/core.o >> /reports/build.log 2>&1
for target in nano-tests nano-rpc-tests nano-ethereum-state-runner; do
 sed -e "s@-o $target @-o /reports/$target @" -e 's@libnano_core.a@/reports/types.o libnano_core.a@' CMakeFiles/$target.dir/link.txt > /reports/link-$target.sh
 if [ "$target" = nano-tests ]; then sed -i 's@CMakeFiles/nano-tests.dir/tests/test_core.cpp.o@/reports/core.o@' /reports/link-$target.sh; fi
 bash /reports/link-$target.sh >> /reports/build.log 2>&1
done
sha256sum /reports/nano-tests /reports/nano-rpc-tests /reports/nano-ethereum-state-runner > /reports/executables.sha256
set +e
for target in nano-tests nano-rpc-tests; do timeout 120 /reports/$target > /reports/$target.log 2>&1; printf '%s\n' "$?" > /reports/$target.exit; done
timeout 180 /reports/nano-ethereum-state-runner /work/tests/fixtures/ethereum-official > /reports/eest.log 2>&1; printf '%s\n' "$?" > /reports/eest.exit
set -e
sha256sum libnano_core.a lib/libmcl.a > /reports/libraries-after.sha256
cmp /reports/libraries-before.sha256 /reports/libraries-after.sha256
