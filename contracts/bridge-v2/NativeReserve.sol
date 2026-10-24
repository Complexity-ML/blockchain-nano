// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import {InventoryBridgeV2 as V2} from "./API.sol";
import {InventoryEndpointV2} from "./Endpoint.sol";

/// @notice No minter, owner, upgrade/reset, rescue or collateral withdrawal.
abstract contract NativeReserveV2 is InventoryEndpointV2 {
    enum FundState { None, Funded, Paid, CancelledUnfunded, Refunded }
    struct Fund { address funder; address recipient; uint256 b; uint256 n; FundState state; }
    bool public backingInitialized;
    bool public supplyFrozen;
    uint256 public accountedReserve;
    uint256 public pendingFunds;
    mapping(bytes32 => bool) public paidLocks;
    mapping(bytes32 => Fund) public funds;
    event NativeAccounting(uint256 reserve, uint256 pendingRefundable, uint256 actualBalance);
    error NativeTransferFailed();

    constructor(V2.Terms memory t, address v) InventoryEndpointV2(t, v, false) {}
    function _safe() private view { _gate(); if (!backingInitialized) revert InvalidState(); }
    function _risk() private view { _safe(); if (supplyFrozen) revert InvalidState(); }
    function _accounting() private {
        if (address(this).balance < accountedReserve + pendingFunds) revert AccountingMismatch();
        emit NativeAccounting(accountedReserve, pendingFunds, address(this).balance);
    }
    function _pay(address recipient, uint256 n) private {
        (bool ok,) = payable(recipient).call{value:n}(""); if (!ok) revert NativeTransferFailed();
    }
    function bootstrap(V2.Message calldata m, bytes calldata proof) external payable nonReentrant returns (V2.Message memory) {
        _gate(); if (backingInitialized || supplyFrozen) revert InvalidState();
        uint256 n = V2.quote(_terms, _terms.totalBaseSupply);
        if (m.baseAmount != _terms.totalBaseSupply || m.nativeAmount != n ||
            m.recipient != address(this) || msg.value != n) revert AccountingMismatch();
        _consume(m, proof, V2.Kind.BOOTSTRAP);
        backingInitialized = true; accountedReserve = n; _accounting();
        return _send(V2.Kind.ACK_BOOTSTRAP, m.transferId, m.sender, m.recipient, m.baseAmount, n);
    }
    function payNative(V2.Message calldata m, bytes calldata proof) external nonReentrant returns (V2.Message memory) {
        _risk(); _accounting(); _pair(m);
        if (paidLocks[m.transferId] || m.recipient == address(this) || m.nativeAmount > accountedReserve) revert InvalidState();
        _consume(m, proof, V2.Kind.LOCK_BASE);
        paidLocks[m.transferId] = true; accountedReserve -= m.nativeAmount;
        _pay(m.recipient, m.nativeAmount); _accounting();
        return _send(V2.Kind.ACK_NATIVE_PAID, m.transferId, m.sender, m.recipient, m.baseAmount, m.nativeAmount);
    }
    function fundNative(V2.Message calldata m, bytes calldata proof) external payable nonReentrant returns (V2.Message memory) {
        _risk(); _accounting(); _pair(m);
        if (funds[m.transferId].state != FundState.None || m.sender != msg.sender ||
            m.sender == address(this) || m.recipient == _terms.baseEndpoint || msg.value != m.nativeAmount) revert InvalidState();
        _consume(m, proof, V2.Kind.RESERVE_BASE);
        funds[m.transferId] = Fund(m.sender, m.recipient, m.baseAmount, m.nativeAmount, FundState.Funded);
        // Refundable funding is never spendable forward-payment collateral.
        pendingFunds += m.nativeAmount; _accounting();
        return _send(V2.Kind.FUND_NATIVE, m.transferId, m.sender, m.recipient, m.baseAmount, m.nativeAmount);
    }
    function acknowledgeBasePaid(V2.Message calldata m, bytes calldata proof) external nonReentrant {
        _safe(); Fund storage f = funds[m.transferId]; if (f.state != FundState.Funded) revert InvalidState();
        _same(m, f.funder, f.recipient, f.b, f.n); _consume(m, proof, V2.Kind.ACK_BASE_PAID);
        f.state = FundState.Paid; pendingFunds -= f.n; accountedReserve += f.n; _accounting();
    }
    function cancelUnfunded(V2.Message calldata m, bytes calldata proof) external nonReentrant returns (V2.Message memory) {
        _safe(); _pair(m); if (funds[m.transferId].state != FundState.None) revert InvalidState();
        _consume(m, proof, V2.Kind.REQUEST_CANCEL);
        funds[m.transferId] = Fund(m.sender, m.recipient, m.baseAmount, m.nativeAmount, FundState.CancelledUnfunded);
        _accounting(); return _send(V2.Kind.ACK_CANCEL_UNFUNDED, m.transferId, m.sender, m.recipient, m.baseAmount, m.nativeAmount);
    }
    function refundNative(V2.Message calldata m, bytes calldata proof) external nonReentrant returns (V2.Message memory) {
        _safe(); Fund storage f = funds[m.transferId]; if (f.state != FundState.Funded) revert InvalidState();
        _same(m, f.funder, f.recipient, f.b, f.n); _consume(m, proof, V2.Kind.CANCEL_FUNDED);
        f.state = FundState.Refunded; pendingFunds -= f.n;
        _pay(f.funder, f.n); _accounting();
        return _send(V2.Kind.ACK_NATIVE_REFUNDED, m.transferId, f.funder, f.recipient, f.b, f.n);
    }
    function acceptSupplyFreeze(V2.Message calldata m, bytes calldata proof) external nonReentrant {
        _gate(); if (supplyFrozen || m.baseAmount == _terms.totalBaseSupply ||
            m.nativeAmount != 0 || m.recipient != address(this)) revert InvalidState();
        _consume(m, proof, V2.Kind.FREEZE_SUPPLY); supplyFrozen = true;
    }
    // No receive/fallback: donations can still arrive forcibly but are never
    // credited to accountedReserve or pendingFunds and confer no entitlement.
}
