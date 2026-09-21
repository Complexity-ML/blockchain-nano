// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
interface IWrappedNano {
    function deposit() external payable;
    function withdraw(uint256 amount) external;
}
contract WrappedNanoReceiver {
    IWrappedNano public immutable token;
    bool public rejectRefund;
    bool public attemptReentry;
    bool public reentrySucceeded;
    uint256 public reentryAttempts;
    constructor(address wrapper) { token = IWrappedNano(wrapper); }
    function deposit() external payable { token.deposit{value: msg.value}(); }
    function configure(bool reject, bool reenter) external { rejectRefund = reject; attemptReentry = reenter; }
    function withdraw(uint256 amount) external { token.withdraw(amount); }
    receive() external payable {
        require(!rejectRefund, "refund rejected");
        if (attemptReentry) {
            ++reentryAttempts;
            (reentrySucceeded,) = address(token).call(abi.encodeCall(token.withdraw, (1)));
        }
    }
}
