// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import {InventoryBridgeV2 as V2} from "../../contracts/bridge-v2/API.sol";
import {InventoryEndpointV2} from "../../contracts/bridge-v2/Endpoint.sol";
import {BaseInventoryV2} from "../../contracts/bridge-v2/BaseInventory.sol";
import {NativeReserveV2} from "../../contracts/bridge-v2/NativeReserve.sol";
import {InventoryV2Fixture, InventoryV2BaseClient} from "./InventoryV2Harness.sol";

/// @dev Separate one-shot suite keeps bytecode bounds strict. These are actual
/// nested EVM calls by two different Base recipients/Native funders, not a model.
contract InventoryV2MultiClientCases {
    uint256 public constant CASE_COUNT = 1;
    bool private started;
    InventoryV2Fixture private f;
    InventoryV2BaseClient private a;
    InventoryV2BaseClient private b;
    bytes32 private idA;
    bytes32 private idB;
    event MultiClientCasePassed(uint256 indexed caseId, address indexed fixture,
        address clientA, address clientB, bytes32 finalState);

    function _reservation(bytes32 id) private view returns (bytes32) {
        (address funder,address recipient,uint256 baseAmount,uint256 nativeAmount,BaseInventoryV2.ReservationState state) = f.base().reservations(id);
        return keccak256(abi.encode(funder,recipient,baseAmount,nativeAmount,state));
    }
    function _fund(bytes32 id) private view returns (bytes32) {
        (address funder,address recipient,uint256 baseAmount,uint256 nativeAmount,NativeReserveV2.FundState state) = f.nativeReserve().funds(id);
        return keccak256(abi.encode(funder,recipient,baseAmount,nativeAmount,state));
    }
    function _snapshot() private view returns (bytes32 result) {
        bytes32 wallets = keccak256(abi.encode(address(a).balance,address(b).balance,
            f.token().balanceOf(address(a)),f.token().balanceOf(address(b))));
        bytes32 records = keccak256(abi.encode(_reservation(idA),_reservation(idB),_fund(idA),_fund(idB)));
        result = keccak256(abi.encode(f.stateHash(),wallets,records));
        V2.Terms memory t = f.base().terms();
        uint256 baseNext = f.base().nextNonce(); uint256 nativeNext = f.nativeReserve().nextNonce();
        require(baseNext <= 10 && nativeNext <= 10, "bounded snapshot");
        for (uint256 i = 1; i <= baseNext; ++i) {
            bytes32 key = keccak256(abi.encode(t.baseChainId,t.baseDomain,t.baseEndpoint,i));
            result = keccak256(abi.encode(result,f.base().outbox(i),f.nativeReserve().consumed(key)));
        }
        for (uint256 i = 1; i <= nativeNext; ++i) {
            bytes32 key = keccak256(abi.encode(t.nativeChainId,t.nativeDomain,t.nativeEndpoint,i));
            result = keccak256(abi.encode(result,f.nativeReserve().outbox(i),f.base().consumed(key)));
        }
    }
    function stateHash() external view returns (bytes32) { require(started); return _snapshot(); }
    function _reject(bytes memory action) private {
        bytes32 beforeState = _snapshot(); (bool ok, bytes memory reason) = address(f).call(action);
        require(!ok && reason.length >= 4 && bytes4(reason) == InventoryEndpointV2.InvalidState.selector,
            "exact ownership/state refusal");
        require(_snapshot() == beforeState, "refusal changed clients/inventory/ids/nonce/replay bits");
    }
    function _owned(V2.Message memory m, InventoryV2BaseClient actor) private view {
        require(m.sender == address(actor) && m.recipient == address(actor) && m.baseAmount == 40 && m.nativeAmount == 60);
        require(_reservation(m.transferId) == keccak256(abi.encode(address(actor),address(actor),uint256(40),uint256(60),BaseInventoryV2.ReservationState.Reserved)));
    }
    function _acknowledgmentsAndReplays(V2.Message memory fundA,V2.Message memory fundB,
        V2.Message memory paidB,V2.Message memory cancelledA) private {
        V2.Message memory refundedA = f.refund(cancelledA);
        f.acknowledgeRefund(refundedA); f.acknowledgeBase(paidB);
        require(f.base().freeBase() == 60 && f.base().reservedBase() == 0 && f.base().accountedBase() == 60);
        require(f.nativeReserve().pendingFunds() == 0 && f.nativeReserve().accountedReserve() == 1410);
        require(address(f.nativeReserve()).balance == 1410 && address(f).balance == 8530);
        require(address(a).balance == 60 && address(b).balance == 0);
        require(f.token().balanceOf(address(a)) == 0 && f.token().balanceOf(address(b)) == 40);
        require(f.token().balanceOf(address(f.base())) == 60 && f.token().balanceOf(address(f)) == 900);
        require(f.base().nextNonce() == 7 && f.nativeReserve().nextNonce() == 6);
        require(f.base().consumed(V2.sourceKey(fundA)) && f.base().consumed(V2.sourceKey(fundB)));
        require(f.nativeReserve().consumed(V2.sourceKey(cancelledA)) && f.nativeReserve().consumed(V2.sourceKey(paidB)));
        _reject(abi.encodeCall(f.settle, (fundA)));
        _reject(abi.encodeCall(f.settle, (fundB)));
        _reject(abi.encodeCall(f.refund, (cancelledA)));
        _reject(abi.encodeCall(f.acknowledgeBase, (paidB)));
        _reject(abi.encodeCall(f.clientCancelFunded, (a,fundA)));
        require(_reservation(idA) == keccak256(abi.encode(address(a),address(a),uint256(40),uint256(60),BaseInventoryV2.ReservationState.Refunded)));
        require(_reservation(idB) == keccak256(abi.encode(address(b),address(b),uint256(40),uint256(60),BaseInventoryV2.ReservationState.Paid)));
        require(_fund(idA) == keccak256(abi.encode(address(a),address(a),uint256(40),uint256(60),NativeReserveV2.FundState.Refunded)));
        require(_fund(idB) == keccak256(abi.encode(address(b),address(b),uint256(40),uint256(60),NativeReserveV2.FundState.Paid)));
    }
    function runCase(uint256 caseId,InventoryV2Fixture fixture) external {
        require(caseId == 0 && !started, "one fresh execution only"); started = true; f = fixture;
        V2.Terms memory t = f.base().terms();
        require(t.totalBaseSupply == 1000 && t.baseLot == 2 && t.nativeLot == 3 && address(f).balance == 10000);
        require(!f.base().supplyDeclared() && !f.nativeReserve().backingInitialized());
        a = new InventoryV2BaseClient(address(f)); b = new InventoryV2BaseClient(address(f));
        require(address(a) != address(b) && a.testDriver() == address(f) && b.testDriver() == address(f));
        f.ready(); f.forward(100);
        V2.Message memory reserveA = f.clientReserve(a,40); idA = reserveA.transferId; _owned(reserveA,a);
        V2.Message memory reserveB = f.clientReserve(b,40); idB = reserveB.transferId; _owned(reserveB,b);
        require(idA != idB && reserveA.nonce == 3 && reserveB.nonce == 4);
        require(f.base().freeBase() == 20 && f.base().reservedBase() == 80);
        _reject(abi.encodeCall(f.clientReserve, (a,uint256(22))));
        _reject(abi.encodeCall(f.clientRequestCancel, (b,idA)));
        _reject(abi.encodeCall(f.clientFund, (a,reserveB)));
        V2.Message memory fundB = f.clientFund(b,reserveB);
        V2.Message memory fundA = f.clientFund(a,reserveA);
        require(fundB.nonce == 3 && fundA.nonce == 4 && f.nativeReserve().pendingFunds() == 120);
        require(f.nativeReserve().consumed(V2.sourceKey(reserveA)) && f.nativeReserve().consumed(V2.sourceKey(reserveB)));
        _reject(abi.encodeCall(f.clientCancelFunded, (b,fundA)));
        _reject(abi.encodeCall(f.clientFund, (a,reserveA)));
        V2.Message memory paidB = f.settle(fundB);
        require(f.token().balanceOf(address(b)) == 40 && f.token().balanceOf(address(a)) == 0);
        _reject(abi.encodeCall(f.clientRequestCancel, (b,idB)));
        _reject(abi.encodeCall(f.clientCancelFunded, (b,fundB)));
        V2.Message memory cancelledA = f.clientCancelFunded(a,fundA);
        _reject(abi.encodeCall(f.settle, (fundA)));
        _acknowledgmentsAndReplays(fundA,fundB,paidB,cancelledA);
        emit MultiClientCasePassed(0,address(f),address(a),address(b),_snapshot());
    }
}
