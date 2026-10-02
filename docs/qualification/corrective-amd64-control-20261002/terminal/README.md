# Corrective AMD64 control — 19/19 passed

Runtime source 1ac57e25ff04f1d79b5c605acc2ae8ff1d73dcc9. Nineteen exact
selected cases ran, all passed, none skipped, in 135.33 seconds. CTest,
container, result audit and post-test manifest generation exited 0. Source
manifests and seven program hashes before/after are identical. Explicit nano
and command-argument program discovery use the pinned strengthened manifest
helper from daab685d, separately recorded from the runtime revision.

The selection qualifies the RPC observation/load-window correction and the
Bellatrix local exit clock barrier on AMD64. In particular, both sustained
Paris/Shanghai RPC cases and exit pruning pass here. Original criteria remain.
This targeted success does not replace the complete c104 campaign (408/417,
nine failures), establish a full coherent candidate, qualify new WAN behavior,
or close the Altair, Bellatrix partition or five Capella failures. No runtime
is promoted to main from this evidence alone.

The confirmed permission failure before any build/test remains in ../permission-retry.
This terminal retry uses the same runtime, image, inner control and selection.
Logs are compressed with unchanged decompressed bytes; raw-sha256.json and
SHA256SUMS retain raw and archived integrity. No extra test was run to archive.
