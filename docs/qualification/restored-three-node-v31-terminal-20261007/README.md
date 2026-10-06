# V31 terminal — 7 October 2026

Original controller50685 exited1 at22:47:46UTC, after the fixed900-second finality deadline. Readiness passed on all three at22:32:41UTC. Zero of three required distinct advancing common checkpoints. Finalized epoch1284/root0xccf88d18b2b98689ce47814d966ab42d10898eddd83a31bb2e3873ac5b2a2c48 remains unchanged. No restart, binary replacement, deadline extension, weakened criterion or controller change during the campaign.

The restored owners executed duties: native16 keys/45 proposals/306 attestations/1156 sync messages; Docker Mac16/37/302/1130; Docker VPS33/74/145/32. Clocks and validators healthy in all stop receipts. Native exit0 and both Docker containers exited0/noOOM. Max poll3381/6402/8475ms; these observations are not a paired performance benchmark. The one-CPU isolated ARM64 relay fixtures overlapped part of V31 and are explicitly recorded in summary.json.

Core7329b7ad and relay e33 stayed frozen; controller checkout87222356. Source checkout was subsequently advanced to0d21e782 for diagnostics and tests only, not loaded into active processes. All controller hashes remain identical. Complete receipts, terminal result, artifact identity and raw-log hashes retained. Databases, configs, keys, journals, independent witnesses and private file hashes stay on their host.

Relay traces show partial dependency admission, not a full catch-up: native17 block-category turns, Docker Mac16, VPS13; quantum yields7/7/3 and historical-head-unconfirmed8/8/5. One native turn reports an admitted selected head, but no common advancing finality follows. Local depth-limit/refusal/yield traces must not be confused with peer equivocation or an admitted head on all three. New five-mode4096-block modeled-receiver tests and precise progress diagnostics are qualified separately; an empty historical catalog/model does not qualify this WAN behavior.

Fresh post-stop independent capture/pin audits are required because V31 signed and published new state. Those audits have been launched separately; no capture from before V31 may substitute for them. No new writer has been launched. Next work must address measured catch-up scheduling/throughput and signed-operation admission while retaining validation, bounded resources and historical votes.

Network recovery gate remains FAILED. All remaining production, EVM fork/profile, Base/native reserve/conversion/redemption, bridge, Doppler and independent-review criteria remain open. No funds, sale or public deployment.
