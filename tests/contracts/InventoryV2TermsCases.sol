// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import {InventoryBridgeV2 as V2} from "../../contracts/bridge-v2/API.sol";

/// @dev Pure arithmetic/wire assertions. No endpoint or proof qualification.
contract InventoryV2TermsCases {
    uint256 public constant CASE_COUNT = 8;
    event TermsCasePassed(uint256 indexed caseId);
    function _terms() private pure returns (V2.Terms memory t) {
        t.baseChainId = 8453; t.baseDomain = keccak256("base-fixture-only");
        t.baseEndpoint = address(1); t.baseToken = address(2);
        t.nativeChainId = 9000; t.nativeDomain = keccak256("native-fixture-only");
        t.nativeEndpoint = address(3); t.nativeAsset = address(0);
        t.totalBaseSupply = 1000 * 1e6; t.baseLot = 2 * 1e6; t.nativeLot = 7 * 1e18;
        t.baseToNativeVerifierDomain = keccak256("to-native-fixture-only");
        t.nativeToBaseVerifierDomain = keccak256("to-base-fixture-only"); t.maxProofBytes = 1024;
    }
    function validate(V2.Terms calldata t) external pure { V2.validate(t); }
    function quote(V2.Terms calldata t, uint256 b) external pure returns (uint256) { return V2.quote(t, b); }
    function _refuse(bytes memory payload, bytes4 selector) private view {
        (bool ok, bytes memory reason) = address(this).staticcall(payload);
        require(!ok && reason.length >= 4 && bytes4(reason) == selector, "exact pure refusal");
    }
    function runCase(uint256 id) external {
        require(id < CASE_COUNT); V2.Terms memory t = _terms();
        if (id == 0) {
            V2.validate(t);
            require(V2.quote(t, t.totalBaseSupply) == 3500 * 1e18);
            require(V2.quote(t, 6 * 1e6) == 21 * 1e18, "6/18 decimals with 2:7 lots");
        } else if (id == 1) {
            _refuse(abi.encodeCall(this.quote, (t, 1)), V2.InexactAmount.selector);
            _refuse(abi.encodeCall(this.quote, (t, 0)), V2.InexactAmount.selector);
            _refuse(abi.encodeCall(this.quote, (t, t.totalBaseSupply + t.baseLot)), V2.InexactAmount.selector);
        } else if (id == 2) {
            t.totalBaseSupply += 1;
            _refuse(abi.encodeCall(this.validate, (t)), V2.InvalidTerms.selector);
        } else if (id == 3) {
            t.totalBaseSupply = type(uint256).max; t.baseLot = 1; t.nativeLot = 2;
            _refuse(abi.encodeCall(this.validate, (t)), V2.InvalidTerms.selector);
            _refuse(abi.encodeCall(this.quote, (t, type(uint256).max)), V2.InexactAmount.selector);
        } else if (id == 4) {
            t.baseDomain = t.nativeDomain;
            _refuse(abi.encodeCall(this.validate, (t)), V2.InvalidTerms.selector);
            t = _terms(); t.baseToNativeVerifierDomain = t.nativeToBaseVerifierDomain;
            _refuse(abi.encodeCall(this.validate, (t)), V2.InvalidTerms.selector);
            t = _terms(); t.maxProofBytes = 1_048_577;
            _refuse(abi.encodeCall(this.validate, (t)), V2.InvalidTerms.selector);
        } else if (id == 5) {
            bytes32 h = V2.termsHash(t); bytes32 r = V2.routeId(t);
            t.nativeLot += 1;
            require(V2.termsHash(t) != h && V2.routeId(t) == r, "terms change keeps route identity but not terms");
            t.baseToken = address(9); require(V2.routeId(t) != r);
        } else if (id == 6) {
            V2.Message memory m; m.sourceChainId = 8453; m.sourceDomain = t.baseDomain;
            m.sourceEndpoint = t.baseEndpoint; m.nonce = 9; m.kind = V2.Kind.LOCK_BASE;
            bytes32 k = V2.sourceKey(m); bytes32 d = V2.digest(m);
            m.kind = V2.Kind.RESERVE_BASE; m.transferId = keccak256("other-transfer");
            m.termsHash = keccak256("other-terms"); m.nativeAmount = 100;
            require(V2.sourceKey(m) == k && V2.digest(m) != d, "exclusive source key, complete wire digest");
            m.nonce += 1; require(V2.sourceKey(m) != k);
        } else {
            t.nativeAsset = address(4);
            _refuse(abi.encodeCall(this.validate, (t)), V2.InvalidTerms.selector);
            t = _terms(); t.baseLot = 0;
            _refuse(abi.encodeCall(this.validate, (t)), V2.InvalidTerms.selector);
            t = _terms(); t.totalBaseSupply = type(uint256).max; t.baseLot = 1; t.nativeLot = 1;
            V2.validate(t); require(V2.quote(t, type(uint256).max) == type(uint256).max);
        }
        emit TermsCasePassed(id);
    }
}
