# Sync admission context — amd64 local qualification, 2026-10-01

Both Docker VPS executions terminated exit 0 on frozen source 67c4103 (code equivalent to cache implementation 7bba333). Local-sync: both new cache scenarios pass, 40 blocks, finality 3, two stores and 64 signing journals reopened. Rotation: 216 blocks, 1680 votes, 6944 sync messages, three rotations, 215 verified aggregates, finality 25, witness restart at slot 127, duplicate poll and backwards-clock protections preserved.

The canonical audit covers 217 blocks and 247 transactions. It explicitly does not qualify historical fork choice or standalone vote replay. These are isolated local transition tests, without network transport. No WAN improvement, resolution of prior failures, or production completion is claimed.
