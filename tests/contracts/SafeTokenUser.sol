// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;
import "./openzeppelin-v5.0.2/contracts/token/ERC20/utils/SafeERC20.sol";
contract SafeTokenUser {
    using SafeERC20 for IERC20;
    function sendToken(IERC20 token, address recipient, uint256 amount) external {
        token.safeTransfer(recipient, amount);
    }
}
// Deliberately nonstandard adversarial tokens, test-only.
contract FalseToken {
    uint256 public calls;
    function transfer(address, uint256) external returns (bool) { calls++; return false; }
}
contract EmptyReturnToken {
    uint256 public amount;
    function transfer(address, uint256 value) external { amount = value; }
}
