# Packaged Nano2 stopped-copy restoration — 2026-10-05

The actual AMD64 candidate archived the complete offline Nano2 fixture, including peer identity, chain witness and validator journals/witnesses. Original source mounted read-only. The canonical audit passed with the independently retained request: 77 blocks, 59 pruned execution states replayed, checkpoint 827 and expected finalized epoch 8/root.

Only the fresh extracted copy changed ownership to 10001:10001. Its content manifests remained identical. A second read-only audit under UID 10001 produced byte-identical JSON; independent full file hashes under UID 10001 matched the post-restore manifest. Archive/private keys/databases are not committed.

Failures retained: first test rejected the wrong mount because profile/anchor files were absent; the corrected fixture used the same source, request and image. The corrected controller later exited **123** when its root observer, with capabilities dropped, could not read peer.key after ownership changed. Do not call that controller successful. The follow-up verification under the actual owner exited **0**, without repeating restore/audit or altering data. The harness now mounts the copy directly for the rootless final audit instead of traversing the root-only proof folder. Initial unit-path validation failure is also preserved; affected 62-test selection passes after correction.

This proves offline packaged restore/audit/ownership within the retained synthetic fixture. No duties, network recovery, power-loss recovery or advancing finality is claimed. Nano2 WAN campaign, native runtime provenance and other production criteria remain open. No image rebuild was necessary for JavaScript/shell changes.
