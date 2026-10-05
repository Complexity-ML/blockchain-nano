#!/bin/bash
set -eu
trap 'nano_control_exit=$?; echo "$nano_control_exit" > /report/controller-exit.txt' EXIT
clang++ -std=c++20 -O2 -I/work/third_party /work/tools/storage-maintenance/migrate-pruned-owned-copy.cpp -lrocksdb -lcrypto -o /report/migrate-tool > /report/compile-migrate.log 2>&1
clang++ -std=c++20 -O2 -I/work/third_party /work/tools/storage-maintenance/compact-owned-copy.cpp -lrocksdb -lcrypto -o /report/compact-tool > /report/compile-compact.log 2>&1
cd /source
find . -type f -print0 | sort -z | xargs -0 sha256sum > /report/source-before.txt
for operation in migrate compact; do
 mkdir /report/$operation
 cp -a /source /report/$operation/chain
 if [ "$operation" = migrate ]; then marker=/report/$operation/.nano-owned-schema-migration-copy; flag=owned-offline-schema-migration-v1; args=(--offline-owned-copy /report/$operation/chain --audit-request /proof/audit-request.json); else marker=/report/$operation/.nano-owned-compaction-copy; flag=owned-offline-qualification-copy-v1; args=(--offline-owned-copy /report/$operation/chain); fi
 cd /report/$operation/chain
 find . -type f -print0 | sort -z | xargs -0 sha256sum > /report/$operation-before.txt
 for rejection in symlink oversize ambiguous; do
  case "$rejection" in
   symlink) printf '%s\n' "$flag" > /report/marker-target; ln -s /report/marker-target "$marker";;
   oversize) printf '%s\n%0200d' "$flag" 0 > "$marker";;
   ambiguous) printf '%s\nextra' "$flag" > "$marker";;
  esac
  if /report/$operation-tool "${args[@]}" > /report/$operation-$rejection.log 2>&1; then exit 31; fi
  grep -Eq 'marker|Owned migration chain copy required' /report/$operation-$rejection.log
  rm "$marker"
 done
 find . -type f -print0 | sort -z | xargs -0 sha256sum > /report/$operation-after-refusals.txt
 cmp /report/$operation-before.txt /report/$operation-after-refusals.txt
 printf '%s\n' "$flag" > "$marker"
 /build/pruning-wan-092a153c-arm64/nano audit-capella /config/genesis.json /report/$operation/chain /proof/audit-request.json > /report/$operation-audit-before.json
 /report/$operation-tool "${args[@]}" > /report/$operation-result.json
 if [ "$operation" = migrate ]; then
  /report/$operation-tool "${args[@]}" > /report/migrate-idempotent.json
  reader=/build/capella-sync-http-arm64-20261005/nano
 else reader=/build/pruning-wan-092a153c-arm64/nano; fi
 "$reader" audit-capella /config/genesis.json /report/$operation/chain /proof/audit-request.json > /report/$operation-audit-after.json
 done
cd /source
find . -type f -print0 | sort -z | xargs -0 sha256sum > /report/source-after.txt
cmp /report/source-before.txt /report/source-after.txt
