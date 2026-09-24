// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
interface ITestV2Pair {
    function token0() external view returns(address);
    function swap(uint256 amount0, uint256 amount1, address to, bytes calldata data) external;
}
// Local callback fixture, not a public lending product.
contract DeveloperFlashBorrower {
    ITestV2Pair public immutable pair;
    IERC20 public immutable token;
    uint256 public calls;
    uint256 public observedBalance;
    bool public nestedSucceeded;
    bytes public nestedResult;
    constructor(address pairAddress, address tokenAddress) { pair=ITestV2Pair(pairAddress); token=IERC20(tokenAddress); }
    function borrow(uint256 amount, uint256 repayment, bool reenter) external {
        bool first=pair.token0()==address(token);
        pair.swap(first?amount:0, first?0:amount, address(this), abi.encode(repayment,reenter));
    }
    function uniswapV2Call(address sender, uint256, uint256, bytes calldata data) external {
        require(msg.sender==address(pair) && sender==address(this), "unexpected callback");
        ++calls;
        observedBalance=token.balanceOf(address(this));
        (uint256 repayment,bool reenter)=abi.decode(data,(uint256,bool));
        if(reenter) { (nestedSucceeded,nestedResult)=address(pair).call(abi.encodeWithSignature("sync()")); }
        if(repayment!=0)require(token.transfer(address(pair),repayment), "repayment failed");
    }
}
