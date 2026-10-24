// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
// TEST NAMESPACE ONLY: arbitrary digest approvals and enable switches are not
// authenticators or governance for a real route. No signatures are produced.
import {ERC20} from "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import {InventoryBridgeV2 as V2, IInventoryBridgeV2Verifier} from "../../contracts/bridge-v2/API.sol";
import {BaseInventoryV2} from "../../contracts/bridge-v2/BaseInventory.sol";
import {NativeReserveV2} from "../../contracts/bridge-v2/NativeReserve.sol";

contract InventoryV2DigestMock is IInventoryBridgeV2Verifier {
    address public immutable testOwner;
    mapping(bytes32 => bool) public approved;
    constructor() { testOwner = msg.sender; }
    function approve(bytes32 digest, bool value) external { require(msg.sender == testOwner); approved[digest] = value; }
    function verifyMessage(bytes32 digest, bytes calldata proof) external view returns (bool) {
        return proof.length == 1 && proof[0] == 0x01 && approved[digest];
    }
}
contract InventoryV2TokenMock is ERC20 {
    enum Mode { Normal, NoOp, Taxed, ExtraMint }
    address public immutable testOwner;
    Mode public mode;
    constructor(uint256 supply) ERC20("SYNTHETIC PREISSUED ONLY", "TEST") { testOwner = msg.sender; _mint(msg.sender, supply); }
    function setMode(Mode value) external { require(msg.sender == testOwner); mode = value; }
    function corruptMint(uint256 n) external { require(msg.sender == testOwner); _mint(testOwner, n); }
    function corruptBurn(uint256 n) external { require(msg.sender == testOwner); _burn(testOwner, n); }
    function transfer(address to, uint256 n) public override returns (bool) {
        if (mode == Mode.NoOp) return true;
        bool ok = super.transfer(to, n);
        if (mode == Mode.Taxed && n > 0) _burn(to, 1);
        if (mode == Mode.ExtraMint) _mint(address(0x1234), 1);
        return ok;
    }
    function transferFrom(address from, address to, uint256 n) public override returns (bool) {
        if (mode == Mode.NoOp) return true;
        bool ok = super.transferFrom(from, to, n);
        if (mode == Mode.Taxed && n > 0) _burn(to, 1);
        if (mode == Mode.ExtraMint) _mint(address(0x1234), 1);
        return ok;
    }
}
contract InventoryV2OpenBase is BaseInventoryV2 {
    bool public enabled;
    address public immutable testOwner;
    constructor(V2.Terms memory t, address v) BaseInventoryV2(t, v) { testOwner = msg.sender; }
    function setEnabled(bool value) external { require(msg.sender == testOwner); enabled = value; }
    function _requireRouteEnabled() internal view override { if (!enabled) revert RouteDisabled(); }
}
contract InventoryV2OpenNative is NativeReserveV2 {
    bool public enabled;
    address public immutable testOwner;
    constructor(V2.Terms memory t, address v) NativeReserveV2(t, v) { testOwner = msg.sender; }
    function setEnabled(bool value) external { require(msg.sender == testOwner); enabled = value; }
    function _requireRouteEnabled() internal view override { if (!enabled) revert RouteDisabled(); }
}
contract InventoryV2NativeActor {
    bool public reject;
    bool public reenter;
    bool public revertAfterReentry;
    bool public nestedBlocked;
    bytes4 public nestedSelector;
    address public target;
    bytes public payload;
    function configure(bool reject_, address target_, bytes calldata payload_, bool revertAfter_) external {
        reject = reject_; target = target_; payload = payload_; reenter = target_ != address(0);
        revertAfterReentry = revertAfter_;
    }
    function fund(NativeReserveV2 endpoint, V2.Message calldata m) external payable returns (V2.Message memory) {
        return endpoint.fundNative{value:msg.value}(m, hex"01");
    }
    receive() external payable {
        require(!reject, "synthetic recipient rejects native");
        if (reenter) {
            (bool ok, bytes memory reason) = target.call(payload);
            require(!ok && reason.length >= 4, "nested call unexpectedly succeeded or lacked refusal");
            nestedSelector = bytes4(reason);
            require(nestedSelector == bytes4(keccak256("ReentrancyGuardReentrantCall()")), "wrong nested refusal");
            nestedBlocked = true;
        }
        require(!revertAfterReentry, "synthetic outer callback rejection");
    }
}
contract InventoryV2ForcedDonation {
    constructor(address payable recipient) payable { selfdestruct(recipient); }
}

