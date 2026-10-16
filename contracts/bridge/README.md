# Common bridge contracts — source proposal, no active route

Four Solidity 0.8.28 sources implement a common ERC-20 lock/mint/burn/release protocol. They are uncompiled and unexecuted by the author. No R file, SDK, private input, wallet, deployment, transaction or network was used. All three endpoint contracts are **abstract**. No proof verifier, production route, token implementation, administrator, proxy, pause policy, quorum or relayer is supplied.

`source/contracts/bridge/API.sol` is the single canonical definition file for `BridgeTypes`, `IBridgeMessageVerifier` and `IBridgeMintBurnToken`; there are no duplicate split definitions. Imports use OpenZeppelin **5.0.2**, whose public sources already exist at `tests/contracts/openzeppelin-v5.0.2/contracts` in R. Root supplies the guarded compiler/import mapping and performs all compilation/testing separately.

## API locked for the independent harness

`BridgeTypes.Kind` is `LOCK = 0`, `BURN = 1`. `Message` fields, in ABI order:

```text
kind, sourceChainId, sourceDomain, sourceEndpoint,
destinationChainId, destinationDomain, destinationEndpoint,
sourceToken, destinationToken, nonce, sender, recipient, amount, verifierDomain
```

Chain IDs, nonce and amount are `uint256`; domains are `bytes32`; other fields are addresses. `kind` is encoded as `uint8`. The type hash is Keccak-256 of this exact string:

```text
NanoBridgeMessageV1(uint8 kind,uint256 sourceChainId,bytes32 sourceDomain,address sourceEndpoint,uint256 destinationChainId,bytes32 destinationDomain,address destinationEndpoint,address sourceToken,address destinationToken,uint256 nonce,address sender,address recipient,uint256 amount,bytes32 verifierDomain)
```

`BridgeTypes.digest(m)` is `keccak256(abi.encode(TYPEHASH, m))`. All 14 fields are static, so the ABI bytes are the type hash followed by those fields in order. This is an event/proof commitment, **not** an EIP-712 wallet-signing envelope. `sourceKey(m)` is `keccak256(abi.encode(m.kind, m.sourceChainId, m.sourceDomain, m.sourceEndpoint, m.nonce))`; it intentionally omits payment, token and destination fields. Two approved contradictory payloads for the same source emission cannot both be consumed by one endpoint.

Each constructor receives one `BridgeTypes.EndpointConfig` struct, in this order:

```text
localChainId, localDomain, remoteChainId, remoteDomain, remoteEndpoint,
localToken, remoteToken, inboundVerifierDomain, outboundVerifierDomain,
verifier, maxProofBytes, maxOutstanding
```

Both `CanonicalEscrow(config)` and `MintBurnGateway(config)` inherit `BaseBridgeEndpoint(config)`. The integration must override `function _requireRouteEnabled() internal view virtual;`. Every state-changing protocol entry checks the current `block.chainid`, the existence of local token/verifier code and this hook **before** token IO or bridge accounting. A missing authenticator must keep both initiation (`lock`, `burnForRelease`) and completion (`mint`, `release`) disabled. There is no default enabled implementation.

Public APIs:

- Escrow: `lock(uint256 amount, address recipient) returns (bytes32 digest)`; `release(Message calldata message, bytes calldata proof)`.
- Gateway: `mint(Message calldata message, bytes calldata proof)`; `burnForRelease(uint256 amount, address recipient) returns (bytes32 digest)`.
- Both: pure `messageDigest(Message)` and `sourceKey(Message)`; getters for configuration, `outstanding`, `nextNonce`, `outbox(uint256)` and `consumed(bytes32)`.
- Events: `Locked`, `Released`, `Minted`, `Burned`, each with `(bytes32 indexed digest, bytes32 indexed sourceKey, Message message)`. No event substitutes for a successful receipt or authenticated source inclusion.

## State and conservation checks

The contract, never a relayer, allocates outbound nonces starting at 1. `outbox[nonce]` records the full digest; checked arithmetic prevents wraparound. The source key is consumed permanently after verification and before token effects. A later revert rolls back the consumed flag, token effects, accounting, nonce/outbox changes and all emitted success logs.

