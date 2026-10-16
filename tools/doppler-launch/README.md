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

With `BASE_RPC_URL`, the adapter snapshots the reviewed terms and reference header,
checks chain 8453, deployed module bytecode and native USDC decimals, then simulates
from the creator. Every code/call request uses `{blockHash, requireCanonical:true}`.
Gas estimation requires the SAME hash selector as a provider extension; unlike
`eth_getCode`/`eth_call`, this extension is not standardized by EIP-1898. Unsupported
providers are rejected without a block-number/latest fallback. Only a complete
successful pass followed by the header fence sets `referenceHashPinned:true`.
This does not prove cryptographic finality or future transaction execution. Only
chain/block/code/call/estimate RPC methods are allowed. Module hashes are recorded
observations, not comparison to an audited allowlist. The shared bounded transport
uses the fixed five-method `createLaunchRpc` factory; `createSourceRpc` retains
its original four observer methods. HTTPS (HTTP loopback only), no redirects or
retries, matching JSON-RPC IDs and exactly one result/error are required. Each
request defaults to 10 seconds including its body and 1 MiB actual response bytes.
Rejection diagnostics retain only method, safe-integer code and revert hex up to
4 KiB, without provider messages or endpoint URLs. No wallet/sign/send method is
available. These are per-request limits, not a whole-preview deadline. A rejected preflight/simulation is saved as `simulation-rejected` and the
CLI exits 1. Offline encoding and mocked RPC tests do not prove on-chain acceptance.
The synthetic fixture reverted on Base during the first real read-only attempt;
no transaction was sent. The rejection was decoded as `InvalidGamma` (0x248d3d6a). The SDK automatic
value 120 failed the on-chain constructor; an explicit synthetic value 20040
passed creation simulation and gas estimation on Base (10,912,762 gas). Token
and hook return addresses match SDK predictions. Both failed and successful
review artifacts are retained under `docs/qualification/doppler-unsigned-preview-20261004`.
Those historical simulations used height selectors. On 2026-10-16, ten bounded
public reads on mainnet.base.org observed getCode/call/estimateGas using the same
canonical-hash selector, with unknown hashes refused and the reference fence
matching. One fresh public synthetic fixture then passed the current strict
creation simulation and estimation: 12 read-only requests, matching predicted
token/hook addresses, 10,912,762 gas. Only its reference timestamp was refreshed.
Root's evidence wrapper subsequently exited 1 on its final BigInt console output;
the successful reports were validated separately without replay. See the
[scoped report and retained failure](../../docs/qualification/pruning-base-hash-20261016/README.md).
This does not qualify arbitrary historical state, a known orphan during a real
reorg, the bridge or actual launch terms. Hash estimation remains a provider
extension outside EIP-1898. Real economic terms still need configuration and qualification.

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

## Private Anvil lifecycle

For the synthetic local lifecycle, start an owned Anvil instance with chain ID
31337 and loopback binding, then run the script with a new output directory:

```sh
anvil --host 127.0.0.1 --port 19546 --chain-id 31337 --fork-url https://mainnet.base.org --fork-block-number 52173454 --no-storage-caching --silent
node fork-lifecycle.mjs /tmp/doppler-lifecycle-new
```

The script only writes to this hardcoded local RPC and requires Anvil/chain
31337 before funding, impersonation or transactions. It mints synthetic USDC
by impersonating the master minter on the local fork, buys, sells, advances
fixture time and migrates to V2. It checks receipt status, token/USDC deltas,
net proceeds, migration event, pair currencies, reserves and beneficiary LP.
Stop the owned Anvil instance afterward. These are synthetic terms, not
Nanobees launch parameters or native Nano qualification. No real private key
is used. The router package is pinned and imported through explicit .js files
because its published index has extensionless ESM imports.

The failed terminal assertion in attempt B remains recorded; attempt C uses
the corrected reserve/event/LP verification and completed the entire cycle.

Le cycle privé comprend aussi trois transactions adversariales : migration
prématurée, transfert de propriété non autorisé, double migration. Chaque refus
exige un sélecteur attendu, un reçu reverted sans épuisement du gas et des états,
soldes, réserves et LP identiques avant/après. Couverture déterministe limitée,
pas une qualification des bridges ou de Nano.

Le vérificateur lie reçu, logs et lectures d'état au hash canonique du bloc via
EIP-1898 et refuse un changement de header, sans fallback par hauteur. Cela reste
une assertion RPC, pas une preuve cryptographique de finalité ou de bridge.
Le scénario Anvil fixe ses timestamps pour que le devis et l'exécution ne
franchissent pas une époque ; la sortie minimale reste 99 % du devis.

Migration LP verification (historical Base V2 only): the bound receipt must show
an ordered, unambiguous mint to the configured migrator and a positive transfer
to its beneficiary. All beneficiary debits in the receipt reduce the attributable
credit; pre-existing balances, unrelated donations and self-transfers do not
substitute for it. Malformed pair Transfer logs are rejected. The six canonical
hash reads and header fences remain unchanged. The scoped 29-test result, causal
baseline and earlier refusals are recorded in
[the qualification report](../../docs/qualification/doppler-lp-attribution-20261016/README.md).
This does not qualify real Base migrations, other migrators, Nano or a bridge.
