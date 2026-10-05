# Restored v7 with bounded depth fallback

Independent post-v6 read-only audit capsules provide exact core and owned signer commitments. Mac sources remain separate; all copied files except peer.key match their own source. VPS reuses its stopped audited volume without database transfer. No journal merge/reset.

Core b5ee00ac unchanged. P2P 81e5ab31 binaries from qualified native/ARM64/AMD64 builds replace only the relay executable. The controller enables existing NANO_TEST_NETWORK_TRACE diagnostics on all three fixture nodes to expose relay refusals/yields and actual admission ACKs. No changes to admission rules, protocol budgets or 900-second deadline. Three distinct common finalized advances plus stable observations required.

Preparation is not a launched or qualified network; prior v6 failure remains preserved. No production qualification or public launch.
