set -eu
check_cpp() {
 clang++ -O3 -DNDEBUG -std=gnu++20 -DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256 -DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1 '-DNANO_TEST_SOURCE="/work"' -I/work/include -I/work/third_party -I/work/third_party/mcl/include -fsyntax-only "$1"
}
check_cpp /work/tests/test_bft_network.cpp
check_cpp /work/tests/test_rpc_cost.cpp
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Werror /work/tests/test_rpc_load_window.cpp -o /tmp/window-controls
/tmp/window-controls
