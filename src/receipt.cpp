#include "nano/receipt.hpp"
namespace nano {
Bytes Receipt::encode()const{
 if(type>2)throw std::invalid_argument("unsupported receipt type");
 std::vector<Rlp> encoded_logs;
 for(const auto& log:logs){std::vector<Rlp> topics;for(const auto& topic:log.topics){if(topic.size()!=32)throw std::invalid_argument("invalid topic width");topics.emplace_back(topic);}encoded_logs.emplace_back(std::vector<Rlp>{Rlp(unhex(address(log.address))),Rlp(topics),Rlp(log.data)});}
 auto raw=Rlp(std::vector<Rlp>{Rlp(Int(success?1:0)),Rlp(cumulative_gas_used),Rlp(logs_bloom(logs)),Rlp(encoded_logs)}).encode();if(type)raw.insert(raw.begin(),uint8_t(type));return raw;
}
Json Receipt::to_json()const{
 Json out={{"transactionHash",hex(transaction_hash)},{"blockHash",hex(block_hash)},{"blockNumber",quantity(block_number)},{"transactionIndex",quantity(index)},{"from",checksum_address(sender)},{"gasUsed",quantity(gas_used)},{"cumulativeGasUsed",quantity(cumulative_gas_used)},{"status",success?"0x1":"0x0"},{"type",quantity(Int(type))},{"logsBloom",hex(logs_bloom(logs))},{"logs",Json::array()}};
 out["to"]=to?Json(checksum_address(*to)):Json(nullptr);out["contractAddress"]=contract_address?Json(checksum_address(*contract_address)):Json(nullptr);for(const auto& log:logs)out["logs"].push_back(log.to_json());return out;
}
Receipt Receipt::from_json(const Json& j){
 Receipt r;r.type=unsigned(u64(quantity(j.value("type",Json("0x0")))));if(r.type>2)throw std::invalid_argument("unsupported receipt type");Int status=quantity(j.at("status"));if(status>1)throw std::invalid_argument("invalid receipt status");r.success=status==1;
 r.gas_used=quantity(j.at("gasUsed"));r.cumulative_gas_used=quantity(j.at("cumulativeGasUsed"));if(r.cumulative_gas_used<r.gas_used)throw std::invalid_argument("invalid cumulative gas");r.block_number=quantity(j.at("blockNumber"));r.index=quantity(j.at("transactionIndex"));r.transaction_hash=unhex(j.at("transactionHash"));r.block_hash=unhex(j.at("blockHash"));if(r.transaction_hash.size()!=32||r.block_hash.size()!=32)throw std::invalid_argument("invalid receipt hash width");r.sender=address(j.at("from"));if(!j.at("to").is_null())r.to=address(j.at("to"));if(!j.at("contractAddress").is_null())r.contract_address=address(j.at("contractAddress"));
 for(const auto& log:j.at("logs")){Log l{address(log.at("address")),{},unhex(log.at("data"))};for(const auto& t:log.at("topics")){auto topic=unhex(t);if(topic.size()!=32)throw std::invalid_argument("invalid topic width");l.topics.push_back(topic);}r.logs.push_back(l);}if(j.contains("logsBloom")&&unhex(j["logsBloom"])!=logs_bloom(r.logs))throw std::invalid_argument("receipt bloom mismatch");return r;
}
Bytes receipts_root(const std::vector<Receipt>& receipts){std::map<Bytes,Bytes> entries;for(size_t i=0;i<receipts.size();++i)entries[Rlp(Int(i)).encode()]=receipts[i].encode();return trie_root(entries);}
}
