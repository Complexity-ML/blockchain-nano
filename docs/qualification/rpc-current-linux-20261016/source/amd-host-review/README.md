# AMD RPC current RocksDB test — host source only

This is a small new main around the public AMD host's already-qualified
functions (`f28ae7a5…` /52822). It authenticates that source's known bytes and
FD/path nine, executes those exact bytes with a non-main module name, and does
not call its old main or write `__pycache__`. The author has performed only
public-source reads, byte hashing, source writing and Python AST parsing.
No private receipt, SDK, executable, Docker, SSH, Git or project program was
read/executed by the author.

Root-only invocation, after source review and remote source preparation:

```text
python3 qualify-current-amd-rpc-root.py --output-root /home/ubuntu/current-amd-rpc-NEW --worker /absolute/public/worker.pl
```

The worker must be exactly public `ab037013…` /36029. The new explicit mode is
`--rpc-eip1898-current`; no old helper's compile/object/MD/arity test is reused
or invoked. Runtime result and a new private software admission stay on the
VPS; only the public result and its pins should be returned by Root's dispatcher.
The host does not provide a dispatcher or authorize remote execution.

## Finite inputs and operations

* The public host functions come from the versioned repository path under
  `docs/qualification/current-runtime-assemblies-20261016/source/amd-host`.
* Git HEAD must be `87a320946f1d3ed494bf54e1bb16b6aa67df685c`, clean; all seven
  selected tree entries must equal the public qualified `bd11bd9f…` entries.
  The current RPC TU is separately projected and fixed to `3f3dc2bc…` /12665.
* Only the four fixed control06 receipts already used by that host are read:
  artifacts `4e9ff3…`, links `55ff75…`, archives `3a4f18…`, LLVM `ccc3bd…`.
  Their SHA/size constants are inherited unchanged. No original global SDK map,
  source admission, fresh SDK fallback or whole historical file walk is opened.
* The bootstrap-audit target supplies the actual canonical Core-only semantic
  flags and link order. Both its simple `PRIVATE nano_core` declaration and the
  four current dedicated RPC target declarations are checked in the exact
  current CMake bytes. The selected flags have no NANO macro. Its old object,
  MD, CLI, source-dependency table and image-dependency table are not consumed.
* Four non-Core archives and two LLVM objects use the original retained full
  nine plus SHA from the fixed producer records. Ordering is preserved. The
  current Core comes instead from the final runtime bundle's original new
  descriptor `5c5bb592…` /8589, retaining its full nine. No old row is refreshed.
* The known Core is copied only into a new 0400 file; all source/control mounts
  are RO, output bind is new Linux UID1000:GID1000 0700. Existing immutable
  image `fd9b0bb9…` runs network none/read-only/cap-drop/no-new-privileges,
  2 CPUs/3 GiB. The test has a fresh 128 MiB `/tmp` tmpfs.

One independent M, one CXX with real MD, one link, one ldd, and one dedicated
`--eip1898-self-test` (at most45s) are required. The worker must return exactly
five groups,120 checks,58 expected refusals, stderr empty. New exe/object/MD and
the hash-bound private software admission are admitted on the host only after
three stable full-nine/known-byte observations over at least2s. The inherited
1100s host deadline and60s owned cleanup budget remain. Only a newly attempted
UUID/name/image/label/full-ID container may be removed; output files are retained
on failure. Global final original-input fences run on both success and failure.

## Scope and remaining work

This is a **pending source proposal**, not a Linux test result. The TU's
RocksDB `unique_ptr`/`DB**` compatibility helper is the reason for the new Linux
test; the earlier variant's success is not relabelled. No old helper suite,
earlier RPC120 test, Core143/142 proof, image build, full CMake configure or Core
compilation is replayed. The synthetic `/tmp` database is opened and removed by
the test; this must not be described as zero DB access.

The worker's new same-container FD/stat/SHA observations are retained through
its private admission. They do not recover old SDK metadata, close the whole
SDK/loader/lazy-symbol/producer DAG, or grant a campaign admission. HTTP, Nano
runtime, network, duties, common finality and production remain unqualified.
Root must review the host and dispatcher, run Perl syntax-only first, then
perform the single bounded qualification. No author execution is claimed.
