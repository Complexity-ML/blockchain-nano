// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import "@openzeppelin/contracts/token/ERC20/extensions/ERC20Votes.sol";
import "@openzeppelin/contracts/governance/TimelockController.sol";
import "@openzeppelin/contracts/utils/cryptography/EIP712.sol";

// Local qualification only: synthetic assets and administrators.
contract DeveloperVotes is ERC20Votes {
    constructor() ERC20("Nano Votes Test", "NVT") EIP712("Nano Votes Test", "1") {
        _mint(msg.sender, 1000000);
    }
}
contract DeveloperTimelock is TimelockController {
    constructor(address owner) TimelockController(1, _one(owner), _one(owner), owner) {}
    function _one(address owner) private pure returns (address[] memory result) {
        result = new address[](1); result[0] = owner;
    }
}
contract DeveloperGovernedTarget {
    address public immutable controller;
    uint256 public value;
    constructor(address controller_) { controller = controller_; }
    function setValue(uint256 value_) external {
        require(msg.sender == controller, "controller only");
        value = value_;
    }
}
