// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
/// @dev Local test asset only. No peg, issuer reserves or redemption promise.
contract PoolQuoteToken is ERC20 {
    constructor() ERC20("Pool Test Quote", "TESTQ") { _mint(msg.sender, 1000000 * 10 ** 6); }
    function decimals() public pure override returns (uint8) { return 6; }
}
