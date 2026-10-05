#!/bin/bash
set -eu
base=$1
parent=$2
out=$3
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
sha256sum "$parent/nano" "$parent/libnano_core.a" > "$out/parent-before.sha256"
sha256sum /work/src/consensus_deposit_history.cpp /work/tests/test_deposit_history.cpp /work/tests/capella/paused_capture.cpp /work/tests/test_capella_transition.cpp /work/src/main.cpp > "$out/sources.sha256"
flags(){ sed -n 's/^CXX_\(DEFINES\|INCLUDES\|FLAGS\) = / /p' "$base/CMakeFiles/$1.dir/flags.make" | tr '\n' ' '; }
compile(){
 local command="/usr/bin/clang++ $(flags "$1") -I/work/third_party/ckzg/blst/bindings -c $2 -o $3"
 printf '%s\n' "$command" > "$out/$4-command.txt"
 eval "$command" > "$out/$4.log" 2>&1
 echo 0 > "$out/$4-exit.txt"
}
link(){
 local template=$1 object=$2 output=$3 library=$4 label=$5
 local command
 command=$(cat "$base/CMakeFiles/$template.dir/link.txt")
 command=${command//CMakeFiles\/$template.dir\/tests\/test_capella_transition.cpp.o/$object}
 command=${command//CMakeFiles\/$template.dir\/src\/main.cpp.o/$object}
 command=${command//-o $template /-o $output }
 command=${command// libnano_core.a / $library }
 printf '%s\n' "$command" > "$out/$label-command.txt"
 (cd "$base"; eval "$command") > "$out/$label.log" 2>&1
 echo 0 > "$out/$label-exit.txt"
}
compile nano_core /work/src/consensus_deposit_history.cpp "$out/consensus_deposit_history.cpp.o" core-compile
cp -p "$parent/libnano_core.a" "$out/libnano_core.a"
/usr/bin/ar r "$out/libnano_core.a" "$out/consensus_deposit_history.cpp.o"
/usr/bin/ranlib "$out/libnano_core.a"
compile nano-capella-transition-tests /work/tests/test_deposit_history.cpp "$out/deposit-test.o" deposit-test-compile
link nano-capella-transition-tests "$out/deposit-test.o" "$out/baseline-deposit-test" "$parent/libnano_core.a" baseline-link
set +e
"$out/baseline-deposit-test" > "$out/baseline-regression.log" 2>&1
status=$?
set -e
echo "$status" > "$out/baseline-regression-exit.txt"
[ "$status" = 1 ]
grep -F 'deposit view swallowed typed duty deadline' "$out/baseline-regression.log" > "$out/baseline-reproduced.log"
link nano-capella-transition-tests "$out/deposit-test.o" "$out/nano-deposit-history-tests" "$out/libnano_core.a" fixed-link
"$out/nano-deposit-history-tests" > "$out/fixed-regression.log" 2>&1
echo 0 > "$out/fixed-regression-exit.txt"
compile nano /work/src/main.cpp "$out/nano.o" nano-compile
link nano "$out/nano.o" "$out/nano" "$out/libnano_core.a" nano-link
compile nano-capella-transition-tests /work/tests/test_capella_transition.cpp "$out/transition.o" transition-compile
link nano-capella-transition-tests "$out/transition.o" "$out/nano-capella-transition-tests" "$out/libnano_core.a" transition-link
for mode in configured-runtime scheduler; do
 "$out/nano-capella-transition-tests" "--$mode" > "$out/$mode.log" 2>&1
 echo 0 > "$out/$mode-exit.txt"
 cat "$out/$mode.log"
done
compile nano-capella-transition-tests /work/tests/capella/paused_capture.cpp "$out/capture.o" capture-compile
link nano-capella-transition-tests "$out/capture.o" "$out/nano-capella-paused-capture" "$out/libnano_core.a" capture-link
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
sha256sum "$out/nano" "$out/libnano_core.a" "$out/nano-deposit-history-tests" "$out/nano-capella-transition-tests" "$out/nano-capella-paused-capture" > "$out/programs.sha256"
printf '%s\n' 'scoped Docker deadline qualification completed'
