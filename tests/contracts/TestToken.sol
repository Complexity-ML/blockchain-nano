// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;
import "./openzeppelin-v5.0.2/contracts/token/ERC20/ERC20.sol";
// Test fixture only: fixed supply, no privileged mint function.
contract TestToken is ERC20 {
    constructor() ERC20("Nano Test Token", "NTT") {
        _mint(msg.sender, 1000000 * 10 ** 18);
    }
}
