set -eu
base=/build/coherent-e33ad89e-arm64/network/CMakeFiles/nano-capella-dependency-preparation-tests.dir
flags=$(sed -n -e 's/^CXX_DEFINES = //p' -e 's/^CXX_INCLUDES = //p' -e 's/^CXX_FLAGS = //p' "$base/flags.make" | tr '\n' ' ')
for name in block_delivery signed_anchor attestation_delivery head_catalog_relay depth_catchup export_resume head_history_fairness dependency_preparation; do
 cp "/work/network/test_capella_$name.cpp" "/probe/regression-$name.cpp"
 eval /usr/bin/clang++ "$flags" -DNANO_HEAD_HISTORY_FIRST -I/probe/overlay -I/work/network -c "/probe/regression-$name.cpp" -o "/probe/regression-$name.o"
 sed -e "s@\"CMakeFiles/nano-capella-dependency-preparation-tests.dir/test_capella_dependency_preparation.cpp.o\"@/probe/regression-$name.o@" -e "s@-o nano-capella-dependency-preparation-tests@-o /probe/regression-$name@" "$base/link.txt" > "/probe/link-$name.sh"
 sh "/probe/link-$name.sh"
 if [ "$name" = dependency_preparation ]; then
  for scenario in normal duplicate unavailable unavailable-anchor refused lost cancel slow slow-duplicate slow-one-step; do "/probe/regression-$name" "$scenario"; done
 else "/probe/regression-$name"; fi
 printf '%s passed\n' "$name"
done
