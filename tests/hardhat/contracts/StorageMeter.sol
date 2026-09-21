// SPDX-License-Identifier: MIT
pragma solidity ^0.8.28;

// Test fixture: expose every intermediate write so optimizer cannot elide it.
contract StorageMeter {
    uint256 public value = 1;
    event Written(uint256 value);

    function write(uint256[] calldata values, bool rollback) external {
        for (uint256 i; i < values.length; ++i) {
            value = values[i];
            emit Written(value);
        }
        require(!rollback, "rollback");
    }

    function nested(bool rollback) external {
        value = 0;
        uint256[] memory values = new uint256[](1);
        values[0] = 1;
        try this.write(values, rollback) {} catch {}
        emit Written(value);
    }
}
