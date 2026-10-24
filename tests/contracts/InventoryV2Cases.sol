// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import {InventoryBridgeV2 as V2} from "../../contracts/bridge-v2/API.sol";
import {InventoryEndpointV2} from "../../contracts/bridge-v2/Endpoint.sol";
import {BaseInventoryV2} from "../../contracts/bridge-v2/BaseInventory.sol";
import {NativeReserveV2} from "../../contracts/bridge-v2/NativeReserve.sol";
import {InventoryV2Fixture, InventoryV2NativeActor, InventoryV2TokenMock} from "./InventoryV2Harness.sol";

/// @dev Assertion-only suite; an external SYNTHETIC backend supplies one fresh,
/// funded fixture per case. Never deploy/fund this on a public chain.
abstract contract InventoryV2CaseLogic {
    event CasePassed(uint256 indexed caseId, address indexed fixture, bytes32 finalState);
    function _reject(InventoryV2Fixture f, bytes memory action, bytes4 selector) private {
        bytes32 beforeState = f.stateHash();
        (bool ok, bytes memory reason) = address(f).call(action);
        require(!ok && reason.length >= 4 && bytes4(reason) == selector, "expected exact refusal");
        require(f.stateHash() == beforeState, "refusal changed accounting/funds/nonce");
    }
    function _ready(InventoryV2Fixture f) private { f.ready(); require(f.base().backingReady()); }
    function _clone(V2.Message memory m) private pure returns (V2.Message memory) {
        // Solidity memory-to-memory struct assignment aliases the original.
        return abi.decode(abi.encode(m), (V2.Message));
    }
    function _forward(InventoryV2Fixture f) private { f.forward(100); require(f.base().freeBase() == 100); }
    function _start(InventoryV2Fixture f) internal view {
        require(address(f).balance == 10000 && f.fullReserve() == 1500,
            "one fresh synthetic 1000/2/3 fixture funded with exactly 10000 wei");
        require(!f.base().supplyDeclared() && !f.nativeReserve().backingInitialized(), "fresh fixture only");
        V2.Terms memory t = f.base().terms();
        require(t.totalBaseSupply == 1000 && t.baseLot == 2 && t.nativeLot == 3);
    }
    function _closed(InventoryV2Fixture f) internal {
        _reject(f, abi.encodeCall(f.declare, ()), InventoryEndpointV2.RouteDisabled.selector);
        require(f.base().nextNonce() == 1 && f.nativeReserve().nextNonce() == 1);
    }
    function _bootstrap(InventoryV2Fixture f) internal {
        f.enable(); V2.Message memory m = f.declare();
        _reject(f, abi.encodeCall(f.finishBootstrap, (m, 1499)), InventoryEndpointV2.AccountingMismatch.selector);
        require(!f.nativeReserve().backingInitialized() && !f.nativeReserve().consumed(V2.sourceKey(m)));
        f.finishBootstrap(m, 1500);
        require(f.nativeReserve().accountedReserve() == 1500 && f.base().freeBase() == 0);
        _reject(f, abi.encodeCall(f.repeatBootstrap, (m)), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.declare, ()), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.reserve, (2, address(f))), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.lock, (3, address(f))), V2.InexactAmount.selector);
    }
    function _roundTrip(InventoryV2Fixture f) internal {
        _ready(f); V2.Message memory lock = f.lock(100, address(f));
        require(f.base().pendingNativeBase() == 100 && f.base().freeBase() == 0);
        V2.Message memory paid = f.payLock(lock);
        require(f.base().freeBase() == 0 && f.nativeReserve().accountedReserve() == 1350);
        f.acknowledgeLock(paid);
        V2.Message memory reservation = f.reserve(40, address(f));
        require(f.base().freeBase() == 60 && f.base().reservedBase() == 40);
        V2.Message memory funded = f.fund(reservation);
        require(f.nativeReserve().pendingFunds() == 60 && f.nativeReserve().accountedReserve() == 1350);
        V2.Message memory basePaid = f.settle(funded);
        require(f.nativeReserve().pendingFunds() == 60 && f.base().accountedBase() == 60);
        f.acknowledgeBase(basePaid);
        require(f.nativeReserve().pendingFunds() == 0 && f.nativeReserve().accountedReserve() == 1410);
        _reject(f, abi.encodeCall(f.settle, (funded)), InventoryEndpointV2.InvalidState.selector);
    }
    function _cancelBeforeFunding(InventoryV2Fixture f) internal {
        _ready(f); _forward(f); V2.Message memory r = f.reserve(40, address(f));
        V2.Message memory request = f.requestCancel(r.transferId);
        require(f.base().freeBase() == 60 && f.base().reservedBase() == 40);
        V2.Message memory ack = f.cancelUnfunded(request);
        require(f.base().freeBase() == 60, "request or remote cancel alone cannot release inventory");
        f.acknowledgeCancel(ack); require(f.base().freeBase() == 100 && f.base().reservedBase() == 0);
        _reject(f, abi.encodeCall(f.fund, (r)), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.acknowledgeCancel, (ack)), InventoryEndpointV2.InvalidState.selector);
    }
    function _fundCancelRace(InventoryV2Fixture f) internal {
        _ready(f); _forward(f); V2.Message memory r = f.reserve(40, address(f));
        V2.Message memory request = f.requestCancel(r.transferId);
        V2.Message memory funded = f.fund(r); // Native has not yet seen request.
        _reject(f, abi.encodeCall(f.cancelUnfunded, (request)), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.settle, (funded)), InventoryEndpointV2.InvalidState.selector);
        V2.Message memory cancelled = f.cancelFunded(funded);
        require(f.base().freeBase() == 100 && f.nativeReserve().pendingFunds() == 60);
        V2.Message memory refunded = f.refund(cancelled); f.acknowledgeRefund(refunded);
        require(f.nativeReserve().pendingFunds() == 0 && f.nativeReserve().accountedReserve() == 1350);
        _reject(f, abi.encodeCall(f.refund, (cancelled)), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.settle, (funded)), InventoryEndpointV2.InvalidState.selector);
    }
    function _payoutWins(InventoryV2Fixture f) internal {
        _ready(f); _forward(f); V2.Message memory r = f.reserve(40, address(f));
        V2.Message memory funded = f.fund(r); V2.Message memory paid = f.settle(funded);
        _reject(f, abi.encodeCall(f.cancelFunded, (funded)), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.requestCancel, (r.transferId)), InventoryEndpointV2.InvalidState.selector);
        f.acknowledgeBase(paid);
        V2.Message memory falseCancel = paid; falseCancel.kind = V2.Kind.CANCEL_FUNDED;
        _reject(f, abi.encodeCall(f.refund, (falseCancel)), InventoryEndpointV2.InvalidState.selector);
    }
    function _nativeRevertRetry(InventoryV2Fixture f) internal {
        _ready(f); InventoryV2NativeActor receiver = new InventoryV2NativeActor();
        receiver.configure(true, address(0), "", false);
        V2.Message memory m = f.lock(100, address(receiver));
        _reject(f, abi.encodeCall(f.payLock, (m)), NativeReserveV2.NativeTransferFailed.selector);
        require(!f.nativeReserve().consumed(V2.sourceKey(m)) && !f.nativeReserve().paidLocks(m.transferId));
        receiver.configure(false, address(0), "", false); V2.Message memory paid = f.payLock(m);
        f.acknowledgeLock(paid); require(address(receiver).balance == 150);
        _reject(f, abi.encodeCall(f.payLock, (m)), InventoryEndpointV2.InvalidState.selector);
    }
    function _reentrance(InventoryV2Fixture f) internal {
        _ready(f); InventoryV2NativeActor receiver = new InventoryV2NativeActor();
        V2.Message memory m = f.lock(100, address(receiver));
        bytes memory action = abi.encodeCall(NativeReserveV2.payNative, (m, hex"01"));
        receiver.configure(false, address(f.nativeReserve()), action, true);
        _reject(f, abi.encodeCall(f.payLock, (m)), NativeReserveV2.NativeTransferFailed.selector);
        receiver.configure(false, address(f.nativeReserve()), action, false);
        V2.Message memory paid = f.payLock(m); f.acknowledgeLock(paid);
        require(receiver.nestedBlocked() && address(receiver).balance == 150 && f.nativeReserve().accountedReserve() == 1350);
    }
    function _refundRevertRetry(InventoryV2Fixture f) internal {
        _ready(f); _forward(f); InventoryV2NativeActor funder = new InventoryV2NativeActor();
        V2.Message memory r = f.reserve(40, address(funder));
        V2.Message memory funded = f.fundActor(funder, r); V2.Message memory cancelled = f.cancelFunded(funded);
        funder.configure(true, address(0), "", false);
        _reject(f, abi.encodeCall(f.refund, (cancelled)), NativeReserveV2.NativeTransferFailed.selector);
        require(!f.nativeReserve().consumed(V2.sourceKey(cancelled)) && f.nativeReserve().pendingFunds() == 60);
        funder.configure(false, address(0), "", false); V2.Message memory refunded = f.refund(cancelled);
        f.acknowledgeRefund(refunded); require(address(funder).balance == 60 && f.nativeReserve().pendingFunds() == 0);
    }
    function _donations(InventoryV2Fixture f) internal {
        _ready(f); _forward(f); f.donate(20, 7);
        require(f.token().balanceOf(address(f.base())) == 120 && f.base().freeBase() == 100);
        require(address(f.nativeReserve()).balance == 1357 && f.nativeReserve().accountedReserve() == 1350);
        _reject(f, abi.encodeCall(f.reserve, (102, address(f))), InventoryEndpointV2.InvalidState.selector);
    }
    function _supplyFreeze(InventoryV2Fixture f) internal {
        _ready(f); _forward(f); V2.Message memory r = f.reserve(40, address(f)); V2.Message memory funded = f.fund(r);
        V2.Message memory pendingLock = f.lock(2, address(f));
        f.corruptSupply(2, false);
        _reject(f, abi.encodeCall(f.lock, (2, address(f))), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.settle, (funded)), InventoryEndpointV2.InvalidState.selector);
        f.freeze(); require(f.base().supplyFrozen() && f.nativeReserve().supplyFrozen());
        _reject(f, abi.encodeCall(f.payLock, (pendingLock)), InventoryEndpointV2.InvalidState.selector);
        V2.Message memory cancelled = f.cancelFunded(funded); V2.Message memory refunded = f.refund(cancelled);
        f.acknowledgeRefund(refunded); require(f.nativeReserve().pendingFunds() == 0);
    }
    function _exclusiveNonceAndDomain(InventoryV2Fixture f) internal {
        _ready(f); V2.Message memory m = f.lock(100, address(f));
        V2.Message memory wrong = _clone(m); wrong.termsHash = keccak256("other terms");
        _reject(f, abi.encodeCall(f.payLock, (wrong)), InventoryEndpointV2.InvalidMessage.selector);
        wrong = _clone(m); wrong.routeId = keccak256("other route");
        _reject(f, abi.encodeCall(f.payLock, (wrong)), InventoryEndpointV2.InvalidMessage.selector);
        wrong = _clone(m); wrong.destinationChainId += 1;
        _reject(f, abi.encodeCall(f.payLock, (wrong)), InventoryEndpointV2.InvalidMessage.selector);
        wrong = _clone(m); wrong.nativeAmount += 1;
        _reject(f, abi.encodeCall(f.payLock, (wrong)), InventoryEndpointV2.InvalidMessage.selector);
        f.approveOnly(keccak256(abi.encode(keccak256("NanoBridgeMessageV1"), m)));
        _reject(f, abi.encodeCall(f.noApprovalPay, (m, hex"01")), InventoryEndpointV2.InvalidProof.selector);
        V2.Message memory paid = f.payLock(m); f.acknowledgeLock(paid);
        wrong = _clone(m); wrong.kind = V2.Kind.RESERVE_BASE; wrong.transferId = keccak256("conflicting reservation");
        _reject(f, abi.encodeCall(f.fund, (wrong)), InventoryEndpointV2.AlreadyConsumed.selector);
    }
    function _refundableNotCollateral(InventoryV2Fixture f) internal {
        _ready(f); _forward(f); V2.Message memory r = f.reserve(40, address(f)); f.fund(r);
        require(address(f.nativeReserve()).balance == 1410 && f.nativeReserve().accountedReserve() == 1350);
        V2.Message memory forged = r; forged.kind = V2.Kind.LOCK_BASE;
        forged.transferId = keccak256("dishonest proof cannot spend pending funding");
        forged.nonce = 1000; forged.baseAmount = 920; forged.nativeAmount = 1380;
        _reject(f, abi.encodeCall(f.payLock, (forged)), InventoryEndpointV2.InvalidState.selector);
    }
    function _tokenDeltasAndPostSupply(InventoryV2Fixture f) internal {
        _ready(f);
        f.tokenMode(InventoryV2TokenMock.Mode.NoOp);
        _reject(f, abi.encodeCall(f.lock, (100, address(f))), InventoryEndpointV2.AccountingMismatch.selector);
        f.tokenMode(InventoryV2TokenMock.Mode.Taxed);
        _reject(f, abi.encodeCall(f.lock, (100, address(f))), InventoryEndpointV2.AccountingMismatch.selector);
        f.tokenMode(InventoryV2TokenMock.Mode.ExtraMint);
        _reject(f, abi.encodeCall(f.lock, (100, address(f))), InventoryEndpointV2.InvalidState.selector);
        f.tokenMode(InventoryV2TokenMock.Mode.Normal); f.corruptSupply(1, true);
        _reject(f, abi.encodeCall(f.lock, (100, address(f))), InventoryEndpointV2.InvalidState.selector);
    }
    function _concurrentReservations(InventoryV2Fixture f) internal {
        _ready(f); _forward(f);
        InventoryV2NativeActor funderA = new InventoryV2NativeActor();
        InventoryV2NativeActor funderB = new InventoryV2NativeActor();
        V2.Message memory a = f.reserve(60, address(funderA));
        V2.Message memory b = f.reserve(40, address(funderB));
        _reject(f, abi.encodeCall(f.reserve, (2, address(f))), InventoryEndpointV2.InvalidState.selector);
        V2.Message memory fundedB = f.fundActor(funderB, b);
        V2.Message memory fundedA = f.fundActor(funderA, a);
        V2.Message memory paidB = f.settle(fundedB);
        V2.Message memory cancelledA = f.cancelFunded(fundedA);
        V2.Message memory refundA = f.refund(cancelledA);
        f.acknowledgeRefund(refundA); f.acknowledgeBase(paidB);
        require(f.base().freeBase() == 60 && f.base().reservedBase() == 0 && f.base().accountedBase() == 60);
        require(f.nativeReserve().pendingFunds() == 0 && f.nativeReserve().accountedReserve() == 1410);
        require(address(funderA).balance == 90 && address(funderB).balance == 0, "refund only original distinct funder");
        _reject(f, abi.encodeCall(f.settle, (fundedA)), InventoryEndpointV2.InvalidState.selector);
        _reject(f, abi.encodeCall(f.refund, (cancelledA)), InventoryEndpointV2.InvalidState.selector);
    }
}