Inbound routes require the exact remote chain/domain/endpoint/token, local destination chain/domain/endpoint/token, kind and inbound verifier domain. Amount, sender, recipient and nonce must be nonzero; the destination endpoint itself is not a valid recipient. Outbound recipients must be nonzero and differ from the **remote** endpoint. Remote token/endpoint code is not checked on the wrong chain. Equal numeric endpoint/token addresses on distinct domain tuples are valid; only the identical complete endpoint tuple is refused.

Escrow `outstanding` is the amount actually locked minus successful releases. Locks require exact sender debit and escrow credit; releases require exact escrow debit and recipient credit. The real escrow balance must cover all accounted outstanding reserves before and after release. Unsolicited donations do not increase the reserve counter or create a mint entitlement. False/no-op/taxed transfers that fail these deltas revert; OpenZeppelin SafeERC20 handles false return values and reverts while preserving standard empty-return support. Tokens with rebases or hostile/unreviewed semantics are not qualified by these checks.

Gateway `outstanding` is the amount minted **by this gateway** minus its own successful burns. Before either mint or burn, the gateway requires `totalSupply() == outstanding` on chain. Because outstanding starts at zero, a representation with a pre-existing supply cannot enter this protocol; there is no legacy-supply import. Additional issuance or outside burning that breaks this equality makes the gateway refuse, rather than allowing inflated tokens to consume the honest reserve. Mint then checks the token's boolean success, exact recipient credit and exact total-supply increase. Burn pulls the exact amount from the caller, checks the intermediate gateway credit/sender debit and unchanged supply, burns its own balance, then checks the supply decrease, retained gateway balance and unchanged sender balance. The initial equality, checked deltas and updated counter imply the same equality after success. `IBridgeMintBurnToken` is IERC20 plus `mint(address,uint256) returns (bool)` and `burn(uint256)` returning no value. Both modules enforce an immutable, positive `maxOutstanding`; inbound proof bytes must be nonempty and no larger than immutable `maxProofBytes`.

## Deliberately unimplemented security and activation

`IBridgeMessageVerifier.verifyMessage(bytes32 digest, bytes calldata proof) external view returns (bool)` is only a boundary. A real implementation must authenticate inclusion/finality of the exact source event and the authorized source protocol. A permissive mock or a relayer signature is not such a proof. The existing `tools/bridge-source` observation remains `eligibleForMint:false`; this proposal does not change it. Copied chain IDs/genesis/deployment domains do not distinguish a fully cloned fork: canonical checkpoints and compromised-quorum assumptions remain responsibilities of the chosen verifier/governance.

The supply equality does **not** prove that the token has no other minter/burner, that arbitrary token storage is truthful, or that a proxy will retain its implementation/roles. The representation must start at zero supply and reserve issuance/burning to its gateway; outside powers that break supply accounting can cause denial of service. The hook cannot repair that by silently rebasing outstanding. A real route must separately qualify all token, verifier and upgrade authorities before activation; this does not resist a fully compromised token or proof authority. There is no initializer for importing old supply/reserves and no migration/reset of `consumed`.

This protocol uses the same integer base-unit amount on the canonical and representation legs; that is a mechanical test interface, **not a NANO/USDC rate or approval of any economic conversion**. `WrappedNano` is unchanged and has no remote mint path. No native-currency escrow, fee, rounding, timeout refund, reserve rescue or price mechanism is supplied. Failed or delayed relaying may be retried with the same ID; expiry alone must not authorize a second payment or remint.

For USDC, this is not a replacement token or a compliant production bridge. The mint/burn interface is compatible with the shape of official FiatToken, but official code/proxy, Circle compiler/deployment requirements, roles, supply reconciliation, optional coordinated reserve burn/role transfer and explicit upgrade governance remain separate. These immutable constructor-based endpoints are **not proxy-ready upgrade implementations**; do not put them behind a proxy or claim upgrade-state preservation. The hook alone does not supply those missing procedures or Circle agreement.

Root's next bounded synthetic qualification can independently compare both hashes, enforce disabled routes, exercise a round trip, payload/dedup attacks, caps and all token/verifier failure/reentrancy rollback paths. Any mocks, activation switches and synthetic tokens must remain in the tests namespace. No such qualification has been run by the author and no production claim is made.
