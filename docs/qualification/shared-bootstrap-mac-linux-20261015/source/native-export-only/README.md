# Native export-only preparer successor

Source-only implementation outside the repository. No execution, private
input/SDK/binary/database read or repository write by this author. Original V7
51bd/41900 and all historical results remain immutable. This package includes
the complete successor, two new refusal tests and both unchanged Perl sibling
sources required by the existing preparer's `import.meta.url` locators.

The public API adds exactly one explicit parameter:

```javascript
await prepareLocalFixture(inputPath, knownInputSha, newWorkspace, 'native-export-only');
```

The default remains `local-supervisor`. Unknown values fail with
`NANO_FIXTURE_PREP_MODE` before reading input, installing signal hooks, creating
output/seed or running any process. The existing CLI stays strictly at its
original three positional arguments and full default mode. There is no new
environment selector or CLI flag; Root uses the explicit API for export-only.

The 23 input fields and exact dynamic 51+N protected set remain unchanged.
Before export, all existing source/alias/fixture/Node/Docker, staged-owner,
loader and image gates remain. One fresh seed and one real native exporter
invocation produce the actual twelve leaves. After their original bounded
first admission and six public copies, the new branch closes the ledger with
exact old names+one, strict old full-nine/SHA and original observed APFS nlink+1.
It does not enter Linux volume initialization/import, the old 0..15 physical
copy, assembler/runtime configuration, work-order creation or driver handoff.

The original full-mode body stays byte-identical inside its `else` block.
Both modes join the original catch/finally cleanup and held/created fences.
There is no early return inside try. Only full mode may create the old startup
age and handoff, and it must have its original handoff. Export-only explicitly
requires no handoff/work order/age. The default CLI is unchanged.

## Capsule and producer pin

The new private file is exactly:
`<workspace>/private/native-export-capsule.private.json`.
Its format is `nano-native-export-only-capsule-v1`. It contains only descriptor
metadata and scope, never the contents of private env/seed/key files:

- `exportLeaves`: exact twelve names -> `{path,sha256,bytes,material}`;
- `publicCopies`: exact six names with the same descriptor shape;
- `ledgerRecord`: the one new reservation's ORIGINAL first-held nine and SHA;
- `sourceInput`: this exact Root input's known SHA/bytes and held nine;
- `sourceInputs`: original external 51+N descriptors, each strictly rechecked;
- `sourceCodeInputs`: current preparer and unchanged two Perl sources, held
  before export, without a new metadata admission;
- original native alias records, campaign ID and export directory;
- old-ledger N unchanged, one new reservation and one exporter invocation;
- zero Nano starts and explicit false driver/work-order/age/import/plan claims.

`material` uses the existing V7 field order, decimal strings:
`dev,ino,mode,nlink,uid,gid,size,mtimeNs,ctimeNs`. The descriptor helper first
requires a prior held row, then calls `read(...,admit=false)` and compares it.
It cannot silently add a freshly sampled source authority.

The capsule is EXCL/fsynced and stabilized before the common final fences.
Success returns `nano-native-export-only-preparation-result-v1` with its
producer's `capsuleSha256` and `capsuleBytes`. Root must match that producer pin
before parsing the private capsule; no current hash replaces it. The public
result is stored separately as `native-export-only-public-result.json`, with
no old `preparedRuntimeOwners`, `importedPairs`, cold reopen, work-order hash
or handoff age certificate. An export creates the specified bootstrap signing
history; this result does not claim a zero-signature producer or verify its
cryptography. The actual later native importer performs that verification.

## Exactly two new negative checks

Root may run only the new source-level refusal file:

```sh
"$PINNED_NODE" --test prepare-export-only-refusals.test.mjs
```

It checks the exact successor source pin, retains its full nine, and uses only
fresh temporary missing-path fixtures. The unknown-mode API child imports the
source with a sentinel argv[1] so its CLI cannot run on import, then requires
the unique mode error. The second child requires existing CLI arity refusal
for a missing workspace before preparation. Both check input/output absence
and unchanged original source identity afterwards. No real control exists, so
no exporter, Nano, P2P, Docker, ledger or seed can be touched. These are two
small new refusals, not a replay of the old full-mode physical scenario.

For an actual export-only call, Root must separately original-pin all Node,
Docker, exporter/dependency/fixture/alias and ledger authorities, including the
newly anchored old reservation. New source copies need known-byte first
stabilization before first admission; no original row is refreshed. The later
actual native importer/inventory and six-doc plan, then the shared real Linux
65-pair import and 16..31 cold audit, are separate consumers of this one fresh
domain. Their original software authorities must already be available before
export. No same-seed retry, reexport, private transfer to another host, duties,
runtime clock qualification, finality, whole lifecycle or production is implied.
