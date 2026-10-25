# Public canonical-stream corpus v1

Fixed synthetic data for the Nano canonical-stream component tests. The fixture
uses 64 publicly reproducible BLS identities, outside a validator network.

| Mode | Signed descendants | Attestations | Files |
|---|---:|---|---|
| `complete` | 1,026 | Enabled | `manifest.json`, `corpus.json` |
| `stalled` | 255 | Disabled | `manifest.json`, `corpus.json` |

Each mode contains exactly those two files. The corpus stores signed SSZ,
frozen producer expectations (`Block::to_json(true)` and execution-state roots),
head/finalized checkpoints and the fixed anchor/genesis binding. The complete
case includes a genuinely valid sibling block for the wrong-branch refusal.
Expected execution data comes from Nano’s public fixture producer; it is kept
separate from the Core replay under test.

The body records the existing SHA-256/byte pins of five source inputs:
`capella-bootstrap-anchor-cb532/{manifest.json,profile.json,genesis-state.ssz,genesis.json}`
and `consensus-capella-slots-official/slots_1/pre.ssz`.
Reviewed literal manifest pins live in
[`capella_stream_public_corpus_pins.hpp`](../../support/capella_stream_public_corpus_pins.hpp);
each manifest also pins its body SHA-256/bytes. Loading requires exact pins,
mode, sizes, canonical JSON and recomputed fixed-anchor identity. It never learns
pins or regenerates a missing corpus automatically.

The normal runner reads these artifacts and still performs the complete Core
cryptographic/state-transition replay and the existing refusal checks. Bootstrap
signatures created while loading are counted separately from producer statistics.
Generation counters in the corpus are provenance, not current-run measurements.

## Explicit regeneration

The generator target `nano-capella-canonical-corpus` is `EXCLUDE_FROM_ALL`.
Build it explicitly, then invoke its binary in two separate processes:

```sh
nano-capella-canonical-corpus --complete /absolute/new/complete
nano-capella-canonical-corpus --stalled /absolute/new/stalled
```

Directories must be new; exports never overwrite files. Generation retains the
300-second, 128-MiB and 20,000-signature guards. Review the real exports before
updating literal pins. Source identifiers: exporter `7faf84e4`, runner `e2ccf155`;
Core source `5df` is unchanged for this dataset work.

The thin runner’s `--generate-fixtures` option preserves the older combined
generation/replay workflow. Its historical AMD64 300-second failure remains
FAILED. Dataset loading does not qualify generation performance or that older
workflow. Network operation, campaigns and production readiness require their
own evidence.
