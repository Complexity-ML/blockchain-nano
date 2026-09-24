// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import "@openzeppelin/contracts/token/ERC20/extensions/ERC20Permit.sol";
// Deterministic local qualification token; no deployment authority implied.
contract DeveloperPermitToken is ERC20Permit {
    constructor() ERC20("Nano Permit Test", "NPT") ERC20Permit("Nano Permit Test") {
        _mint(msg.sender, 1000000);
    }
}
