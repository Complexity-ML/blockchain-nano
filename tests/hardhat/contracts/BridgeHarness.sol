// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

import {ERC20} from "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import {IERC20} from "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import {BridgeTypes, IBridgeMessageVerifier} from "../../../contracts/bridge/API.sol";
import {CanonicalEscrow} from "../../../contracts/bridge/CanonicalEscrow.sol";
import {MintBurnGateway} from "../../../contracts/bridge/MintBurnGateway.sol";

/// @dev Local synthetic controls only. None of these actors is a production route.
abstract contract BridgeTestControl {
    error NotTestOwner();
    address public immutable testOwner = msg.sender;

    modifier onlyTestOwner() {
        if (msg.sender != testOwner) revert NotTestOwner();
        _;
    }
}

/// @dev Approval of a digest is NOT a source-event, inclusion or finality proof.
contract MockProofVerifier is IBridgeMessageVerifier, BridgeTestControl {
    error InvalidTestCallback();
    mapping(bytes32 => bool) public approved;
    address public viewCallbackTarget;
    bytes public viewCallbackData;
    bytes4 public expectedCallbackError;

    function configureDigest(bytes32 digest, bool value) external onlyTestOwner {
        approved[digest] = value;
    }

    function approveDigest(bytes32 digest, bool value) external onlyTestOwner {
        approved[digest] = value;
    }

    /// @dev A successful verification with this hook also proves the nested
    /// static call reverted with the exact error selected by the test driver.
    /// Select ReentrancyGuardReentrantCall(), not merely any static-call error.
    function configureViewCallback(address target, bytes calldata data, bytes4 expectedError)
        external onlyTestOwner
    {
        if (target == address(0)) {
            if (data.length != 0 || expectedError != bytes4(0)) revert InvalidTestCallback();
        } else if (target.code.length == 0 || data.length < 4 || expectedError == bytes4(0)) {
            revert InvalidTestCallback();
        }
        viewCallbackTarget = target;
        viewCallbackData = data;
        expectedCallbackError = expectedError;
    }

    function verifyMessage(bytes32 digest, bytes calldata proof) external view override returns (bool) {
        if (!approved[digest] || (proof.length != 32 && proof.length != 64)) return false;
        bytes32 expectedProof = proof.length == 32
            ? keccak256(abi.encode(digest)) : keccak256(abi.encode(digest, uint256(1)));
        if (keccak256(proof) != expectedProof) return false;
        if (viewCallbackTarget != address(0)) {
            (bool success, bytes memory result) = viewCallbackTarget.staticcall(viewCallbackData);
            if (success || result.length < 4 || bytes4(result) != expectedCallbackError) return false;
        }
        return true;
    }
}

interface IBridgeTokenReceiver {
    function onBridgeTokenTransfer(address from, uint256 amount) external;
}

/// @dev ERC20 state and allowance accounting stay in the public OZ 5.0.2 base.
/// Faucet, failure modes and receiver hooks are intentionally synthetic.
contract BridgeToken is ERC20, BridgeTestControl {
    enum TransferMode { Normal, ReturnFalse, NoOp, Tax, ExtraDebit, NoReturn }
    enum MintMode { Normal, ReturnFalse, NoOp, ExtraSupply }
    enum BurnMode { Normal, Revert, NoOp, SupplyMismatch }

    error InvalidTestBridge();
    error OnlyTestBridge();
    error CanonicalFaucetOnly();
    error RepresentationCorruptionOnly();
    error TestBurnFailed();
    error InvalidTestRecipient();

    bool public immutable canonicalAsset;
    address public bridge;
    address public constant SUPPLY_SINK = address(uint160(0xBEEF));
    TransferMode public transferMode;
    TransferMode public transferFromMode;
    MintMode public mintMode;
    BurnMode public burnMode;
    address public callbackRecipient;
    bool public recipientCallbackEnabled;

    constructor(string memory name_, string memory symbol_, bool canonical_)
        ERC20(name_, symbol_)
    {
        canonicalAsset = canonical_;
    }

    function decimals() public pure override returns (uint8) { return 6; }

    function setBridge(address value) external onlyTestOwner {
        if (bridge != address(0) || value == address(0) || value.code.length == 0) {
            revert InvalidTestBridge();
        }
        bridge = value;
    }

    function configureModes(TransferMode transfer_, TransferMode transferFrom_,
        MintMode mint_, BurnMode burn_) external onlyTestOwner
    {
        transferMode = transfer_;
        transferFromMode = transferFrom_;
        mintMode = mint_;
        burnMode = burn_;
    }

    function configureRecipientCallback(address recipient, bool enabled) external onlyTestOwner {
        if (enabled && (recipient == address(0) || recipient.code.length == 0)) {
            revert InvalidTestRecipient();
        }
        callbackRecipient = recipient;
        recipientCallbackEnabled = enabled;
    }

    /// @dev Test genesis funding only on the canonical token. The wrapped token
    /// has no faucet and starts with zero supply. Not an issuer/minter design.
    function faucet(address recipient, uint256 amount) external onlyTestOwner {
        if (!canonicalAsset) revert CanonicalFaucetOnly();
        _mint(recipient, amount);
    }

    /// @dev Deliberate test-only issuer compromise. It must make the gateway's
    /// supply/outstanding precondition fail; never a representation faucet.
    function corruptSupply(address recipient, uint256 amount) external onlyTestOwner {
        if (canonicalAsset) revert RepresentationCorruptionOnly();
        _mint(recipient, amount);
    }

    function mint(address recipient, uint256 amount) external returns (bool) {
        if (msg.sender != bridge) revert OnlyTestBridge();
        if (mintMode == MintMode.ReturnFalse) return false;
        if (mintMode == MintMode.NoOp) return true;
        _mint(recipient, amount);
        // Recipient delta is exact while total supply is intentionally wrong.
        if (mintMode == MintMode.ExtraSupply) _mint(SUPPLY_SINK, 1);
        return true;
    }

    function burn(uint256 amount) external {
        if (msg.sender != bridge) revert OnlyTestBridge();
        if (burnMode == BurnMode.Revert) revert TestBurnFailed();
        if (burnMode == BurnMode.NoOp) return;
        _burn(msg.sender, amount);
        // Gateway debit is exact while total supply falls by one unit too few.
        if (burnMode == BurnMode.SupplyMismatch) _mint(SUPPLY_SINK, 1);
    }

    function transfer(address to, uint256 value) public override returns (bool) {
        if (transferMode == TransferMode.ReturnFalse) return false;
        bool success = super.transfer(to, value);
        if (transferMode == TransferMode.NoReturn) {
            assembly ("memory-safe") { return(0, 0) }
        }
        return success;
    }

    function transferFrom(address from, address to, uint256 value) public override returns (bool) {
        if (transferFromMode == TransferMode.ReturnFalse) return false;
        bool success = super.transferFrom(from, to, value);
        if (transferFromMode == TransferMode.NoReturn) {
            assembly ("memory-safe") { return(0, 0) }
        }
        return success;
    }

    function _update(address from, address to, uint256 value) internal override {
        if (from != address(0) && to != address(0)) {
            // These are the only two external transfer entry points. Mint,
            // burn and faucet use a zero endpoint and bypass transfer modes.
            TransferMode mode = msg.sig == IERC20.transferFrom.selector
                ? transferFromMode : transferMode;
            if (mode == TransferMode.NoOp) return;
            if (mode == TransferMode.Tax || mode == TransferMode.ExtraDebit) {
                uint256 fee = value == 0 ? 0 : value / 100;
                if (value != 0 && fee == 0) fee = 1;
                super._update(from, address(0), fee);
                super._update(from, to, mode == TransferMode.Tax ? value - fee : value);
            } else {
                super._update(from, to, value);
            }
        } else {
            super._update(from, to, value);
        }
        if (recipientCallbackEnabled && to == callbackRecipient && to != address(0)) {
            IBridgeTokenReceiver(to).onBridgeTokenTransfer(from, value);
        }
    }
}

