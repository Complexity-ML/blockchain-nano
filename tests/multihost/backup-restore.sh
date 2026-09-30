#!/usr/bin/env bash
# Qualification of an explicitly stopped fixture, never a live-node backup.
set -euo pipefail
if [ "$#" -ne 5 ]; then
  echo 'Usage: backup-restore.sh stopped-data new-output genesis.json audit-request.json nano-binary' >&2
  exit 2
fi
source_root=$(realpath "$1")
output_root=$(realpath -m "$2")
genesis_file=$(realpath "$3")
request_file=$(realpath "$4")
nano_binary=$(realpath "$5")
test -f "$source_root/chain/CURRENT"
test -f "$source_root/core-witness/CURRENT"
test -f "$source_root/peer.key"
# Fixtures contain only directories and regular files; never follow symlinks.
if [ -n "$(find "$source_root" ! -type d ! -type f -print -quit)" ]; then
  echo 'Special files or links in source' >&2; exit 1
fi
case "$output_root/" in "$source_root/"*) echo 'Output inside source' >&2; exit 1;; esac
mkdir -m 700 "$output_root"
manifest() { (cd "$1" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum); }
manifest "$source_root" > "$output_root/source-before.sha256"
sha256sum "$nano_binary" "$genesis_file" "$request_file" > "$output_root/inputs.sha256"
tar -C "$source_root" -czf "$output_root/data.tar.gz" .
sha256sum "$output_root/data.tar.gz" > "$output_root/archive.sha256"
mkdir -m 700 "$output_root/restored"
tar -C "$output_root/restored" -xzf "$output_root/data.tar.gz"
manifest "$output_root/restored" > "$output_root/restored-before.sha256"
cmp "$output_root/source-before.sha256" "$output_root/restored-before.sha256"
"$nano_binary" audit-capella "$genesis_file" "$output_root/restored/chain" "$request_file" > "$output_root/audit.json"
manifest "$output_root/restored" > "$output_root/restored-after.sha256"
manifest "$source_root" > "$output_root/source-after.sha256"
cmp "$output_root/restored-before.sha256" "$output_root/restored-after.sha256"
cmp "$output_root/source-before.sha256" "$output_root/source-after.sha256"
printf '%s\n' 'Stopped fixture archive, full byte-equivalent restoration and non-mutating canonical audit passed.' > "$output_root/passed.txt"
