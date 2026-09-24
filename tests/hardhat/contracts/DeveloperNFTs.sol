// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;
import "@openzeppelin/contracts/token/ERC721/ERC721.sol";
import "@openzeppelin/contracts/token/ERC1155/ERC1155.sol";
import "@openzeppelin/contracts/token/ERC721/utils/ERC721Holder.sol";
import "@openzeppelin/contracts/token/ERC1155/utils/ERC1155Holder.sol";

// Local qualification fixtures; these are not deployment contracts.
contract DeveloperNFT721 is ERC721 {
    constructor() ERC721("Nano Test NFT", "T721") { _mint(msg.sender, 1); }
}
contract DeveloperNFT1155 is ERC1155 {
    constructor() ERC1155("test://{id}") {
        _mint(msg.sender, 1, 10, "");
        _mint(msg.sender, 2, 20, "");
    }
}
contract DeveloperNFTReceiver is ERC721Holder, ERC1155Holder {
    bool public reject = true;
    uint256 public calls;
    function configure(bool value) external { reject = value; }
    function onERC721Received(address, address, uint256, bytes memory) public override returns(bytes4) {
        ++calls;
        return reject ? bytes4(0) : this.onERC721Received.selector;
    }
    function onERC1155BatchReceived(address, address, uint256[] memory, uint256[] memory, bytes memory) public override returns(bytes4) {
        ++calls;
        return reject ? bytes4(0) : this.onERC1155BatchReceived.selector;
    }
}
