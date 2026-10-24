// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import {ReentrancyGuard} from "@openzeppelin/contracts/utils/ReentrancyGuard.sol";
import {InventoryBridgeV2 as V2, IInventoryBridgeV2Verifier} from "./API.sol";

abstract contract InventoryEndpointV2 is ReentrancyGuard {
    error RouteDisabled();
    error WrongLocalChain();
    error InvalidMessage();
    error InvalidProof();
    error AlreadyConsumed();
    error InvalidState();
    error AccountingMismatch();
    error InvalidRecipient();
    V2.Terms internal _terms;
    bool public immutable baseSide;
    bytes32 public immutable routeId;
    bytes32 public immutable termsHash;
    IInventoryBridgeV2Verifier public immutable verifier;
    uint256 public nextNonce = 1;
    mapping(uint256 => bytes32) public outbox;
    mapping(bytes32 => bool) public consumed;
    event MessageSent(bytes32 indexed digest, bytes32 indexed sourceKey, V2.Message message);

    constructor(V2.Terms memory t, address v, bool isBase) {
        V2.validate(t);
        if (v.code.length == 0 || address(this) != (isBase ? t.baseEndpoint : t.nativeEndpoint)) revert V2.InvalidTerms();
        _terms = t; baseSide = isBase; routeId = V2.routeId(t); termsHash = V2.termsHash(t);
        verifier = IInventoryBridgeV2Verifier(v);
        _localChain();
    }
    function _requireRouteEnabled() internal view virtual;
    function _localChain() internal view {
        if (block.chainid != (baseSide ? _terms.baseChainId : _terms.nativeChainId)) revert WrongLocalChain();
    }
    function _gate() internal view {
        _localChain(); if (address(verifier).code.length == 0) revert InvalidProof();
        _requireRouteEnabled();
    }
    function terms() external view returns (V2.Terms memory) { return _terms; }
    function quoteNative(uint256 b) external view returns (uint256) { return V2.quote(_terms, b); }
    function messageDigest(V2.Message calldata m) external pure returns (bytes32) { return V2.digest(m); }
    function messageSourceKey(V2.Message calldata m) external pure returns (bytes32) { return V2.sourceKey(m); }
    function _newTransferId(V2.Kind kind) internal view returns (bytes32) {
        return keccak256(abi.encode(routeId, termsHash, address(this), nextNonce, kind));
    }
    function _pair(V2.Message memory m) internal view {
        if (V2.quote(_terms, m.baseAmount) != m.nativeAmount) revert InvalidMessage();
    }
    function _same(V2.Message memory m, address sender, address recipient, uint256 b, uint256 n) internal pure {
        if (m.sender != sender || m.recipient != recipient || m.baseAmount != b || m.nativeAmount != n) revert InvalidMessage();
    }
    function _consume(V2.Message calldata m, bytes calldata proof, V2.Kind kind) internal {
        if (m.kind != kind || m.routeId != routeId || m.termsHash != termsHash ||
            m.sourceChainId != (baseSide ? _terms.nativeChainId : _terms.baseChainId) ||
            m.sourceDomain != (baseSide ? _terms.nativeDomain : _terms.baseDomain) ||
            m.sourceEndpoint != (baseSide ? _terms.nativeEndpoint : _terms.baseEndpoint) ||
            m.destinationChainId != (baseSide ? _terms.baseChainId : _terms.nativeChainId) ||
            m.destinationDomain != (baseSide ? _terms.baseDomain : _terms.nativeDomain) ||
            m.destinationEndpoint != address(this) ||
            m.verifierDomain != (baseSide ? _terms.nativeToBaseVerifierDomain : _terms.baseToNativeVerifierDomain) ||
            m.nonce == 0 || m.transferId == bytes32(0) || m.sender == address(0) || m.recipient == address(0)) revert InvalidMessage();
        if (proof.length == 0 || proof.length > _terms.maxProofBytes) revert InvalidProof();
        bytes32 key = V2.sourceKey(m);
        if (consumed[key]) revert AlreadyConsumed();
        if (!verifier.verifyMessage(V2.digest(m), proof)) revert InvalidProof();
        consumed[key] = true;
    }
    function _send(V2.Kind kind, bytes32 id, address sender, address recipient, uint256 b, uint256 n)
        internal returns (V2.Message memory m)
    {
        uint256 nonce = nextNonce; nextNonce = nonce + 1;
        m.kind = kind; m.routeId = routeId; m.termsHash = termsHash;
        m.sourceChainId = baseSide ? _terms.baseChainId : _terms.nativeChainId;
        m.sourceDomain = baseSide ? _terms.baseDomain : _terms.nativeDomain;
        m.sourceEndpoint = address(this);
        m.destinationChainId = baseSide ? _terms.nativeChainId : _terms.baseChainId;
        m.destinationDomain = baseSide ? _terms.nativeDomain : _terms.baseDomain;
        m.destinationEndpoint = baseSide ? _terms.nativeEndpoint : _terms.baseEndpoint;
        m.nonce = nonce; m.transferId = id; m.sender = sender; m.recipient = recipient;
        m.baseAmount = b; m.nativeAmount = n;
        m.verifierDomain = baseSide ? _terms.baseToNativeVerifierDomain : _terms.nativeToBaseVerifierDomain;
        bytes32 h = V2.digest(m); outbox[nonce] = h;
        emit MessageSent(h, V2.sourceKey(m), m);
    }
}