/// @dev Local enablement must be explicitly toggled by the deployer in tests.
contract MockCanonicalEscrow is CanonicalEscrow, BridgeTestControl {
    error TestRouteDisabled();
    bool public enabled;

    constructor(BridgeTypes.EndpointConfig memory config) CanonicalEscrow(config) {}

    function setEnabled(bool value) external onlyTestOwner { enabled = value; }

    function _requireRouteEnabled() internal view override {
        if (!enabled) revert TestRouteDisabled();
    }
}

contract MockMintBurnGateway is MintBurnGateway, BridgeTestControl {
    error TestRouteDisabled();
    bool public enabled;

    constructor(BridgeTypes.EndpointConfig memory config) MintBurnGateway(config) {}

    function setEnabled(bool value) external onlyTestOwner { enabled = value; }

    function _requireRouteEnabled() internal view override {
        if (!enabled) revert TestRouteDisabled();
    }
}

/// @dev Token-recipient callback probe. A failed outer transaction also rolls
/// back these counters/results; the driver must check endpoint state rollback.
contract BridgeReentrantRecipient is IBridgeTokenReceiver, BridgeTestControl {
    error WrongTestToken();
    error InvalidTestReentry();
    error TestRecipientRefused();

    address public immutable token;
    address public target;
    bytes public reentryData;
    bool public revertAfterCallback;
    uint256 public callbackCount;
    bool public reentryAttempted;
    bool public reentrySucceeded;
    bytes public reentryResult;
    address public lastFrom;
    uint256 public lastAmount;

    constructor(address token_) {
        if (token_ == address(0) || token_.code.length == 0) revert WrongTestToken();
        token = token_;
    }

    function configureReentry(address target_, bytes calldata data_, bool refuseAfter)
        external onlyTestOwner
    {
        if (target_ == address(0)) {
            if (data_.length != 0) revert InvalidTestReentry();
        } else if (target_.code.length == 0 || data_.length < 4) {
            revert InvalidTestReentry();
        }
        target = target_;
        reentryData = data_;
        revertAfterCallback = refuseAfter;
        reentryAttempted = false;
        reentrySucceeded = false;
        delete reentryResult;
    }

    /// @notice Committed callback evidence only; an outer revert undoes it.
    function callbackAttempted() external view returns (bool) { return reentryAttempted; }

    function callbackBlocked() external view returns (bool) {
        bytes memory result = reentryResult;
        return reentryAttempted && !reentrySucceeded && result.length >= 4 &&
            bytes4(result) == bytes4(keccak256("ReentrancyGuardReentrantCall()"));
    }

    function callbackReturnHash() external view returns (bytes32) {
        return reentryAttempted ? keccak256(reentryResult) : bytes32(0);
    }

    function onBridgeTokenTransfer(address from, uint256 amount) external override {
        if (msg.sender != token) revert WrongTestToken();
        ++callbackCount;
        lastFrom = from;
        lastAmount = amount;
        if (target != address(0)) {
            reentryAttempted = true;
            (reentrySucceeded, reentryResult) = target.call(reentryData);
        }
        if (revertAfterCallback) revert TestRecipientRefused();
    }
}
