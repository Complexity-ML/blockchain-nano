# Live Nano outage → durable alert → recovery

Complete run terminal exit 0, harness e32dce426522ec44ea734b7120da1638fbae6482, unchanged runtime candidate 74784c4. Native Mac + Docker ARM64 Mac + Docker AMD64 VPS, two physical hosts, 64 distributed validator keys. Images/binary SHA-256 verified against candidate manifest. No new images, public RPC, funds or load injector.

Actual VPS Nano/P2P process tree paused with Docker: observed pause 1,892 ms between recorded confirmations. Monitor detected unavailable VPS. Local HTTP receiver rejected first POST with 503; queued event survived, retry used identical eventId and was accepted. After unpause, healthy recovery accepted. Three attempts, two logical accepted notifications. This receiver is a test fixture, not a provisioned external operator service.

Following independent strict execution-bound gate: common finality 2 → 3 → 4 → 5, 134,283 ms. Shadow monitor: 23 healthy observations, each node advanced 2 → 5, wrong fixture genesis rejected. Terminal validators healthy with admitted attestations native 137, ARM64 134, AMD64 137. Both containers exited 0, no OOM/no pause; native PID 70350 absent.

Scope: one brief live process pause, alert queue HTTP rejection/retry and recovery, subsequent shared-finality window. Not network partition, long outage, daemon SIGKILL, physical I/O power loss, production scheduler, external destination authentication, universal exactly-once delivery or full production readiness. Separate supervisor SIGKILL tests remain in monitor-handoff-20261001. Independent security review and bridge/launch remain open.

Only public reports, runtime logs/stop receipts, hashes and source are archived. No databases, validator private keys, environment files, signing journals or recovery witnesses.
