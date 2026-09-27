# Automatic Docker recovery — PASSED

Two local Docker nodes, 32 synthetic validators each, Capella/Shanghai.
The automatic witness policy is enabled before first boot on the crashed node.
Baseline common finality epoch 2; SIGKILL of the complete container; persistent
volume retained; offline 96.757 seconds. The half-stake survivor
keeps its finalized checkpoint while its head advances.

Restart retains the container, P2P identity and pre-crash finalized header.
The configuration SHA256 is unchanged through recovery, with initial snapshot
still configured. Readiness/header verification takes 2.022s;
common finalized epoch 7 is qualified after 143.11s from restart
request preparation, including three observations and finalized-header checks.
These are one-run measurements, not an availability SLA.

Final root: `0x3759f85bf75e70826113d84b0d7f4c9e51ed64255bb462594cc07bd0aba86ca5`.
No contradictory finalized checkpoint observed. Logs retain transient heads,
refusals and transport events. Harness exits 0; summary PASSED. Containers and
network removed by harness; independent inventories empty, crashed volume absent.

This qualifies the unchanged-configuration SIGKILL/restart path for one local
pair. It does not prove physical power-loss safety, independent witness rollback
protection, long outages, WAN, heavy DeFi traffic, many-peer fairness or overall
production readiness. No VPS change, public deployment or real funds.

Reproduce with `node tests/capella/docker-partition.mjs EXPORTER NEW_OUT IMAGE REV automatic-restart`.
The image carries base revision aeafb6f and a dirty-source label; manifest records
its ID and hashes. Public synthetic fixture keys are test-only.
