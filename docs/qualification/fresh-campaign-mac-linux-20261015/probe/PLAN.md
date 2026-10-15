# Local synthetic physical probe, before a new seed

Source-only controller, not executed by its author. It imports the exact
`createMacLinuxFiles` module from Root's already staged public replica:
`../fresh-campaign-mac-linux-root-20261015/execute-01/source`.
No C++, importer, Nano, P2P, SDK, genesis, real journal, key, seed, exporter,
private campaign, SSH or network launcher is used. No new image is built,
pulled or configured. This probe does not rerun the successful pure tests.

## Control and invocation

Root alone composes a new EXCL 0600 control file from its original Node,
Docker and staged source authorities; the author must not read that control.
Its exact ten fields are:

```text
moduleRoot        exact absolute public replica root
helperPath        <moduleRoot>/docs/qualification/local-supervisor-linux-20261015/source/known-files.pl
copierPath        same directory /copy-bootstrap-pairs.pl
dockerPath        Root's canonical, previously pinned Docker executable
dockerEndpoint    unix:///Users/boris/.docker/run/docker.sock
outputDirectory   a new absent private directory, disjoint from moduleRoot and every input
uuid              fresh lowercase RFC4122 v4 UUID
prefix            nano-transport-probe-<uuid>
softwareMounts    []
sourcePins        array of original {path,sha256,bytes,material} descriptors
```

The nine material values are decimal strings in this exact order:
`dev,ino,mode,nlink,uid,gid,size,mtimeNs,ctimeNs`. They must be carried from
Root's existing authority, never sampled anew as a substitute for old identity.
Include canonical Node, Docker, the module and both Perl sources; Root may
carry its full existing replica closure. The controller requires the original
Docker and canonical executing Node pin and fences all supplied descriptors
before and after every Docker CLI operation. Root additionally fences its
original Node and the new controller before and after invocation. The new
control itself is first held only after matching its explicit known SHA/size.

Source command, to be executed only by Root after peer review:

```sh
"$PINNED_NODE" probe.mjs "$NEW_CONTROL" "$CONTROL_SHA256" "$CONTROL_BYTES"
```

The controller binds Docker to the explicit local Unix socket with `-H`, uses
a new empty private Docker-client config directory, and removes ambient
`DOCKER_*`, `NODE_*`, `DYLD_*`, `LD_*`, `NANO_TEST_*` selectors from child
environment. It checks exact existing image ID, Linux OS and arm64 before
using image `sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb`.
It never starts the daemon or calls build, pull, prune or volume deletion.

## Exact reused programs and actual syntax

Module: `tests/multihost/linux-owned-files.mjs`, SHA-256
`d86be73f14b258c0678b84e1dfc5e7a28429e676fb1110c6102aacb289615bd6`,
18094 bytes, from source candidate manifest e27d1fe8 (8392 bytes).
Known-file helper: SHA-256
`7dd88b7d99ab15e9f2b1e52aac796265e6ed9bed24323e66584b3f31138cc0f6`,
12273 bytes. Original copier: SHA-256
`9fcd47d3bee3530c163e00e8b6a8dd475645336ba48579db45bdc9e20bb3bdf3`,
10433 bytes. The module derives its exact Mac-Docker copier through its own
checked `macDockerCopier` function, keeping its 16-pair limit and changing
only the existing owner/index anchors to `mac-docker`, indices 16 through 31.

Before any fixture payload is produced, actual isolated Perl jobs check
dependencies and run `perl -c` on helper7dd, that derived copier, the exact
internal opaque-reader literal, and the small synthetic setup program. The
reader is extracted only from the exact pinned module's unique
`const nestedReader=String.raw` source literal; no alternative reader is
substituted. `preflight(importer)` is deliberately not called: its binary
loader check would require a project executable outside this synthetic scope.

Every job uses the existing module's network-none, read-only container,
501:20 identity, CPU/memory limits, RO helper file and strict owned volumes.
The module's root-only empty-volume initialization retains its exact CHOWN
exception. This tests the same callback/stdin/mount/receipt path as the port.

## Minimal physical sequence

1. Construct all expected opaque-reader bytes and the full 64-blob copy table
   in memory before the first producer. Persist their known paths/SHA/size as
   a new private output. No file inventory supplies this authority.
2. Create eight new labeled owned Linux volumes. Populate three reader
   fixtures, one bootstrap source tree and two intentionally invalid alias
   reader fixtures. The remaining proof/data/witness volumes start empty.
   Sources are fixed non-signing text/binary buffers, including NUL/0xff.
   Setup uses EXCL writes, 0600 files, 0700 directories and file/directory
   fsync, never a DB engine or signing library.
3. Invoke the exact opaque reader for all three allowed kinds:
   bootstrap `journal-bindings.json`, action `probe-result/action.private.json`,
   and accepted `/proof/accepted-job.private.json`. Compare each returned
   buffer with its prior known bytes. Those filenames are synthetic containers
   only; no logical action, accepted job or 65-key report is asserted.
4. Require three specific reader refusals: an incorrect known digest at the
   already admitted bootstrap path, an allowed action path that is a symlink,
   and that path hardlinked to another file. They are separate fresh volumes,
   not mutations of old inputs. Accept only a completed nonzero Perl command
   with the exact reader refusal marker, never a timeout, truncation, image or
   setup failure. Failed reads do not refresh the stored material authority.
5. Bootstrap contains only `primary-16..31` and `witness-16..31`, each with
   `record.bin` and `sub/chunk.bin`: 32 roots, 64 files, 64 directories. No
   indices 0..15 exist, so successful copying exercises the actual successor
   mapping rather than the old copier. Write the exact 16-pair spec and derived
   copier through the actual helper7dd known-file API, read the ASCII spec back
   and verify its original authority.
6. Run copier `create`, read its generated proof by the producer's held SHA,
   and independently reconcile every source and copy file with the prior
   64-blob table: 128 exact payload comparisons. Verify all 256 source/copy
   directory/file dev:ino identities are distinct. This prevents a newly
   scanned inventory from becoming the sole authority for fixture contents.
   Raw physical rows remain only in the new private output.
7. Run copier `verify` against the exact same proof SHA, then the module's
   `verifyAll` against original helper and reader authorities. Never normalize
   or replace an original material row. The copy proof's existing GNU-stat
   row representation is retained as produced; it is not rewritten to match
   a different authority format.
8. Verify absence of every owned container and preserve every exact labeled
   volume. Only a still-present container with the exact UUID label and image
   may be removed. No unrelated resource or volume is pruned.

## Bounds and reviewable outcome

The main probe has a 600-second deadline, then one global 45-second cleanup
budget. Each CLI child has an explicit bound, 4 MiB per output stream and joined
close; timeout/truncation/spawn/stdin errors cannot count as expected refusals.
Root's separate outer bound may be 900 seconds with owned process-group control.
Cleanup reports exact owned-name, verified-absent and retained-volume counters;
failure to finish cleanup remains a failure, with volumes left intact.

Only a typed JSON terminal summary is written to stdout after success. Raw
command output, authority receipts and diagnostics remain EXCL 0600 in the new
0700 output, outside Git. Even a full success qualifies only this local Linux
transport, literal Perl programs, known-byte reads/refusals, assigned copy
mapping and physical identities for these synthetic buffers. It does not
qualify the 65 real P/W tokens, source-domain/history correspondence, RocksDB,
actual importer or duties, private-transfer permissions, finality, the full
lifecycle, Base/bridge liquidity or production readiness.
