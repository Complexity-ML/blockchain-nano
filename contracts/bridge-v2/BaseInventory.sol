// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import {IERC20} from "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import {SafeERC20} from "@openzeppelin/contracts/token/ERC20/utils/SafeERC20.sol";
import {InventoryBridgeV2 as V2} from "./API.sol";
import {InventoryEndpointV2} from "./Endpoint.sol";

/// @notice Closed prototype for a predetermined preissued token, not a minter.
abstract contract BaseInventoryV2 is InventoryEndpointV2 {
    using SafeERC20 for IERC20;
    enum LockState { None, PendingNative, AckedNative }
    enum ReservationState { None, Reserved, CancelRequested, Paid, CancelledUnfunded, CancelledFunded, Refunded }
    struct Lock { address sender; address recipient; uint256 b; uint256 n; LockState state; }
    struct Reservation { address funder; address recipient; uint256 b; uint256 n; ReservationState state; }
    IERC20 public immutable token;
    bool public supplyDeclared;
    bool public backingReady;
    bool public supplyFrozen;
    bytes32 public bootstrapId;
    address public bootstrapSender;
    uint256 public accountedBase;
    uint256 public freeBase;
    uint256 public pendingNativeBase;
    uint256 public reservedBase;
    mapping(bytes32 => Lock) public locks;
    mapping(bytes32 => Reservation) public reservations;
    event BaseAccounting(uint256 accounted, uint256 free, uint256 pendingNative, uint256 reserved);

    constructor(V2.Terms memory t, address v) InventoryEndpointV2(t, v, true) {
        if (t.baseToken.code.length == 0) revert V2.InvalidTerms();
        token = IERC20(t.baseToken);
    }
    function _safe() private view { _gate(); if (!backingReady) revert InvalidState(); }
    function _supply() private view {
        if (supplyFrozen || address(token).code.length == 0 || token.totalSupply() != _terms.totalBaseSupply) revert InvalidState();
    }
    function _accounting() private {
        if (accountedBase != freeBase + pendingNativeBase + reservedBase ||
            accountedBase > _terms.totalBaseSupply || token.balanceOf(address(this)) < accountedBase) revert AccountingMismatch();
        emit BaseAccounting(accountedBase, freeBase, pendingNativeBase, reservedBase);
    }
    function declareSupply() external nonReentrant returns (V2.Message memory) {
        _gate(); _supply(); if (supplyDeclared) revert InvalidState();
        // No preloaded balance becomes available inventory or another claim.
        supplyDeclared = true; bootstrapSender = msg.sender;
        bootstrapId = _newTransferId(V2.Kind.BOOTSTRAP);
        return _send(V2.Kind.BOOTSTRAP, bootstrapId, msg.sender, _terms.nativeEndpoint,
            _terms.totalBaseSupply, V2.quote(_terms, _terms.totalBaseSupply));
    }
    function confirmBacking(V2.Message calldata m, bytes calldata proof) external nonReentrant {
        _gate(); _supply(); if (!supplyDeclared || backingReady || m.transferId != bootstrapId) revert InvalidState();
        _same(m, bootstrapSender, _terms.nativeEndpoint, _terms.totalBaseSupply, V2.quote(_terms, _terms.totalBaseSupply));
        _consume(m, proof, V2.Kind.ACK_BOOTSTRAP); backingReady = true; _accounting();
    }
    function lockBase(uint256 b, address nativeRecipient) external nonReentrant returns (V2.Message memory) {
        _safe(); _supply(); _accounting();
        if (nativeRecipient == address(0) || nativeRecipient == _terms.nativeEndpoint) revert InvalidRecipient();
        uint256 n = V2.quote(_terms, b);
        uint256 beforeMe = token.balanceOf(address(this));
        uint256 beforeSender = token.balanceOf(msg.sender);
        token.safeTransferFrom(msg.sender, address(this), b);
        if (beforeSender < b || token.balanceOf(msg.sender) != beforeSender - b || token.balanceOf(address(this)) != beforeMe + b) revert AccountingMismatch();
        _supply();
        bytes32 id = _newTransferId(V2.Kind.LOCK_BASE);
        locks[id] = Lock(msg.sender, nativeRecipient, b, n, LockState.PendingNative);
        accountedBase += b; pendingNativeBase += b; _accounting();
        return _send(V2.Kind.LOCK_BASE, id, msg.sender, nativeRecipient, b, n);
    }
    function acknowledgeNativePaid(V2.Message calldata m, bytes calldata proof) external nonReentrant {
        _safe(); Lock storage l = locks[m.transferId]; if (l.state != LockState.PendingNative) revert InvalidState();
        _same(m, l.sender, l.recipient, l.b, l.n); _consume(m, proof, V2.Kind.ACK_NATIVE_PAID);
        l.state = LockState.AckedNative; pendingNativeBase -= l.b; freeBase += l.b; _accounting();
    }
    function reserveBase(uint256 b, address nativeFunder) external nonReentrant returns (V2.Message memory) {
        _safe(); _supply(); _accounting();
        if (nativeFunder == address(0) || nativeFunder == _terms.nativeEndpoint || msg.sender == address(this)) revert InvalidRecipient();
        uint256 n = V2.quote(_terms, b); if (b > freeBase) revert InvalidState();
        bytes32 id = _newTransferId(V2.Kind.RESERVE_BASE);
        // The recipient requests its own reservation; never take native first.
        reservations[id] = Reservation(nativeFunder, msg.sender, b, n, ReservationState.Reserved);
        freeBase -= b; reservedBase += b; _accounting();
        return _send(V2.Kind.RESERVE_BASE, id, nativeFunder, msg.sender, b, n);
    }
    function requestCancel(bytes32 id) external nonReentrant returns (V2.Message memory) {
        _safe(); Reservation storage r = reservations[id];
        if (r.state != ReservationState.Reserved || msg.sender != r.recipient) revert InvalidState();
        r.state = ReservationState.CancelRequested;
        // Crucially neither freeBase nor reservedBase changes here.
        return _send(V2.Kind.REQUEST_CANCEL, id, r.funder, r.recipient, r.b, r.n);
    }
    function acknowledgeUnfundedCancel(V2.Message calldata m, bytes calldata proof) external nonReentrant {
        _safe(); Reservation storage r = reservations[m.transferId];
        if (r.state != ReservationState.CancelRequested) revert InvalidState();
        _same(m, r.funder, r.recipient, r.b, r.n); _consume(m, proof, V2.Kind.ACK_CANCEL_UNFUNDED);
        r.state = ReservationState.CancelledUnfunded; reservedBase -= r.b; freeBase += r.b; _accounting();
    }
    function settleBase(V2.Message calldata m, bytes calldata proof) external nonReentrant returns (V2.Message memory) {
        _safe(); _supply(); _accounting(); Reservation storage r = reservations[m.transferId];
        if (r.state != ReservationState.Reserved) revert InvalidState();
        _same(m, r.funder, r.recipient, r.b, r.n); _consume(m, proof, V2.Kind.FUND_NATIVE);
        uint256 beforeMe = token.balanceOf(address(this));
        uint256 beforeRecipient = token.balanceOf(r.recipient);
        r.state = ReservationState.Paid; reservedBase -= r.b; accountedBase -= r.b;
        token.safeTransfer(r.recipient, r.b);
        if (token.balanceOf(address(this)) != beforeMe - r.b || token.balanceOf(r.recipient) != beforeRecipient + r.b) revert AccountingMismatch();
        _supply();
        _accounting(); return _send(V2.Kind.ACK_BASE_PAID, m.transferId, r.funder, r.recipient, r.b, r.n);
    }
    function cancelFunded(V2.Message calldata m, bytes calldata proof) external nonReentrant returns (V2.Message memory) {
        _safe(); Reservation storage r = reservations[m.transferId];
        if ((r.state != ReservationState.Reserved && r.state != ReservationState.CancelRequested) || msg.sender != r.recipient) revert InvalidState();
        _same(m, r.funder, r.recipient, r.b, r.n); _consume(m, proof, V2.Kind.FUND_NATIVE);
        r.state = ReservationState.CancelledFunded; reservedBase -= r.b; freeBase += r.b; _accounting();
        return _send(V2.Kind.CANCEL_FUNDED, m.transferId, r.funder, r.recipient, r.b, r.n);
    }
    function acknowledgeNativeRefund(V2.Message calldata m, bytes calldata proof) external nonReentrant {
        _safe(); Reservation storage r = reservations[m.transferId];
        if (r.state != ReservationState.CancelledFunded) revert InvalidState();
        _same(m, r.funder, r.recipient, r.b, r.n); _consume(m, proof, V2.Kind.ACK_NATIVE_REFUNDED);
        r.state = ReservationState.Refunded;
    }
    function signalSupplyMismatch() external nonReentrant returns (V2.Message memory) {
        _gate(); uint256 observed = token.totalSupply();
        if (supplyFrozen || observed == _terms.totalBaseSupply) revert InvalidState();
        supplyFrozen = true;
        return _send(V2.Kind.FREEZE_SUPPLY, _newTransferId(V2.Kind.FREEZE_SUPPLY), msg.sender,
            _terms.nativeEndpoint, observed, 0);
    }
}
