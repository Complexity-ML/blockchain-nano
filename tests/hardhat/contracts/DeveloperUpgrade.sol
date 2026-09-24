// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import "@openzeppelin/contracts/proxy/utils/Initializable.sol";
import "@openzeppelin/contracts/proxy/utils/UUPSUpgradeable.sol";
import "@openzeppelin/contracts/proxy/ERC1967/ERC1967Proxy.sol";

// Fixtures only. Storage layouts are intentionally compatible for this test.
contract DeveloperUpgradeV1 is Initializable, UUPSUpgradeable {
    address public owner;
    uint256 public value;
    constructor() { _disableInitializers(); }
    function initialize(address initialOwner, uint256 initialValue) external initializer {
        owner = initialOwner; value = initialValue;
    }
    function setValue(uint256 next) external { require(msg.sender == owner, "owner only"); value = next; }
    function version() external pure virtual returns(uint256) { return 1; }
    function _authorizeUpgrade(address) internal view override { require(msg.sender == owner, "owner only"); }
}
contract DeveloperUpgradeV2 is DeveloperUpgradeV1 {
    uint256 public migrationValue;
    function version() external pure override returns(uint256) { return 2; }
    function migrate(uint256 next) external reinitializer(2) {
        require(msg.sender == owner, "owner only");
        migrationValue = next;
        require(next != 99, "fixture migration failure");
    }
}
contract DeveloperBadProxiable {
    function proxiableUUID() external pure returns(bytes32) { return bytes32(0); }
}
contract DeveloperProxy is ERC1967Proxy {
    constructor(address implementation, bytes memory data) ERC1967Proxy(implementation, data) {}
}
