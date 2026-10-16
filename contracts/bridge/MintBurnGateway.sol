// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

import {IERC20} from "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import {SafeERC20} from "@openzeppelin/contracts/token/ERC20/utils/SafeERC20.sol";
import {BridgeTypes, IBridgeMintBurnToken} from "./API.sol";
import {BaseBridgeEndpoint} from "./BaseBridgeEndpoint.sol";

/// @notice Routes an existing mint/burn token. It neither implements a token nor
/// asserts that its token is USDC, WNANO or backed by an authenticated remote chain.
abstract contract MintBurnGateway is BaseBridgeEndpoint {
    using SafeERC20 for IERC20;

    event Minted(bytes32 indexed digest, bytes32 indexed sourceKey, BridgeTypes.Message message);
    event Burned(bytes32 indexed digest, bytes32 indexed sourceKey, BridgeTypes.Message message);

    constructor(BridgeTypes.EndpointConfig memory config) BaseBridgeEndpoint(config) {}

    function mint(BridgeTypes.Message calldata message, bytes calldata proof) external nonReentrant {
        _checkRouteEnabled();
        (bytes32 digest, bytes32 sourceKey) = _consumeInbound(message, proof, BridgeTypes.Kind.LOCK);
        uint256 supplyBefore = localToken.totalSupply();
        if (supplyBefore != outstanding) revert TokenAccountingMismatch();
        uint256 recipientBefore = localToken.balanceOf(message.recipient);
        _increaseOutstanding(message.amount);
        if (!IBridgeMintBurnToken(address(localToken)).mint(message.recipient, message.amount)) {
            revert TokenAccountingMismatch();
        }
        uint256 supplyAfter = localToken.totalSupply();
        uint256 recipientAfter = localToken.balanceOf(message.recipient);
        if (supplyAfter != supplyBefore + message.amount ||
            recipientAfter != recipientBefore + message.amount) revert TokenAccountingMismatch();
        emit Minted(digest, sourceKey, message);
    }

    function burnForRelease(uint256 amount, address recipient) external nonReentrant returns (bytes32 digest) {
        _checkRouteEnabled();
        if (amount == 0 || recipient == address(0) || recipient == remoteEndpoint) {
            revert InvalidAmountOrRecipient();
        }
        uint256 supplyBefore = localToken.totalSupply();
        if (supplyBefore != outstanding) revert TokenAccountingMismatch();
        uint256 balanceBefore = localToken.balanceOf(address(this));
        uint256 senderBefore = localToken.balanceOf(msg.sender);
        _decreaseOutstanding(amount);
        localToken.safeTransferFrom(msg.sender, address(this), amount);
        uint256 balancePulled = localToken.balanceOf(address(this));
        uint256 senderAfterPull = localToken.balanceOf(msg.sender);
        if (balancePulled != balanceBefore + amount || senderBefore != senderAfterPull + amount ||
            localToken.totalSupply() != supplyBefore) revert TokenAccountingMismatch();
        IBridgeMintBurnToken(address(localToken)).burn(amount);
        uint256 balanceAfter = localToken.balanceOf(address(this));
        uint256 supplyAfter = localToken.totalSupply();
        if (balanceAfter != balanceBefore || supplyBefore != supplyAfter + amount ||
            localToken.balanceOf(msg.sender) != senderAfterPull) revert TokenAccountingMismatch();
        BridgeTypes.Message memory message;
        bytes32 sourceKey;
        (message, digest, sourceKey) = _recordOutbound(BridgeTypes.Kind.BURN, amount, recipient);
        emit Burned(digest, sourceKey, message);
    }
}
