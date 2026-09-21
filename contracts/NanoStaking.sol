// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

/// @notice Native-coin staking registry consumed by Nano's PoS engine.
/// @dev No owner, proxy or administrative withdrawal. Rewards are paid by consensus.
contract NanoStaking {
    uint256 public constant MIN_STAKE = 10_000 ether;
    uint256 public constant LOCK_BLOCKS = 100;
    uint256 public constant MAX_VALIDATORS = 1000;
    struct Deposit { uint256 amount; uint256 lockedUntil; }
    mapping(address => Deposit) private deposits;
    address[] private validators;
    mapping(address => uint256) private positions;
    bool private entered;

    event Registered(address indexed validator, uint256 amount, uint256 lockedUntil);
    event StakeAdded(address indexed validator, uint256 amount, uint256 lockedUntil);
    event Withdrawn(address indexed validator, uint256 amount);

    modifier nonReentrant() {
        require(!entered, "reentrant call");
        entered = true;
        _;
        entered = false;
    }

    /// @dev The deployment transaction funds the first validator, avoiding an empty registry.
    constructor() payable { _register(msg.sender, msg.value); }

    function registerValidator() external payable nonReentrant { _register(msg.sender, msg.value); }

    function _register(address account, uint256 amount) private {
        require(amount >= MIN_STAKE, "stake below minimum");
        require(positions[account] == 0, "already registered");
        require(validators.length < MAX_VALIDATORS, "registry full");
        uint256 until = block.number + LOCK_BLOCKS;
        deposits[account] = Deposit(amount, until);
        validators.push(account);
        positions[account] = validators.length;
        emit Registered(account, amount, until);
    }

    function addStake() external payable nonReentrant {
        require(positions[msg.sender] != 0, "not registered");
        require(msg.value > 0, "zero deposit");
        Deposit storage deposit = deposits[msg.sender];
        deposit.amount += msg.value;
        deposit.lockedUntil = block.number + LOCK_BLOCKS;
        emit StakeAdded(msg.sender, msg.value, deposit.lockedUntil);
    }

    function withdrawStake() external nonReentrant {
        uint256 position = positions[msg.sender];
        require(position != 0, "not registered");
        Deposit memory deposit = deposits[msg.sender];
        require(block.number >= deposit.lockedUntil, "stake locked");
        // Keep a producer available. The last validator can exit after a replacement joins.
        require(validators.length > 1, "last validator");
        address last = validators[validators.length - 1];
        validators[position - 1] = last;
        positions[last] = position;
        validators.pop();
        delete positions[msg.sender];
        delete deposits[msg.sender];
        (bool ok,) = payable(msg.sender).call{value: deposit.amount}("");
        require(ok, "withdrawal transfer failed");
        emit Withdrawn(msg.sender, deposit.amount);
    }

    function getValidators() external view returns (address[] memory) { return validators; }
    function getStake(address account) external view returns (uint256) { return deposits[account].amount; }
    function isValidator(address account) external view returns (bool) { return positions[account] != 0; }
    function lockedUntil(address account) external view returns (uint256) { return deposits[account].lockedUntil; }
}
