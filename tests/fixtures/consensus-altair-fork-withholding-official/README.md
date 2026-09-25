# Altair withholding event corpus

All 2 minimal scenarios from ethereum/consensus-spec-tests at
bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908 (v1.6.0-beta.0).
Original YAML/Snappy bytes, decoded SSZ and PyYAML 6.0.3 JSON conversions
are preserved with per-file sizes and SHA-256 in manifest.json.
Shared pinned references and historical time/boost configuration are verified
from ../consensus-altair-fork-head-official/ by the common C++ runner.

Only the minimal preset is covered, not mainnet-only scenarios. Tests check
published event validity and branch/checkpoint expectations with BLS enabled.
They are deterministic in-memory replay, not distributed attack simulation,
persistent recovery, resource qualification or network consensus activation.
