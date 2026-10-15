#!/usr/bin/env bash
# Qualification of an explicitly stopped fixture, never a live-node backup.
set -euo pipefail

# Explicit independent-root mode. Stop/nonce JSON is validated by the caller;
# its externally supplied SHA binds the same stopped receipt throughout copying.
if [ "${1-}" = '--two-roots' ]; then
  [ "$#" -eq 9 ] || [ "$#" -eq 10 ] || {
    echo 'Usage: backup-restore.sh --two-roots stopped-data stopped-witness new-output genesis.json audit-request.json nano-binary stopped-receipt.json expected-stop-sha256 [10001:10001]' >&2; exit 2;
  }
  owner=''
  if [ "$#" -eq 10 ]; then
    [ "${10}" = '10001:10001' ] && [ "$(id -u)" -eq 0 ] || { echo 'Invalid restored-copy owner or offline user' >&2; exit 2; }
    owner=${10}
  fi
  # Reject symlink components before canonicalization can hide an alias.
  canonical_directory() {
    test -d "$1" && [ "$(realpath -- "$1")" = "$(realpath -ms -- "$1")" ] || { echo 'Noncanonical or linked source root' >&2; return 1; }
    realpath -- "$1"
  }
  data=$(canonical_directory "$2"); witness=$(canonical_directory "$3")
  output=$(realpath -m -- "$4")
  [ "$output" = "$(realpath -ms -- "$4")" ] || { echo 'Linked output path refused' >&2; exit 1; }
  genesis=$(realpath -- "$5"); request=$(realpath -- "$6"); nano=$(realpath -- "$7"); stop=$(realpath -- "$8"); stop_sha=$9
  [[ "$stop_sha" =~ ^[0-9a-f]{64}$ ]] || { echo 'Invalid independent stopped-receipt SHA' >&2; exit 2; }
  disjoint() { case "$1/" in "$2/"*) return 1;; esac; case "$2/" in "$1/"*) return 1;; esac; }
  disjoint "$data" "$witness" && disjoint "$data" "$output" && disjoint "$witness" "$output" || { echo 'Overlapping data, witness or output roots' >&2; exit 1; }
  for path in "$stop" "$genesis" "$request" "$nano"; do
    test -f "$path" && disjoint "$path" "$data" && disjoint "$path" "$witness" && disjoint "$path" "$output" || { echo 'Inputs must be independent of archive and output roots' >&2; exit 1; }
  done
  test ! -e "$output" && test ! -L "$output"
  test -f "$data/chain/CURRENT" && test -f "$data/peer.key" && test -f "$witness/chain-witness/CURRENT"
  tree_safe() {
    local bad
    bad=$(find "$1" ! -type d ! -type f -print -quit) || return 1
    [ -z "$bad" ] || { echo 'Source alias, link or special entry refused' >&2; return 1; }
    bad=$(find "$1" -type f ! -links 1 -print -quit) || return 1
    [ -z "$bad" ] || { echo 'Source alias, link or special entry refused' >&2; return 1; }
  }
  tree_safe "$data"; tree_safe "$witness"
  # Duplicate dev/inode entries across the two trees also reject bind aliases.
  aliases=$(find "$data" "$witness" -printf '%D:%i\n' | LC_ALL=C sort | uniq -d)
  [ -z "$aliases" ] || { echo 'Aliased source namespace' >&2; exit 1; }
  check_stop() { [ "$(sha256sum -- "$stop" | cut -d ' ' -f 1)" = "$stop_sha" ] || { echo 'Independent stopped-receipt mismatch' >&2; return 1; }; }
  manifest() { (cd "$1" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 -r sha256sum); }
  # Nine stat fields (mode represented by type+permissions), excluding atime.
  materials() { (cd "$1" && find . -printf '%D %i %y %m %U %G %n %s %T@ %C@ %P\0' | LC_ALL=C sort -z); }
  input_materials() { find "$nano" "$genesis" "$request" "$stop" -maxdepth 0 -printf '%D %i %y %m %U %G %n %s %T@ %C@ %p\0' | LC_ALL=C sort -z; }
  check_stop
  mkdir -m 700 -- "$output"
  sha256sum -- "$nano" "$genesis" "$request" "$stop" > "$output/inputs.sha256"
  input_materials > "$output/inputs-before.stat"
  for role in data witness; do
    root=${!role}
    manifest "$root" > "$output/$role-source-before.sha256"
    materials "$root" > "$output/$role-source-before.stat"
    tar -C "$root" -czf "$output/$role.tar.gz" .
    mkdir -m 700 -- "$output/restored-$role"
    tar -C "$output/restored-$role" -xzf "$output/$role.tar.gz"
    tree_safe "$output/restored-$role"
    manifest "$output/restored-$role" > "$output/$role-restored-before.sha256"
    materials "$output/restored-$role" > "$output/$role-restored-before.stat"
    cmp "$output/$role-source-before.sha256" "$output/$role-restored-before.sha256"
  done
  aliases=$(find "$data" "$witness" "$output/restored-data" "$output/restored-witness" -type f -printf '%D:%i\n' | LC_ALL=C sort | uniq -d)
  [ -z "$aliases" ] || { echo 'Restoration is not physically independent' >&2; exit 1; }
  sha256sum -- "$output/data.tar.gz" "$output/witness.tar.gz" > "$output/archive.sha256"
  cp -- "$stop" "$output/stopped-receipt.json"
  [ "$(sha256sum -- "$output/stopped-receipt.json" | cut -d ' ' -f 1)" = "$stop_sha" ]
  "$nano" audit-capella "$genesis" "$output/restored-data/chain" "$request" > "$output/audit.json"
  for role in data witness; do
    root=${!role}; tree_safe "$root"
    manifest "$root" > "$output/$role-source-after.sha256"
    materials "$root" > "$output/$role-source-after.stat"
    manifest "$output/restored-$role" > "$output/$role-restored-after.sha256"
    materials "$output/restored-$role" > "$output/$role-restored-after.stat"
    cmp "$output/$role-restored-before.stat" "$output/$role-restored-after.stat"
    cmp "$output/$role-source-before.sha256" "$output/$role-source-after.sha256"
    cmp "$output/$role-source-before.stat" "$output/$role-source-after.stat"
    cmp "$output/$role-restored-before.sha256" "$output/$role-restored-after.sha256"
  done
  check_stop
  sha256sum --check --status "$output/inputs.sha256"
  input_materials > "$output/inputs-after.stat"
  cmp "$output/inputs-before.stat" "$output/inputs-after.stat"
  # Ownership changes affect fresh copies only, after all original fences.
  if [ -n "$owner" ]; then
    chown -R -- "$owner" "$output/restored-data" "$output/restored-witness"
    for role in data witness; do
      manifest "$output/restored-$role" > "$output/$role-restored-owner-after.sha256"
      cmp "$output/$role-restored-after.sha256" "$output/$role-restored-owner-after.sha256"
    done
    printf '%s\n' "$owner" > "$output/restored-owner.txt"
  fi
  printf '%s\n' 'Stopped independent data/witness archives, byte-equivalent physical copies and separate non-mutating canonical chain audit passed; signer/witness authentication and resumed duties require their own checks.' > "$output/passed.txt"
  exit 0
fi
if [ "$#" -ne 5 ] && [ "$#" -ne 6 ]; then
  echo 'Usage: backup-restore.sh stopped-data new-output genesis.json audit-request.json nano-binary [10001:10001]' >&2
  exit 2
fi
if [ "$#" -eq 6 ]; then
  [ "$6" = '10001:10001' ] && [ "$(id -u)" -eq 0 ] || { echo 'Invalid restored-copy owner or offline user' >&2; exit 2; }
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
# Extraction is fresh and rejects links above. Only its copy changes ownership.
if [ "$#" -eq 6 ]; then
  chown -R -- "$6" "$output_root/restored"
  manifest "$output_root/restored" > "$output_root/restored-owner-after.sha256"
  cmp "$output_root/restored-after.sha256" "$output_root/restored-owner-after.sha256"
  printf '%s\n' "$6" > "$output_root/restored-owner.txt"
fi
printf '%s\n' 'Stopped fixture archive, full byte-equivalent restoration and non-mutating canonical audit passed.' > "$output_root/passed.txt"
