#!/bin/bash
set -eu
out=$1
scope=$2
trap 'status=$?; echo "$status" > "$out/tail-verification-exit.txt"' EXIT
for target in nano-capella-depth-catchup-tests nano-capella-head-catalog-tests nano-capella-catalog-relay-tests nano-capella-block-delivery-tests nano-p2p;do
 if [ "$scope" = arm64 ] && [ "$target" != nano-p2p ] && [ "$target" != nano-capella-depth-catchup-tests ];then continue;fi
 [ "$(cat "$out/$target-compile-exit.txt")" = 0 ]
 [ "$(cat "$out/$target-link-exit.txt")" = 0 ]
 if [ "$target" != nano-p2p ];then [ "$(cat "$out/$target-run-exit.txt")" = 0 ];fi
done
sha256sum -c "$out/sources.sha256" > "$out/sources-unchanged.log"
sha256sum -c "$out/parent-before.sha256" > "$out/parent-unchanged.log"
set +e
"$out/nano-p2p" --help > "$out/p2p-cli-negative-verification.log" 2>&1
status=$?
set -e
printf '%s\n' "$status" > "$out/p2p-cli-negative-verification-exit.txt"
[ "$status" = 1 ]
grep -F 'Usage: nano-p2p <core-rpc-port> <identity-file> <listen-multiaddress>' "$out/p2p-cli-negative-verification.log"
sha256sum "$out/nano-p2p" >> "$out/programs.sha256"
echo 'Compiled/linked targets verified; unsupported --help negative CLI behavior verified; live networking not qualified'