/// @dev Split dispatchers preserve all original IDs and assertion bodies. Only
/// reachable internal cases enter each runtime; neither suite gets a size waiver.
contract InventoryV2CasesA is InventoryV2CaseLogic {
    uint256 public constant FIRST_CASE_ID = 0;
    uint256 public constant CASE_COUNT = 8;
    function runCase(uint256 id, InventoryV2Fixture f) external {
        require(id < 8, "A owns original case IDs 0..7"); _start(f);
        if (id == 0) _closed(f);
        else if (id == 1) _bootstrap(f);
        else if (id == 2) _roundTrip(f);
        else if (id == 3) _cancelBeforeFunding(f);
        else if (id == 4) _fundCancelRace(f);
        else if (id == 5) _payoutWins(f);
        else if (id == 6) _nativeRevertRetry(f);
        else _reentrance(f);
        emit CasePassed(id, address(f), f.stateHash());
    }
}
contract InventoryV2CasesB is InventoryV2CaseLogic {
    uint256 public constant FIRST_CASE_ID = 8;
    uint256 public constant CASE_COUNT = 7;
    function runCase(uint256 id, InventoryV2Fixture f) external {
        require(id >= 8 && id < 15, "B owns original case IDs 8..14"); _start(f);
        if (id == 8) _refundRevertRetry(f);
        else if (id == 9) _donations(f);
        else if (id == 10) _supplyFreeze(f);
        else if (id == 11) _exclusiveNonceAndDomain(f);
        else if (id == 12) _refundableNotCollateral(f);
        else if (id == 13) _tokenDeltasAndPostSupply(f);
        else _concurrentReservations(f);
        emit CasePassed(id, address(f), f.stateHash());
    }
}
