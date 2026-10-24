# Independent inventory oracle

`inventory-model.mjs` recomputes an independent journal with exact BigInt atomic units. Its seven new test groups passed in the existing isolated AMD64 Docker image on candidate1b260fbb; the implementation, tests and vectors are byte-identical in the delivered candidate. See the [scoped evidence](../../docs/qualification/closed-inventory-v2-20261025/README.md).

It models full preissued-supply backing, pending/quarantined/reserved inventory, independently refundable funding, source-nonce exclusivity, out-of-order delivery, cancellation, donations and issuer mismatch. Failed transitions preserve the complete snapshot. The model imports no contract implementation.

The standalone vector/proposal examples use2:7 atomic lots, including explicit6/18 decimals. The separately executed Solidity trace instantiates this same model with2:3 lots and compares22 checkpoints. Neither example adopts economic terms. This oracle is test tooling; it authenticates no source finality, real reserve, Nano runtime or production bridge.
