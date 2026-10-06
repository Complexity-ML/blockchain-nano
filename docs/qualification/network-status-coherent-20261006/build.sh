#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
parent=/build/catalog-selection-$arch-20261006
out=/build/network-status-coherent-$arch-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/nano_core.dir/flags.make" | tr '\n' ' ')
extra='-I/work/include -I/work/third_party/secp256k1/include -I/work/third_party/ckzg/blst/bindings'
sha256sum "$parent/libnano_core.a" > "$out/parent-before.sha256"
mkdir "$out/objects" "$out/dependencies"
# Re-scan every current core translation unit. Only objects proven independent
# of the changed class layout reuse the qualified parent archive.
for source in /work/src/*.cpp; do
 stem=$(basename "$source")
 [ "$stem" != main.cpp ] || continue
 command="/usr/bin/clang++ $extra $flags -MM -MT $stem -MF $out/dependencies/$stem.d $source"
 printf '%s\n' "$command" >> "$out/dependency-commands.txt"
 eval "$command" >> "$out/dependency-scan.log" 2>&1
 if grep -Eq 'consensus_capella_reactor.hpp|altair_worker.hpp|altair_work.hpp|capella_consensus_service.hpp|simulation_queue.hpp|capella_inventory_sessions.hpp' "$out/dependencies/$stem.d"; then
  printf '%s\n' "$stem" >> "$out/rebuilt-sources.txt"
  command="/usr/bin/clang++ $extra $flags -c $source -o $out/objects/$stem.o"
  printf '%s\n' "$command" >> "$out/compile-commands.txt"
  eval "$command" > "$out/objects/$stem.log" 2>&1
  echo 0 > "$out/objects/$stem.exit"
  echo "rebuilt $stem"
 else printf '%s\n' "$stem" >> "$out/reused-sources.txt"; fi
done
cp -p "$parent/libnano_core.a" "$out/libnano_core.a"
ar r "$out/libnano_core.a" "$out"/objects/*.o
ranlib "$out/libnano_core.a"
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum /work/include/nano/consensus_capella_reactor.hpp /work/src/consensus_capella_reactor.cpp "$out/libnano_core.a" > "$out/programs.sha256"
