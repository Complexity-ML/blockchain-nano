#pragma once
#include "core.hpp"
namespace nano {
struct Receipt {
 unsigned type=0;
 bool success=false;
 Int gas_used=0,cumulative_gas_used=0,block_number=0,index=0;
 Bytes transaction_hash=Bytes(32),block_hash=Bytes(32);
 std::string sender="0x0000000000000000000000000000000000000000";
 std::optional<std::string> to,contract_address;
 std::vector<Log> logs;
 Bytes encode() const;
 Json to_json() const;
 static Receipt from_json(const Json& json);
};
Bytes receipts_root(const std::vector<Receipt>& receipts);
}
