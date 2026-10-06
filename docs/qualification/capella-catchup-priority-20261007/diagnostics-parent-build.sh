set -eu
arch=$1
out=/build/relay-progress-diagnostics-$arch-20261007
mkdir -p "$out"
base=/build/coherent-e33ad89e-$arch/network/CMakeFiles/nano-p2p.dir
flags=$(sed -n -e 's/^CXX_DEFINES = //p' -e 's/^CXX_INCLUDES = //p' -e 's/^CXX_FLAGS = //p' "$base/flags.make" | tr '\n' ' ')
eval /usr/bin/clang++ "$flags" -c /work/network/main.cpp -o "$out/main.cpp.o"
sed -e "s@CMakeFiles/nano-p2p.dir/main.cpp.o@$out/main.cpp.o@" -e "s@-o nano-p2p@-o $out/nano-p2p@" "$base/link.txt" > "$out/link.sh"
sh "$out/link.sh"
sha256sum "$out/nano-p2p" /work/network/main.cpp /work/network/capella_block_delivery.hpp /work/network/capella_head_catalog_relay.hpp
