// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

import {IERC20} from "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import {SafeERC20} from "@openzeppelin/contracts/token/ERC20/utils/SafeERC20.sol";
import {BridgeTypes} from "./API.sol";
import {BaseBridgeEndpoint} from "./BaseBridgeEndpoint.sol";

/// @notice ERC-20 canonical escrow only; no native-currency or economic conversion.
abstract contract CanonicalEscrow is BaseBridgeEndpoint {
    using SafeERC20 for IERC20;

    event Locked(bytes32 indexed digest, bytes32 indexed sourceKey, BridgeTypes.Message message);
    event Released(bytes32 indexed digest, bytes32 indexed sourceKey, BridgeTypes.Message message);

    constructor(BridgeTypes.EndpointConfig memory config) BaseBridgeEndpoint(config) {}

    function lock(uint256 amount, address recipient) external nonReentrant returns (bytes32 digest) {
        _checkRouteEnabled();
        if (amount == 0 || recipient == address(0) || recipient == remoteEndpoint) {
            revert InvalidAmountOrRecipient();
        }
        uint256 balanceBefore = localToken.balanceOf(address(this));
        if (balanceBefore < outstanding) revert TokenAccountingMismatch();
        uint256 senderBefore = localToken.balanceOf(msg.sender);
        _increaseOutstanding(amount);
        localToken.safeTransferFrom(msg.sender, address(this), amount);
        uint256 balanceAfter = localToken.balanceOf(address(this));
        uint256 senderAfter = localToken.balanceOf(msg.sender);
        if (balanceAfter != balanceBefore + amount || senderBefore != senderAfter + amount) {
            revert TokenAccountingMismatch();
        }
        BridgeTypes.Message memory message;
        bytes32 sourceKey;
        (message, digest, sourceKey) = _recordOutbound(BridgeTypes.Kind.LOCK, amount, recipient);
        emit Locked(digest, sourceKey, message);
    }

    function release(BridgeTypes.Message calldata message, bytes calldata proof) external nonReentrant {
        _checkRouteEnabled();
        (bytes32 digest, bytes32 sourceKey) = _consumeInbound(message, proof, BridgeTypes.Kind.BURN);
        uint256 balanceBefore = localToken.balanceOf(address(this));
        if (balanceBefore < outstanding) revert TokenAccountingMismatch();
        uint256 recipientBefore = localToken.balanceOf(message.recipient);
        _decreaseOutstanding(message.amount);
        localToken.safeTransfer(message.recipient, message.amount);
        uint256 balanceAfter = localToken.balanceOf(address(this));
        uint256 recipientAfter = localToken.balanceOf(message.recipient);
        if (balanceBefore != balanceAfter + message.amount ||
            recipientAfter != recipientBefore + message.amount || balanceAfter < outstanding) {
            revert TokenAccountingMismatch();
        }
        emit Released(digest, sourceKey, message);
    }
}
