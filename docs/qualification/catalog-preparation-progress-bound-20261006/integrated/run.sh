#!/bin/bash
set -eu
arch=$1
if [ "$arch" = native ];then
 out=/private/tmp/nano-catalog-busy-progress-integrated-20261006
 sdk=/private/tmp/nano-catalog-busy-integrated-native-20261006/network
 headers=/private/tmp/nano-catalog-busy-integrated-20261006/source/network
else
 out=/progress
 sdk=/build/catalog-busy-integrated-$arch-20261006/network
 headers=/drivers/source/network
fi
proof="$out/$arch";mkdir -p "$proof"
trap 'echo $? > "$proof/controller-exit.txt"' EXIT
flags=$(sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$sdk/CMakeFiles/nano-capella-dependency-preparation-tests.dir/flags.make" | tr '\n' ' ')
command="/usr/bin/clang++ -I$headers $flags -c $out/test.cpp -o $proof/test.o"
printf '%s\n' "$command" > "$proof/compile-command.txt"
eval "$command" > "$proof/compile.log" 2>&1
link=$(cat "$sdk/CMakeFiles/nano-capella-dependency-preparation-tests.dir/link.txt")
link=${link//CMakeFiles\/nano-capella-dependency-preparation-tests.dir\/test_capella_dependency_preparation.cpp.o/$proof\/test.o}
link=${link//-o nano-capella-dependency-preparation-tests /-o $proof\/test }
printf '%s\n' "$link" > "$proof/link-command.txt"
(cd "$sdk";eval "$link") > "$proof/link.log" 2>&1
for mode in normal duplicate unavailable unavailable-anchor refused lost cancel slow slow-duplicate slow-one-step;do
 set +e
 "$proof/test" "$mode" > "$proof/$mode.log" 2>&1
 code=$?
 set -e
 echo "$code" > "$proof/$mode-exit.txt"
 tail -2 "$proof/$mode.log"
 if [ "$code" -ne 0 ];then exit "$code";fi
done
if command -v sha256sum >/dev/null;then sha256sum "$proof/test" "$out/test.cpp";else shasum -a 256 "$proof/test" "$out/test.cpp";fi > "$proof/program-source.sha256"
