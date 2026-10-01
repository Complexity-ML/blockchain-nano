# Monitor handoff — VPS Docker AMD64

Source fafd9ec3269de2705f8882a5a9f469d55267c46f pulled by Git. 35 passed, zero failed/skipped, terminal exit 0, 3116.086161 ms. Four new integration cases include actual supervisor SIGKILL and ordered fault/recovery handoff. Local synthetic RPC/receiver, no live Nano daemon in this test.

Existing builder sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c, 2 CPUs/512 MiB, network none, checkout and Node read-only. Existing official temporary Node v25.9.0 binary SHA verified before run: 7fd113446e9171a28885227b3c275ff965b2fd1f45ccf1fb3a1c20308f34e627. Exact operation sources in ../source-hashes.json. No new image.

This qualifies the tested handoff on native/ARM64/AMD64, not physical power loss, actual operator destination, scheduler, live Nano outage or overall production readiness.
