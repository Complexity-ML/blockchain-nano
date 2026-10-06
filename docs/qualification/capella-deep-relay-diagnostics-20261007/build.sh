set -eu
base=/build/coherent-e33ad89e-arm64/network/CMakeFiles/nano-capella-dependency-preparation-tests.dir
flags=$(sed -n -e 's/^CXX_DEFINES = //p' -e 's/^CXX_INCLUDES = //p' -e 's/^CXX_FLAGS = //p' "$base/flags.make" | tr '\n' ' ')
eval /usr/bin/clang++ "$flags" -I/work/network -c /probe/test.cpp -o /probe/test.o
sed -e 's@"CMakeFiles/nano-capella-dependency-preparation-tests.dir/test_capella_dependency_preparation.cpp.o"@/probe/test.o@' -e 's@-o nano-capella-dependency-preparation-tests@-o /probe/test@' "$base/link.txt" > /probe/link.sh
sh /probe/link.sh
