// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "@openzeppelin/contracts/utils/ReentrancyGuard.sol";

/// @notice Native NANO wrapper. One base unit deposited backs one WNANO unit.
/// @dev No owner, proxy, external mint authority or administrative withdrawal.
contract WrappedNano is ERC20, ReentrancyGuard {
    error NativeTransferFailed();
    event Deposit(address indexed account, uint256 amount);
    event Withdrawal(address indexed account, uint256 amount);

    constructor() ERC20("Wrapped Nano", "WNANO") {}

    receive() external payable { deposit(); }

    function deposit() public payable nonReentrant {
        _mint(msg.sender, msg.value);
        emit Deposit(msg.sender, msg.value);
    }

    function withdraw(uint256 amount) external nonReentrant {
        _burn(msg.sender, amount);
        (bool sent,) = payable(msg.sender).call{value: amount}("");
        if (!sent) revert NativeTransferFailed();
        emit Withdrawal(msg.sender, amount);
    }
}
