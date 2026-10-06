#!/bin/bash
set -eu
arch=$1
base=/build/coherent-efbeb9bc-$arch
parent=/build/deposit-deadline-$arch-b5ee00ac-20261005
core=/build/incoming-alternation-$arch-20261006/libnano_core.a
out=/build/incoming-runtime-$arch-20261006
mkdir "$out"
trap 'status=$?; echo "$status" > "$out/controller-exit.txt"' EXIT
sha256sum "$parent/nano" "$parent/nano-capella-paused-capture" "$parent/libnano_core.a" "$core" > "$out/input-before.sha256"
for kind in nano capture;do
 template=nano;object="$parent/nano.o";program=nano
 if [ "$kind" = capture ];then template=nano-capella-transition-tests;object="$parent/capture.o";program=nano-capella-paused-capture;fi
 command=$(cat "$base/CMakeFiles/$template.dir/link.txt")
 command=${command//CMakeFiles\/$template.dir\/src\/main.cpp.o/$object}
 command=${command//CMakeFiles\/$template.dir\/tests\/test_capella_transition.cpp.o/$object}
 command=${command//-o $template /-o $out\/$program }
 command=${command// libnano_core.a / $core }
 printf '%s\n' "$command" > "$out/$kind-link-command.txt"
 (cd "$base";eval "$command") > "$out/$kind-link.log" 2>&1
 echo 0 > "$out/$kind-link-exit.txt"
done
"$out/nano" --help > "$out/nano-help.log" 2>&1
echo 0 > "$out/nano-help-exit.txt"
sha256sum -c "$out/input-before.sha256" > "$out/inputs-unchanged.log"
sha256sum "$out/nano" "$out/nano-capella-paused-capture" > "$out/programs.sha256"
