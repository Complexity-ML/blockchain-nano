// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;
import "@openzeppelin/contracts/token/ERC721/IERC721.sol";

// Deliberately vulnerable LOCAL TEST FIXTURE. Never deploy as a real market.
contract DeveloperVulnerableMarket {
    mapping(address => mapping(uint256 => address)) public sellers;
    function list(address nft, uint256 id) external {
        IERC721(nft).transferFrom(msg.sender, address(this), id);
        sellers[nft][id] = msg.sender;
    }
    // Missing seller authorization: an attacker steals the escrowed NFT.
    function cancel(address nft, uint256 id) external {
        require(sellers[nft][id] != address(0), "no listing");
        delete sellers[nft][id];
        IERC721(nft).transferFrom(address(this), msg.sender, id);
    }
    uint256 public writes;
    function exhaustGas() external {
        while (true) { unchecked { writes++; } }
    }
}
