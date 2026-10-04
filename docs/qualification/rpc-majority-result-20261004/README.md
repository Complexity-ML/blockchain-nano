# Majority island — failed qualification, 4 October 2026

Runtime `d1597d9a6987d0839ed2f0c7ded268aead35d071`; driver `ba7bac4f038827c2d4dabcfac3695b136c907205`. Fresh native Mac / Docker Mac / Docker VPS campaign using qualified immutable artifacts, with no rebuild. Controller terminal exit **1**, runtime/source hashes unchanged.

The SSZ-bound effective stake is 43/64 on the connected Mac island and 21/64 on the isolated VPS. Direct Mac peer identities remain connected throughout the observed cut. VPS Docker network disconnected for 192000 ms; isolated observations show zero VPS peers. Conservative pre-cut commitment ceiling fixed at epoch **5**. Majority finality advances **2→4**, justification reaches **6**, last head slot **64**; minority finality stays **2**. No conflicting finalized roots observed. This fails the unchanged requirement to finalize beyond epoch5 during the cut.

The network is reattached in `finally`; all signers report healthy terminal state. The failing cut gate stops the campaign before recovery and three-epoch stability qualification. Reconnection alone proves neither. The post-restore scope correction is not qualified by this earlier-exiting run.

Cause not established. Global effective stake does not by itself prove timely per-epoch target participation and inclusion; missing proposals and boundary inclusion must be reconstructed from persisted states before attributing this failure to C++ or test duration. No threshold, quorum, recovery budget or original verdict changed. Do not repeat the campaign until diagnostic evidence motivates a change.

`timeline.json` contains all partition snapshots, `result.json` the explicit failed verdict, `archive.json` the local public evidence archive identity. Complete stopped datasets remain in Docker and verified compressed backups outside Docker. The archive excludes private keys and databases. Physical scale, sustained asymmetric latency, validator lifecycle, load, public fork activation, bridge and independent review remain open. Production is not qualified.
