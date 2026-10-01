# Durable alert delivery — arm64

31 tests passed, zero failed/skipped: 10 new delivery/configuration cases and 21 existing monitor cases. Synthetic monitor reports and local HTTP receivers; no live Nano daemon or external recipient.

Coverage: durable enqueue before POST, retry after 503/restart, identical idempotency key after receiver acceptance and local ACK failure, recovery, duplicate suppression, response/timeout budgets, refusal of redirects, corrupt/cross-monitor state, full queue and explicit drain. Monitor identity is derived from reviewed configuration. File and directory fsync are implemented; physical power loss is not qualified.

Node v25.9.0. Docker ARM64 uses the existing builder `sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4`, network none, 2 CPUs/512 MiB, repository and temporary official Node executable read-only. Native uses the Mac runtime.

Base commit ecf927871bd4e01d53345f057b5ea7cefe61586b; exact tested source hashes are in ../source-hashes.json. Runtime C++ unchanged. AMD64, live monitor-to-receiver campaign, scheduler and actual operator destination remain open.
