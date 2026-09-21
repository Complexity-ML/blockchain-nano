// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

interface Registry {
    function registerValidator() external payable;
    function withdrawStake() external;
}

// Adversarial test fixture, not an application contract.
contract StakingReceiver {
    Registry public registry;
    bool public rejectRefund = true;
    bool public reentryRejected;
    constructor(address target) { registry = Registry(target); }
    function register() external payable { registry.registerValidator{value: msg.value}(); }
    function withdraw() external { registry.withdrawStake(); }
    function acceptRefund() external { rejectRefund = false; }
    receive() external payable {
        require(!rejectRefund, "refund rejected");
        (bool success,) = address(registry).call(abi.encodeWithSignature("withdrawStake()"));
        require(!success, "reentry succeeded");
        reentryRejected = true;
    }
}
