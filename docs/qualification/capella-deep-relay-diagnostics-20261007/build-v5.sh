set -eu
base=/build/coherent-e33ad89e-arm64/network/CMakeFiles/nano-capella-dependency-preparation-tests.dir
flags=$(sed -n -e 's/^CXX_DEFINES = //p' -e 's/^CXX_INCLUDES = //p' -e 's/^CXX_FLAGS = //p' "$base/flags.make" | tr '\n' ' ')
eval /usr/bin/clang++ "$flags" -I/probe/overlay -I/work/network -c /probe/test-v5.cpp -o /probe/test-v5.o
sed -e 's@"CMakeFiles/nano-capella-dependency-preparation-tests.dir/test_capella_dependency_preparation.cpp.o"@/probe/test-v5.o@' -e 's@-o nano-capella-dependency-preparation-tests@-o /probe/test-v5@' "$base/link.txt" > /probe/link-v5.sh
sh /probe/link-v5.sh
