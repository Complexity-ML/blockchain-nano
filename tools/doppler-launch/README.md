# Doppler Base/USDC — unsigned launch review

This tool prepares a real Airlock `create` call through the official Doppler SDK
1.0.43. It never creates a wallet, requests a signature or broadcasts a transaction.
It does not implement the Base–Nano bridge or prove backing for the Base token.
The Dynamic configuration is separate from the C++ candidate under qualification.

```sh
cd tools/doppler-launch
npm ci --ignore-scripts
npm test
node preview.mjs example.synthetic.json /tmp/doppler-review.json
# Optional read-only on-chain preflight and simulation:
BASE_RPC_URL=https://mainnet.base.org node preview.mjs YOUR-CONFIG.json /tmp/doppler-onchain-review.json
```

`example.synthetic.json` is a synthetic SDK fixture, **not approved launch terms**.
Replace every economic/identity/timing parameter for the actual launch. Supply uses
18-decimal token base units, proceeds use 6-decimal USDC base units, fees use Uniswap
millionths, fee shares use WAD (1e18). Ticks represent the ordered raw-unit ratio,
not directly a dollar price; decimal conversion and token ordering matter.
Dynamic tick spacing must be at most 30. Gamma is mandatory and describes
total-curve movement; its exact integer normalized epoch value must be nonzero
and it must be divisible by tick spacing. The wrapper does not use SDK gamma
defaults. The official SDK validates tick direction
and mines the token/hook salt. Yearly inflation is mandatory in this wrapper;
zero in the fixture is a fixture choice, not an adopted token policy.

The review records configuration, creator, deployed SDK module addresses,
encoded calldata, calldata hash and predicted token/hook addresses. These predictions
are not a deployment. Output creation is exclusive to avoid overwriting an earlier
review. Any change to time/parameters produces a new review and requires review again.
No private key or RPC URL is recorded.

With `BASE_RPC_URL`, the adapter checks chain 8453, deployed module bytecode and
native USDC decimals, then simulates and estimates gas from the creator at a single
block number, checking its hash again. Only chain/block/code/call/estimate RPC methods
are allowed. Module hashes are recorded observations, not comparison to an audited
allowlist. A rejected preflight/simulation is saved as `simulation-rejected` and the
CLI exits 1. Offline encoding and mocked RPC tests do not prove on-chain acceptance.
The synthetic fixture reverted on Base during the first real read-only attempt;
no transaction was sent. The rejection was decoded as `InvalidGamma` (0x248d3d6a). The SDK automatic
value 120 failed the on-chain constructor; an explicit synthetic value 20040
passed creation simulation and gas estimation on Base (10,912,762 gas). Token
and hook return addresses match SDK predictions. Both failed and successful
review artifacts are retained under `docs/qualification/doppler-unsigned-preview-20261004`.
Real economic terms still need configuration and qualification.

Supported governance choices are explicit `noOp`/`launchpad`; migration choices are
V2/V4. V4 requires explicit fee beneficiaries and lock duration. Other SDK modes,
vesting, module overrides and proceeds splits are deliberately rejected rather than
silently applied. No assumption is made about USDC being available for transfer to
Nano after migration. This tool cannot establish native redemption rights, production
readiness, a sale opening or third-party indexing at T0.

References checked 2026-10-04:
- https://github.com/whetstoneresearch/doppler-sdk
- https://docs.doppler.lol/
- https://developers.circle.com/stablecoins/usdc-contract-addresses
