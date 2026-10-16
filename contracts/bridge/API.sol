// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

import {IERC20} from "@openzeppelin/contracts/token/ERC20/IERC20.sol";

/// @notice Shared wire format only. It authenticates neither a chain nor a proof.
library BridgeTypes {
    enum Kind { LOCK, BURN }

    struct Message {
        Kind kind;
        uint256 sourceChainId;
        bytes32 sourceDomain;
        address sourceEndpoint;
        uint256 destinationChainId;
        bytes32 destinationDomain;
        address destinationEndpoint;
        address sourceToken;
        address destinationToken;
        uint256 nonce;
        address sender;
        address recipient;
        uint256 amount;
        bytes32 verifierDomain;
    }

    struct EndpointConfig {
        uint256 localChainId;
        bytes32 localDomain;
        uint256 remoteChainId;
        bytes32 remoteDomain;
        address remoteEndpoint;
        address localToken;
        address remoteToken;
        bytes32 inboundVerifierDomain;
        bytes32 outboundVerifierDomain;
        address verifier;
        uint256 maxProofBytes;
        uint256 maxOutstanding;
    }

    bytes32 internal constant TYPEHASH = keccak256(
        "NanoBridgeMessageV1(uint8 kind,uint256 sourceChainId,bytes32 sourceDomain,address sourceEndpoint,uint256 destinationChainId,bytes32 destinationDomain,address destinationEndpoint,address sourceToken,address destinationToken,uint256 nonce,address sender,address recipient,uint256 amount,bytes32 verifierDomain)"
    );

    function digest(Message memory message) internal pure returns (bytes32) {
        // Message contains only static ABI fields: this is TYPEHASH followed by
        // every field, in the declared order, without offsets or packed encoding.
        return keccak256(abi.encode(TYPEHASH, message));
    }

    function sourceKey(Message memory message) internal pure returns (bytes32) {
        // Intentionally independent of recipient/amount/tokens/destination.
        // A verifier approving two payloads for one source nonce cannot make
        // both consumable at this endpoint.
        return keccak256(abi.encode(
            message.kind, message.sourceChainId, message.sourceDomain,
            message.sourceEndpoint, message.nonce
        ));
    }
}

/// @dev A production implementation is deliberately absent. It must bind the
/// exact digest to authorized source-event inclusion and finality/canonicality.
interface IBridgeMessageVerifier {
    function verifyMessage(bytes32 digest, bytes calldata proof) external view returns (bool);
}

/// @dev Adapter boundary, not an ERC-20 implementation or a USDC designation.
/// A FiatToken route must use the official contracts and a reviewed adapter.
interface IBridgeMintBurnToken is IERC20 {
    function mint(address recipient, uint256 amount) external returns (bool);
    function burn(uint256 amount) external;
}
