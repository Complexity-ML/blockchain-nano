set -euo pipefail
arch=$1
build=/build/full-coherent-0f53b42d-$arch-20261002
export TMPDIR=/build/full-coherent-0f53b42d-$arch-tmp-20261002
test ! -e "$build";mkdir "$TMPDIR"
cmake -S /work -B "$build" -DCMAKE_BUILD_TYPE=Release -DNANO_NETWORK_EXECUTABLE=/build/network/nano-p2p > /reports/configure.log 2>&1
cmake --build "$build" --parallel 2 > /reports/build.log 2>&1
printf '0\n' > /reports/build-exit.txt
ctest --test-dir "$build" --show-only=json-v1 > /reports/registry.json
cmake -DREGISTRY=/reports/registry.json -P /control/registry.cmake
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/programs-before.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /control/manifest-tool.cmake
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/link-inputs-before.sha256
set +e
ctest --test-dir "$build" --no-tests=error --parallel 1 --output-on-failure --output-junit /reports/results.xml > /reports/tests.log 2>&1
code=$?
set -e
printf '%s\n' "$code" > /reports/ctest-exit.txt
cmake -DREGISTRY=/reports/registry.json -DOUTPUT=/reports/programs-after.json -DNETWORK_EXECUTABLE=/build/network/nano-p2p -DNODE_EXECUTABLE="$build/nano" -P /control/manifest-tool.cmake
find "$build" -type f \( -name '*.a' -o -name '*.so' \) -print0 | sort -z | xargs -0 sha256sum > /reports/link-inputs-after.sha256
cmp /reports/programs-before.json /reports/programs-after.json
cmp /reports/link-inputs-before.sha256 /reports/link-inputs-after.sha256
cp "$build/Testing/Temporary/LastTest.log" /reports/LastTest.log
exit "$code"
