// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

import {IERC20} from "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import {ReentrancyGuard} from "@openzeppelin/contracts/utils/ReentrancyGuard.sol";
import {BridgeTypes, IBridgeMessageVerifier} from "./API.sol";

/// @notice Immutable message routing and replay accounting, not a finality verifier.
/// @dev Not deployable: an integrating route MUST implement its enablement gate.
abstract contract BaseBridgeEndpoint is ReentrancyGuard {
    error InvalidConfiguration();
    error WrongLocalChain();
    error LocalContractUnavailable();
    error InvalidAmountOrRecipient();
    error InvalidMessageRoute();
    error InvalidProofSize();
    error InvalidProof();
    error SourceAlreadyConsumed();
    error OutstandingLimit();
    error InsufficientOutstanding();
    error TokenAccountingMismatch();

    uint256 public immutable localChainId;
    bytes32 public immutable localDomain;
    uint256 public immutable remoteChainId;
    bytes32 public immutable remoteDomain;
    address public immutable remoteEndpoint;
    IERC20 public immutable localToken;
    address public immutable remoteToken;
    bytes32 public immutable inboundVerifierDomain;
    bytes32 public immutable outboundVerifierDomain;
    IBridgeMessageVerifier public immutable verifier;
    uint256 public immutable maxProofBytes;
    uint256 public immutable maxOutstanding;

    uint256 public nextNonce = 1;
    uint256 public outstanding;
    mapping(uint256 => bytes32) public outbox;
    mapping(bytes32 => bool) public consumed;

    constructor(BridgeTypes.EndpointConfig memory config) {
        if (
            config.localChainId == 0 || config.localChainId != block.chainid ||
            config.remoteChainId == 0 || config.localDomain == bytes32(0) ||
            config.remoteDomain == bytes32(0) || config.remoteEndpoint == address(0) ||
            config.localToken == address(0) || config.remoteToken == address(0) ||
            config.inboundVerifierDomain == bytes32(0) ||
            config.outboundVerifierDomain == bytes32(0) ||
            config.maxProofBytes == 0 || config.maxOutstanding == 0 ||
            (config.localChainId == config.remoteChainId &&
             config.localDomain == config.remoteDomain &&
             config.remoteEndpoint == address(this))
        ) revert InvalidConfiguration();
        // Remote addresses belong to the other domain. Their local code is not
        // evidence for or against the remote contracts, so do not inspect it.
        if (config.localToken.code.length == 0 || config.verifier.code.length == 0) {
            revert LocalContractUnavailable();
        }
        localChainId = config.localChainId;
        localDomain = config.localDomain;
        remoteChainId = config.remoteChainId;
        remoteDomain = config.remoteDomain;
        remoteEndpoint = config.remoteEndpoint;
        localToken = IERC20(config.localToken);
        remoteToken = config.remoteToken;
        inboundVerifierDomain = config.inboundVerifierDomain;
        outboundVerifierDomain = config.outboundVerifierDomain;
        verifier = IBridgeMessageVerifier(config.verifier);
        maxProofBytes = config.maxProofBytes;
        maxOutstanding = config.maxOutstanding;
    }

    /// @dev Must reject BOTH initiating and completing transfers until the route
    /// has its explicitly selected authentication, terms and governance in place.
    function _requireRouteEnabled() internal view virtual;

    function _checkRouteEnabled() internal view {
        if (block.chainid != localChainId) revert WrongLocalChain();
        if (address(localToken).code.length == 0 || address(verifier).code.length == 0) {
            revert LocalContractUnavailable();
        }
        _requireRouteEnabled();
    }

    function messageDigest(BridgeTypes.Message calldata message) external pure returns (bytes32) {
        return BridgeTypes.digest(message);
    }

    function sourceKey(BridgeTypes.Message calldata message) external pure returns (bytes32) {
        return BridgeTypes.sourceKey(message);
    }

    function _increaseOutstanding(uint256 amount) internal {
        if (amount > maxOutstanding - outstanding) revert OutstandingLimit();
        outstanding += amount;
    }

    function _decreaseOutstanding(uint256 amount) internal {
        if (amount > outstanding) revert InsufficientOutstanding();
        outstanding -= amount;
    }

    function _recordOutbound(BridgeTypes.Kind kind, uint256 amount, address recipient)
        internal returns (BridgeTypes.Message memory message, bytes32 digest, bytes32 sourceKey)
    {
        if (amount == 0 || recipient == address(0) || recipient == remoteEndpoint) {
            revert InvalidAmountOrRecipient();
        }
        uint256 nonce = nextNonce;
        nextNonce = nonce + 1; // Checked: never wrap or reset a source nonce.
        message = BridgeTypes.Message({
            kind: kind,
            sourceChainId: localChainId,
            sourceDomain: localDomain,
            sourceEndpoint: address(this),
            destinationChainId: remoteChainId,
            destinationDomain: remoteDomain,
            destinationEndpoint: remoteEndpoint,
            sourceToken: address(localToken),
            destinationToken: remoteToken,
            nonce: nonce,
            sender: msg.sender,
            recipient: recipient,
            amount: amount,
            verifierDomain: outboundVerifierDomain
        });
        digest = BridgeTypes.digest(message);
        sourceKey = BridgeTypes.sourceKey(message);
        outbox[nonce] = digest;
    }

    function _consumeInbound(BridgeTypes.Message calldata message, bytes calldata proof,
        BridgeTypes.Kind expectedKind) internal returns (bytes32 digest, bytes32 sourceKey)
    {
        if (
            message.kind != expectedKind ||
            message.sourceChainId != remoteChainId || message.sourceDomain != remoteDomain ||
            message.sourceEndpoint != remoteEndpoint ||
            message.destinationChainId != localChainId || message.destinationDomain != localDomain ||
            message.destinationEndpoint != address(this) ||
            message.sourceToken != remoteToken || message.destinationToken != address(localToken) ||
            message.verifierDomain != inboundVerifierDomain || message.nonce == 0 ||
            message.sender == address(0)
        ) revert InvalidMessageRoute();
        if (message.amount == 0 || message.recipient == address(0) || message.recipient == address(this)) {
            revert InvalidAmountOrRecipient();
        }
        if (proof.length == 0 || proof.length > maxProofBytes) revert InvalidProofSize();
        sourceKey = BridgeTypes.sourceKey(message);
        if (consumed[sourceKey]) revert SourceAlreadyConsumed();
        digest = BridgeTypes.digest(message);
        if (!verifier.verifyMessage(digest, proof)) revert InvalidProof();
        // Set before any token effect. A later revert atomically undoes it.
        consumed[sourceKey] = true;
    }
}
