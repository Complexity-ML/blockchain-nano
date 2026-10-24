// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

/// @notice Closed prototype wire format. Neither a proof nor adopted economics.
library InventoryBridgeV2 {
    enum Kind {
        BOOTSTRAP, ACK_BOOTSTRAP, LOCK_BASE, ACK_NATIVE_PAID,
        RESERVE_BASE, FUND_NATIVE, ACK_BASE_PAID, REQUEST_CANCEL,
        ACK_CANCEL_UNFUNDED, CANCEL_FUNDED, ACK_NATIVE_REFUNDED, FREEZE_SUPPLY
    }
    struct Terms {
        uint256 baseChainId;
        bytes32 baseDomain;
        address baseEndpoint;
        address baseToken;
        uint256 nativeChainId;
        bytes32 nativeDomain;
        address nativeEndpoint;
        address nativeAsset;
        uint256 totalBaseSupply;
        uint256 baseLot;
        uint256 nativeLot;
        bytes32 baseToNativeVerifierDomain;
        bytes32 nativeToBaseVerifierDomain;
        uint256 maxProofBytes;
    }
    struct Message {
        Kind kind;
        bytes32 routeId;
        bytes32 termsHash;
        uint256 sourceChainId;
        bytes32 sourceDomain;
        address sourceEndpoint;
        uint256 destinationChainId;
        bytes32 destinationDomain;
        address destinationEndpoint;
        uint256 nonce;
        bytes32 transferId;
        address sender;
        address recipient;
        uint256 baseAmount;
        uint256 nativeAmount;
        bytes32 verifierDomain;
    }
    bytes32 internal constant TERMS_TYPEHASH = keccak256("NanoInventoryTermsV2(uint256 baseChainId,bytes32 baseDomain,address baseEndpoint,address baseToken,uint256 nativeChainId,bytes32 nativeDomain,address nativeEndpoint,address nativeAsset,uint256 totalBaseSupply,uint256 baseLot,uint256 nativeLot,bytes32 baseToNativeVerifierDomain,bytes32 nativeToBaseVerifierDomain,uint256 maxProofBytes)");
    bytes32 internal constant ROUTE_TYPEHASH = keccak256("NanoInventoryRouteV2(uint256 baseChainId,bytes32 baseDomain,address baseEndpoint,address baseToken,uint256 nativeChainId,bytes32 nativeDomain,address nativeEndpoint,address nativeAsset)");
    bytes32 internal constant MESSAGE_TYPEHASH = keccak256("NanoInventoryMessageV2(uint8 kind,bytes32 routeId,bytes32 termsHash,uint256 sourceChainId,bytes32 sourceDomain,address sourceEndpoint,uint256 destinationChainId,bytes32 destinationDomain,address destinationEndpoint,uint256 nonce,bytes32 transferId,address sender,address recipient,uint256 baseAmount,uint256 nativeAmount,bytes32 verifierDomain)");
    error InvalidTerms();
    error InexactAmount();

    function validate(Terms memory t) internal pure {
        if (t.baseChainId == 0 || t.nativeChainId == 0 ||
            t.baseDomain == bytes32(0) || t.nativeDomain == bytes32(0) || t.baseDomain == t.nativeDomain ||
            t.baseEndpoint == address(0) || t.nativeEndpoint == address(0) || t.baseToken == address(0) ||
            t.nativeAsset != address(0) || t.totalBaseSupply == 0 || t.baseLot == 0 || t.nativeLot == 0 ||
            t.totalBaseSupply % t.baseLot != 0 ||
            t.totalBaseSupply / t.baseLot > type(uint256).max / t.nativeLot ||
            t.baseToNativeVerifierDomain == bytes32(0) || t.nativeToBaseVerifierDomain == bytes32(0) ||
            t.baseToNativeVerifierDomain == t.nativeToBaseVerifierDomain ||
            t.maxProofBytes == 0 || t.maxProofBytes > 1_048_576) revert InvalidTerms();
    }
    function termsHash(Terms memory t) internal pure returns (bytes32) {
        return keccak256(abi.encode(TERMS_TYPEHASH, t));
    }
    function routeId(Terms memory t) internal pure returns (bytes32) {
        return keccak256(abi.encode(ROUTE_TYPEHASH, t.baseChainId, t.baseDomain,
            t.baseEndpoint, t.baseToken, t.nativeChainId, t.nativeDomain,
            t.nativeEndpoint, t.nativeAsset));
    }
    function quote(Terms memory t, uint256 b) internal pure returns (uint256) {
        if (b == 0 || b > t.totalBaseSupply || b % t.baseLot != 0 ||
            b / t.baseLot > type(uint256).max / t.nativeLot) revert InexactAmount();
        return b / t.baseLot * t.nativeLot;
    }
    function digest(Message memory m) internal pure returns (bytes32) {
        return keccak256(abi.encode(MESSAGE_TYPEHASH, m));
    }
    function sourceKey(Message memory m) internal pure returns (bytes32) {
        // Exclusive across kinds, terms, transfers and conflicting payloads.
        return keccak256(abi.encode(m.sourceChainId, m.sourceDomain, m.sourceEndpoint, m.nonce));
    }
}

/// @dev Deliberately no production implementation: exact event inclusion,
/// canonical finality and the authorized source protocol must be authenticated.
interface IInventoryBridgeV2Verifier {
    function verifyMessage(bytes32 digest, bytes calldata proof) external view returns (bool);
}
