// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;
import "./openzeppelin-v5.0.2/contracts/token/ERC721/ERC721.sol";
import "./openzeppelin-v5.0.2/contracts/token/ERC1155/ERC1155.sol";
contract TestNFT721 is ERC721 {
    constructor() ERC721("Nano Test NFT", "NNFT") { _mint(msg.sender, 1); }
}
contract TestNFT1155 is ERC1155 {
    constructor() ERC1155("ipfs://test/{id}.json") { _mint(msg.sender, 1, 10, ""); }
}

// Test-only receiver: mode 0 accepts, 1 returns the wrong selector,
// 2 reverts after writing state, 3 forwards during the callback.
contract TestNFTReceiver {
    uint256 public mode;
    uint256 public calls;
    address public forwardTo;
    function configure(uint256 value, address recipient) external {
        mode = value;
        forwardTo = recipient;
    }
    function onERC721Received(address, address, uint256 id, bytes calldata) external returns (bytes4) {
        calls++;
        require(mode != 2, "receiver rejected");
        if (mode == 3) IERC721(msg.sender).transferFrom(address(this), forwardTo, id);
        return mode == 1 ? bytes4(0) : this.onERC721Received.selector;
    }
    function onERC1155Received(address, address, uint256 id, uint256 amount, bytes calldata) external returns (bytes4) {
        calls++;
        require(mode != 2, "receiver rejected");
        if (mode == 3) IERC1155(msg.sender).safeTransferFrom(address(this), forwardTo, id, amount, "");
        return mode == 1 ? bytes4(0) : this.onERC1155Received.selector;
    }
    function onERC1155BatchReceived(address, address, uint256[] calldata ids, uint256[] calldata amounts, bytes calldata) external returns (bytes4) {
        calls++;
        require(mode != 2, "receiver rejected");
        if (mode == 3) IERC1155(msg.sender).safeBatchTransferFrom(address(this), forwardTo, ids, amounts, "");
        return mode == 1 ? bytes4(0) : this.onERC1155BatchReceived.selector;
    }
}
