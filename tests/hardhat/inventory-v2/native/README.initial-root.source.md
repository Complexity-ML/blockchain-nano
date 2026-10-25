# Initial state root: one empty Geth transition

SOURCE ONLY. No program, test, compiler, client, Docker, SSH or chain was run by the author. This is a proposed supplementary control, not an additional set of 65 transaction passes. Root must execute and independently adopt its actual terminal evidence.

## Exact purpose

Use the existing official standalone `evm` 1.16.7 binary, SHA-256 `7bad7839da2684bbc6eeab1c5b045e2f2bedd2e9132d8971ccd4f25adc058861`, for exactly one Shanghai transition containing an empty transaction list. Keep `--state.chainid=31337`, `--state.reward=-1` and the original first-step environment. The new input is the JSON string `"0xc0"` followed by a newline, following the existing `t8n-reference.mjs` input contract. No signed transaction is replayed, produced or submitted.

The result must have zero accepted/rejected transaction receipts and its `stateRoot` must equal `nativeAccounting.transactions[0].stateRootBefore`: `0x59b1d26c399cf72a530c508fd24c52b6375ce7adfe54fd7d37fbf33431886d7e` in the retained public accounting. This expected root is read from the externally pinned accounting, not substituted into the allocation or Geth result. Its own `post.json` must also contain exactly the same three fixture EOA with unchanged balances, zero nonces and empty code/storage. Geth's optional address prefix/case and omitted zero fields are accepted semantically; duplicate address aliases, extra accounts or account fields are refused. These comparisons never rewrite, canonicalize or refresh its allocation bytes.

## Caller contract

Invoke `node initial-root.mjs /absolute/control.json CONTROL_SHA256`. Root creates this control after reviewing the proposal; no real control or execution authority is created here.

```json
{
  "format": "nano-inventory-v2-initial-root-control-v1",
  "output": "/absolute/new-output",
  "caseRoot": "/absolute/qualified-source-root",
  "evm": {"path": "/absolute/evm", "sha256": "EXTERNAL_SHA256", "bytes": 1},
  "nativeAccounting": {"path": "/absolute/native-accounting.json", "sha256": "EXTERNAL_SHA256", "bytes": 1},
  "referenceTrace": {"path": "/absolute/t8n-reference-trace.json", "sha256": "EXTERNAL_SHA256", "bytes": 1},
  "referenceBinding": {"path": "/absolute/input-binding.json", "sha256": "EXTERNAL_SHA256", "bytes": 1},
  "referenceResult": {"path": "/absolute/result-public.json", "sha256": "EXTERNAL_SHA256", "bytes": 1},
  "sourceAlloc": {"path": "/absolute/original/step-01/alloc.json", "sha256": "EXTERNAL_SHA256", "bytes": 1},
  "sourceEnv": {"path": "/absolute/original/step-01/env.json", "sha256": "EXTERNAL_SHA256", "bytes": 1}
}
```

All placeholder pins and byte counts above must be replaced. Artifact fields are exact `{path,sha256,bytes}`; lower-case SHA-256, canonical absolute paths and positive bounded lengths are required. The output must be new and disjoint from retained input files. `caseRoot/tests/multihost/{campaign-deadline,campaign-commands}.mjs` must have the exact SHA values embedded in the controller. They are loaded from their verified bytes; no SDK or package import is needed.

Allowed existing public inputs are the three retained reference JSON files under `inventory-v2-t8n-reference-root-01/retained-public` and `inventory-v2-native-runtime-root-01/retained-public/outputs--native-run--native-accounting.json`. The reference binding must join the Native accounting SHA/length, Geth SHA/version, chain ID and fork. The trace first row must join the same transaction and post-root, with all eight named checks present and true. The original allocation must equal `trace[0].inputAlloc` SHA/length (retained value `abb3a67d69f1a197c4279aa271df5e7c65b32f07bf8d1bad50d0faac9a0fc236`, 459 bytes), and contain exactly the three original public fixture accounts funded with `10^24`, nonce zero and no code/storage.

Root identified the original VPS paths as `/home/ubuntu/nano-inventory-v2-t8n-reference-root-20261025-01/outputs/result/step-01/{alloc.json,env.json}`. The original trace does not retain an environment hash. Therefore the controller additionally requires exact original producer bytes, independently reconstructed from source: coinbase `0x0000000000000000000000000000000000009999`, gas limit 60000000, block number 1, timestamp 1700000012, zero random/base fee, empty withdrawals and block hashes, JSON two-space formatting plus newline in the producer's property order. A caller pin alone cannot replace this comparison. It makes no claim of historical inode continuity for these files.

## Terminal evidence and limits

Inputs and outputs are regular files opened with `O_NOFOLLOW`, link count one, bounded by 128 MiB (retained JSON bytes 16 MiB), and held by descriptors. SHA/length plus full nine-field FD/path identity are checked at admission, before the sole command, and finally. The full-nine order is `dev,ino,mode,nlink,uid,gid,size,mtimeNs,ctimeNs`. No input is rewritten; the exact allocation/environment bytes are copied to new files. Result, post-allocation and body are retained with pins. A present total-gas field must be zero; an absent field is reported absent, not fabricated.

The monotone budget is 120 seconds including a 20-second closeout reserve. The sole detached child has a 30-second command cap, additionally clamped by the original work deadline. It uses a minimal clean environment and the pinned program path. Normal completion requires actual exit and close code zero with no signal, helper-owned process-group join, final input/output fences and all known descriptors closed. Cancellation, late completion, missing output, changed files, rejected input or unequal root remains a failure. Budgets are cooperative and do not claim a physical hard wall.

`initial-root-controller.json` and the final stdout item deliberately keep `passed:false`. `terminalEvidenceReady` is only a candidate for Root's separate external exit/close/group/descriptor adoption; a stored JSON boolean cannot establish that the outer process exited. If publication or a late deadline fails, the terminal item and exit code reject the run even if an earlier file was written. Root retains command logs and independently pins outputs. No Native engine rerun, compilation, image build, consensus, public network, economic relationship, launch or production readiness is qualified. Existing 65-transition counts and historical failures remain unchanged.
