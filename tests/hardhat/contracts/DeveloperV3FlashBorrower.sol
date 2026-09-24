// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
interface ITestV3FlashPool {
    function token0() external view returns(address);
    function token1() external view returns(address);
    function flash(address recipient,uint256 amount0,uint256 amount1,bytes calldata data) external;
}
// Adversarial local test fixture; deliberately unrestricted entry point.
contract DeveloperV3FlashBorrower {
    ITestV3FlashPool public immutable pool;
    IERC20 public immutable token0;
    IERC20 public immutable token1;
    uint256 public calls;
    uint256 public observed0;
    uint256 public observed1;
    bool public nestedSucceeded;
    bytes public nestedResult;
    constructor(address target) {
        pool=ITestV3FlashPool(target);
        token0=IERC20(pool.token0()); token1=IERC20(pool.token1());
    }
    function borrow(uint256 amount0,uint256 amount1,uint256 repay0,uint256 repay1,bool reenter) external {
        pool.flash(address(this),amount0,amount1,abi.encode(repay0,repay1,reenter));
    }
    function uniswapV3FlashCallback(uint256,uint256,bytes calldata data) external {
        require(msg.sender==address(pool),"unexpected callback");
        ++calls; observed0=token0.balanceOf(address(this)); observed1=token1.balanceOf(address(this));
        (uint256 repay0,uint256 repay1,bool reenter)=abi.decode(data,(uint256,uint256,bool));
        if(reenter) (nestedSucceeded,nestedResult)=address(pool).call(abi.encodeWithSelector(pool.flash.selector,address(this),0,0,bytes("")));
        if(repay0!=0)require(token0.transfer(address(pool),repay0),"repay0 failed");
        if(repay1!=0)require(token1.transfer(address(pool),repay1),"repay1 failed");
    }
}