/// @dev Test-only real Base caller and Native funder. Fixture is the driver,
/// but each endpoint observes this actor's address as msg.sender, not Fixture.
contract InventoryV2BaseClient {
    address public immutable testDriver;
    constructor(address driver) { require(driver.code.length != 0); testDriver = driver; }
    modifier onlyDriver() { require(msg.sender == testDriver, "test driver only"); _; }
    function reserve(BaseInventoryV2 endpoint, uint256 b) external onlyDriver returns (V2.Message memory) {
        return endpoint.reserveBase(b, address(this));
    }
    function requestCancel(BaseInventoryV2 endpoint, bytes32 id) external onlyDriver returns (V2.Message memory) {
        return endpoint.requestCancel(id);
    }
    function cancelFunded(BaseInventoryV2 endpoint, V2.Message calldata m) external onlyDriver returns (V2.Message memory) {
        return endpoint.cancelFunded(m, hex"01");
    }
    function fund(NativeReserveV2 endpoint, V2.Message calldata m) external payable onlyDriver returns (V2.Message memory) {
        return endpoint.fundNative{value:msg.value}(m, hex"01");
    }
    receive() external payable {}
}

/// @dev One fresh fixture per case. Deploy separately from the assertion suite:
/// embedding this creation code in suite runtime would obscure EIP-170 bounds.
contract InventoryV2Fixture {
    InventoryV2TokenMock public token;
    InventoryV2DigestMock public baseProof;
    InventoryV2DigestMock public nativeProof;
    InventoryV2OpenBase public base;
    InventoryV2OpenNative public nativeReserve;
    uint256 public immutable fullReserve;
    constructor(uint256 supply, uint256 baseLot, uint256 nativeLot) payable {
        token = new InventoryV2TokenMock(supply); // CREATE nonce 1
        baseProof = new InventoryV2DigestMock(); // 2
        nativeProof = new InventoryV2DigestMock(); // 3
        V2.Terms memory t;
        t.baseChainId = block.chainid; t.baseDomain = keccak256("synthetic-base-v2");
        t.baseEndpoint = _createAddress(4); t.baseToken = address(token);
        t.nativeChainId = block.chainid; t.nativeDomain = keccak256("synthetic-native-v2");
        t.nativeEndpoint = _createAddress(5); t.nativeAsset = address(0);
        t.totalBaseSupply = supply; t.baseLot = baseLot; t.nativeLot = nativeLot;
        t.baseToNativeVerifierDomain = keccak256("synthetic-base-proof-v2");
        t.nativeToBaseVerifierDomain = keccak256("synthetic-native-proof-v2"); t.maxProofBytes = 1024;
        base = new InventoryV2OpenBase(t, address(baseProof)); // 4
        nativeReserve = new InventoryV2OpenNative(t, address(nativeProof)); // 5
        require(address(base) == t.baseEndpoint && address(nativeReserve) == t.nativeEndpoint);
        fullReserve = V2.quote(t, supply);
        token.approve(address(base), type(uint256).max);
    }
    function _createAddress(uint8 nonce) private view returns (address) {
        return address(uint160(uint256(keccak256(abi.encodePacked(hex"d694", address(this), nonce)))));
    }
    function enable() external { base.setEnabled(true); nativeReserve.setEnabled(true); }
    function declare() external returns (V2.Message memory) { return base.declareSupply(); }
    function finishBootstrap(V2.Message calldata m, uint256 n) external returns (V2.Message memory ack) {
        nativeProof.approve(V2.digest(m), true);
        ack = nativeReserve.bootstrap{value:n}(m, hex"01");
        baseProof.approve(V2.digest(ack), true); base.confirmBacking(ack, hex"01");
    }
    function repeatBootstrap(V2.Message calldata m) external { nativeProof.approve(V2.digest(m), true); nativeReserve.bootstrap{value:fullReserve}(m, hex"01"); }
    function ready() external {
        base.setEnabled(true); nativeReserve.setEnabled(true);
        V2.Message memory m = base.declareSupply(); nativeProof.approve(V2.digest(m), true);
        V2.Message memory ack = nativeReserve.bootstrap{value:fullReserve}(m, hex"01");
        baseProof.approve(V2.digest(ack), true); base.confirmBacking(ack, hex"01");
    }
    function lock(uint256 b, address recipient) external returns (V2.Message memory) { return base.lockBase(b, recipient); }
    function payLock(V2.Message calldata m) external returns (V2.Message memory) {
        nativeProof.approve(V2.digest(m), true); return nativeReserve.payNative(m, hex"01");
    }
    function acknowledgeLock(V2.Message calldata m) external { baseProof.approve(V2.digest(m), true); base.acknowledgeNativePaid(m, hex"01"); }
    function forward(uint256 b) external returns (V2.Message memory ack) {
        V2.Message memory m = base.lockBase(b, address(this)); nativeProof.approve(V2.digest(m), true);
        ack = nativeReserve.payNative(m, hex"01"); baseProof.approve(V2.digest(ack), true); base.acknowledgeNativePaid(ack, hex"01");
    }
    function reserve(uint256 b, address funder) external returns (V2.Message memory) { return base.reserveBase(b, funder); }
    function fund(V2.Message calldata m) external returns (V2.Message memory) {
        nativeProof.approve(V2.digest(m), true); return nativeReserve.fundNative{value:m.nativeAmount}(m, hex"01");
    }
    function fundActor(InventoryV2NativeActor actor, V2.Message calldata m) external returns (V2.Message memory) {
        nativeProof.approve(V2.digest(m), true); return actor.fund{value:m.nativeAmount}(nativeReserve, m);
    }
    function settle(V2.Message calldata m) external returns (V2.Message memory) { baseProof.approve(V2.digest(m), true); return base.settleBase(m, hex"01"); }
    function acknowledgeBase(V2.Message calldata m) external { nativeProof.approve(V2.digest(m), true); nativeReserve.acknowledgeBasePaid(m, hex"01"); }
    function requestCancel(bytes32 id) external returns (V2.Message memory) { return base.requestCancel(id); }
    function cancelUnfunded(V2.Message calldata m) external returns (V2.Message memory) { nativeProof.approve(V2.digest(m), true); return nativeReserve.cancelUnfunded(m, hex"01"); }
    function acknowledgeCancel(V2.Message calldata m) external { baseProof.approve(V2.digest(m), true); base.acknowledgeUnfundedCancel(m, hex"01"); }
    function cancelFunded(V2.Message calldata m) external returns (V2.Message memory) { baseProof.approve(V2.digest(m), true); return base.cancelFunded(m, hex"01"); }
    function refund(V2.Message calldata m) external returns (V2.Message memory) { nativeProof.approve(V2.digest(m), true); return nativeReserve.refundNative(m, hex"01"); }
    function acknowledgeRefund(V2.Message calldata m) external { baseProof.approve(V2.digest(m), true); base.acknowledgeNativeRefund(m, hex"01"); }
    function tokenMode(InventoryV2TokenMock.Mode mode) external { token.setMode(mode); }
    function corruptSupply(uint256 amount, bool burn) external { if (burn) token.corruptBurn(amount); else token.corruptMint(amount); }
    function freeze() external { V2.Message memory m = base.signalSupplyMismatch(); nativeProof.approve(V2.digest(m), true); nativeReserve.acceptSupplyFreeze(m, hex"01"); }
    function donate(uint256 b, uint256 n) external { if (b != 0) token.transfer(address(base), b); if (n != 0) new InventoryV2ForcedDonation{value:n}(payable(address(nativeReserve))); }
    function approveOnly(bytes32 digest) external { nativeProof.approve(digest, true); }
    function noApprovalPay(V2.Message calldata m, bytes calldata proof) external { nativeReserve.payNative(m, proof); }
    function nativeFundRaw(V2.Message calldata m, bytes calldata proof, uint256 n) external { nativeReserve.fundNative{value:n}(m, proof); }
    function clientReserve(InventoryV2BaseClient actor, uint256 b) external returns (V2.Message memory) { return actor.reserve(base, b); }
    function clientRequestCancel(InventoryV2BaseClient actor, bytes32 id) external returns (V2.Message memory) { return actor.requestCancel(base, id); }
    function clientFund(InventoryV2BaseClient actor, V2.Message calldata m) external returns (V2.Message memory) {
        nativeProof.approve(V2.digest(m), true); return actor.fund{value:m.nativeAmount}(nativeReserve, m);
    }
    function clientCancelFunded(InventoryV2BaseClient actor, V2.Message calldata m) external returns (V2.Message memory) {
        baseProof.approve(V2.digest(m), true); return actor.cancelFunded(base, m);
    }
    function stateHash() external view returns (bytes32) {
        bytes32 a = keccak256(abi.encode(base.backingReady(), base.supplyDeclared(), base.supplyFrozen(),
            base.accountedBase(), base.freeBase(), base.pendingNativeBase(), base.reservedBase(), base.nextNonce()));
        bytes32 b = keccak256(abi.encode(nativeReserve.backingInitialized(), nativeReserve.supplyFrozen(),
            nativeReserve.accountedReserve(), nativeReserve.pendingFunds(), nativeReserve.nextNonce(), address(nativeReserve).balance));
        bytes32 c = keccak256(abi.encode(token.totalSupply(), token.balanceOf(address(base)),
            token.balanceOf(address(this)), address(this).balance));
        return keccak256(abi.encode(a, b, c));
    }
    receive() external payable {}
}
