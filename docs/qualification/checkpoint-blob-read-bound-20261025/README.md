# Checkpoint blob application-copy bound — 25 October 2026

C++ candidate `0bdc9c8de1343cc12b489ed5841703dd57caf975`; delivery code `50070e03` changes only the JavaScript CI assertion. `ChainStore::checkpoint_blob` fetches a PinnableSlice, refuses empty/oversized values, then hashes and copies accepted data. Integrity, chain/genesis binding and missing-record behavior remain intact. No persisted format or public C++ API changed.

## Executed qualification

Five exact CTests passed without skips on each reused Docker ARM64 and AMD64 image: application-copy boundary (two groups), immutable blobs, blob sync failure, reactor sync failure and cold-reactor guards. The seven emitted runtime records agree byte-for-value across architectures. Three programs were built/linked. Two Core translation units were actually compiled: storage plus KZG after regeneration of its setup header; KZG's object and generated header remained byte-identical. Only the storage object payload changed among 145 inherited Core members. This is an incremental component qualification, not a fresh full-client build or a bitwise reproducibility proof.

The two new groups exercise the actual production hash/copy boundary and a real fresh ChainStore/provider, including exact/oversized/empty/missing data, corrupt data, chain/genesis/digest/zero-limit refusal, original exception propagation, unchanged checkpoint and provider closure. A 64 KiB stored blob is refused with a 256-byte application bound.

Sixteen host JavaScript tests passed for the exact 26-test daily registry and its refusal controls. The complete daily C++ selection of 26 tests was not rerun. GitHub CI remains unqualified; its previous run was rejected before steps by account billing/spending limits. No manual rerun or spending occurred.

## Preserved failures and limits

ARM envelope01 remains FAILED: compilation succeeded, but the controller expected only storage.cpp to compile and rejected the additional byte-identical KZG rebuild. Tests02 admitted the retained binaries and executed the five tests without recompiling. The first host CI-guard run remains FAILED (15/16): one assertion still expected 25 rather than 26; its original raw stream is retained in the tool transcript only. The corrected run and both failed outcomes are described in summary.json. Earlier production/history failures remain untouched.

PinnableSlice may allocate inside RocksDB before the application checks length; identity reads and old immutable-duplicate writer reads still materialize without this application bound. This change does not bound all allocations, decompression, physical RSS, WAL or recovery time. It does not implement durable cold-deposit context v4, activate pruning, qualify physical power loss, start a validation network or restore signing duties. Existing old-format cold-record restart tests are distinct from the new deposit-context work. No image was built, node deployed, real funds used or sale opened. The production goal and independent security review remain open.

`proofs/` contains exact JUnit/full test streams, build streams, results, architecture/binary hashes and controller sources; `FILES.json` binds their bytes. Original qualified build outputs were copied to a separate incremental workspace, not overwritten.
