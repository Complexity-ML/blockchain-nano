#include "nano/core.hpp"
namespace nano {
Bytes Header::encode()const{if(parent_hash.size()!=32||uncle_hash.size()!=32||state_root.size()!=32||transactions_root.size()!=32||receipts_root.size()!=32||logs_bloom.size()!=256||mix_hash.size()!=32||nonce.size()!=8||extra_data.size()>32||(withdrawals_root&&withdrawals_root->size()!=32))throw std::invalid_argument("invalid header field width");std::vector<Rlp> fields{Rlp(parent_hash),Rlp(uncle_hash),Rlp(unhex(address(coinbase))),Rlp(state_root),Rlp(transactions_root),Rlp(receipts_root),Rlp(logs_bloom),Rlp(difficulty),Rlp(number),Rlp(gas_limit),Rlp(gas_used),Rlp(timestamp),Rlp(extra_data),Rlp(mix_hash),Rlp(nonce)};if(base_fee)fields.emplace_back(*base_fee);if(withdrawals_root){if(!base_fee)throw std::invalid_argument("withdrawals without base fee");fields.emplace_back(*withdrawals_root);}if(blob_gas_used||excess_blob_gas){if(!withdrawals_root||!blob_gas_used||!excess_blob_gas||*blob_gas_used<0||*excess_blob_gas<0||*blob_gas_used>Int("18446744073709551615")||*excess_blob_gas>Int("18446744073709551615"))throw std::invalid_argument("invalid blob header fields");fields.emplace_back(*blob_gas_used);fields.emplace_back(*excess_blob_gas);}if(parent_beacon_block_root){if(!blob_gas_used||!excess_blob_gas||parent_beacon_block_root->size()!=32)throw std::invalid_argument("invalid parent beacon block root");fields.emplace_back(*parent_beacon_block_root);}if(requests_hash){if(!parent_beacon_block_root||requests_hash->size()!=32)throw std::invalid_argument("invalid requests hash");fields.emplace_back(*requests_hash);}return Rlp(fields).encode();}
Json Header::to_json()const{Json j={{"parentHash",hex(parent_hash)},{"sha3Uncles",hex(uncle_hash)},{"miner",coinbase},{"stateRoot",hex(state_root)},{"transactionsRoot",hex(transactions_root)},{"receiptsRoot",hex(receipts_root)},{"logsBloom",hex(logs_bloom)},{"difficulty",quantity(difficulty)},{"number",quantity(number)},{"gasLimit",quantity(gas_limit)},{"gasUsed",quantity(gas_used)},{"timestamp",quantity(timestamp)},{"extraData",hex(extra_data)},{"mixHash",hex(mix_hash)},{"nonce",hex(nonce)},{"hash",hex(hash())}};if(base_fee)j["baseFeePerGas"]=quantity(*base_fee);if(withdrawals_root)j["withdrawalsRoot"]=hex(*withdrawals_root);if(blob_gas_used)j["blobGasUsed"]=quantity(*blob_gas_used);if(excess_blob_gas)j["excessBlobGas"]=quantity(*excess_blob_gas);if(parent_beacon_block_root)j["parentBeaconBlockRoot"]=hex(*parent_beacon_block_root);if(requests_hash)j["requestsHash"]=hex(*requests_hash);return j;}
Header Header::from_json(const Json& j){Header h;h.parent_hash=unhex(j.at("parentHash"));h.uncle_hash=unhex(j.at("sha3Uncles"));h.coinbase=address(j.at("miner"));h.state_root=unhex(j.at("stateRoot"));h.transactions_root=unhex(j.at("transactionsRoot"));h.receipts_root=unhex(j.at("receiptsRoot"));h.logs_bloom=unhex(j.at("logsBloom"));h.difficulty=quantity(j.at("difficulty"));h.number=quantity(j.at("number"));h.gas_limit=quantity(j.at("gasLimit"));h.gas_used=quantity(j.at("gasUsed"));h.timestamp=quantity(j.at("timestamp"));h.extra_data=unhex(j.at("extraData"));h.mix_hash=unhex(j.at("mixHash"));h.nonce=unhex(j.at("nonce"));h.base_fee=j.contains("baseFeePerGas")?std::optional<Int>(quantity(j["baseFeePerGas"])):std::nullopt;if(j.contains("withdrawalsRoot"))h.withdrawals_root=unhex(j["withdrawalsRoot"]);if(j.contains("blobGasUsed"))h.blob_gas_used=quantity(j["blobGasUsed"]);if(j.contains("excessBlobGas"))h.excess_blob_gas=quantity(j["excessBlobGas"]);if(j.contains("parentBeaconBlockRoot"))h.parent_beacon_block_root=unhex(j["parentBeaconBlockRoot"]);if(j.contains("requestsHash"))h.requests_hash=unhex(j["requestsHash"]);auto hash=h.hash();if(j.contains("hash")&&hash!=unhex(j.at("hash")))throw std::invalid_argument("header hash mismatch");return h;}
Json Block::to_json(bool full)const{auto j=header.to_json();if(!producer_signature.empty())j["producerProof"]={{"version",1},{"signature",hex(producer_signature)}};j["transactions"]=Json::array();size_t size=500;for(const auto& tx:transactions){j["transactions"].push_back(full?tx.to_json():Json(hex(tx.hash())));size+=tx.raw().size();}j["uncles"]=Json::array();for(const auto& u:uncles)j["uncles"].push_back(hex(u.hash()));if(!withdrawals.empty())j["withdrawals"]=withdrawals;j["size"]=quantity(Int(size));return j;}
Json Block::to_transport_json()const{
 auto j=to_json(false);j["transactionEncoding"]="signed-rlp-v1";j["transactions"]=Json::array();
 for(const auto& tx:transactions)j["transactions"].push_back(hex(tx.raw()));
 return j;
}
Block Block::from_json(const Json& j){Block b;b.header=Header::from_json(j);if(j.contains("producerProof")){const auto& proof=j.at("producerProof");if(!proof.is_object()||proof.size()!=2||!proof.contains("version")||!proof.at("version").is_number_integer()||proof.at("version")!=1)throw std::invalid_argument("unsupported producer proof version");b.producer_signature=unhex(proof.at("signature").get<std::string>());if(b.producer_signature.size()!=65||b.producer_signature[64]>1)throw std::invalid_argument("invalid producer proof encoding");}bool compact=j.contains("transactionEncoding");if(compact&&j.at("transactionEncoding")!="signed-rlp-v1")throw std::invalid_argument("unsupported block transaction encoding");for(const auto& t:j.at("transactions")){
 SignedTransaction tx;
 if(compact){if(!t.is_string())throw std::invalid_argument("encoded block requires signed transaction bytes");auto raw=unhex(t.get_ref<const std::string&>());tx=SignedTransaction::decode(raw);if(tx.raw()!=raw)throw std::invalid_argument("noncanonical signed transaction encoding");}
 else{if(!t.is_object())throw std::invalid_argument("full transactions required to import block");tx={Transaction::from_json(t),quantity(t.at("v")),quantity(t.at("r")),quantity(t.at("s"))};}
 tx.sender();b.transactions.push_back(std::move(tx));}if(j.contains("withdrawals"))b.withdrawals=j["withdrawals"];if(j.contains("uncles")&&!j["uncles"].empty()){if(!j.contains("uncleHeaders")||j["uncleHeaders"].size()!=j["uncles"].size())throw std::invalid_argument("full uncle headers required");for(size_t i=0;i<j["uncleHeaders"].size();++i){auto uncle=Header::from_json(j["uncleHeaders"][i]);if(hex(uncle.hash())!=j["uncles"][i].get<std::string>())throw std::invalid_argument("uncle hash mismatch");b.uncles.push_back(uncle);}}return b;}
Bytes transactions_root(const std::vector<SignedTransaction>& txs){std::map<Bytes,Bytes> entries;for(size_t i=0;i<txs.size();++i)entries[Rlp(Int(i)).encode()]=txs[i].raw();return trie_root(entries);}
Int next_base_fee(const Header& h){Int fee=h.base_fee.value_or(Int(1000000000));Int target=h.gas_limit/2;if(target==0)throw std::invalid_argument("invalid gas target");if(h.gas_used==target)return fee;if(h.gas_used>target)return Int(fee+std::max(Int(1),Int(fee*(h.gas_used-target)/target/8)));return Int(fee-fee*(target-h.gas_used)/target/8);}
void validate_execution_header(const Header& h,const Header& parent,const Int& minimum_gas_limit){
 if(h.gas_limit<minimum_gas_limit||abs(h.gas_limit-parent.gas_limit)>=parent.gas_limit/1024||h.gas_used<0||h.gas_used>h.gas_limit)throw std::invalid_argument("invalid block gas limits");
 if(parent.base_fee){if(h.base_fee!=std::optional<Int>(next_base_fee(parent)))throw std::invalid_argument("invalid base fee");}
 else if(h.base_fee)throw std::invalid_argument("unexpected base fee before activation");
}

Rlp withdrawal_payload(const Json& withdrawal){
 if(!withdrawal.is_object())throw std::invalid_argument("invalid withdrawal object");
 auto number=[&](const char* name){Int n=quantity(withdrawal.at(name));if(n<0||n>=(Int(1)<<64))throw std::invalid_argument("withdrawal integer exceeds uint64");return n;};
 auto recipient=unhex(withdrawal.at("address"));if(recipient.size()!=20)throw std::invalid_argument("invalid withdrawal address width");
 return Rlp(std::vector<Rlp>{Rlp(number("index")),Rlp(number("validatorIndex")),Rlp(recipient),Rlp(number("amount"))});
}
Bytes withdrawals_root(const Json& withdrawals){
 if(!withdrawals.is_array())throw std::invalid_argument("withdrawals must be an array");
 std::map<Bytes,Bytes> entries;for(size_t i=0;i<withdrawals.size();++i)entries[Rlp(Int(i)).encode()]=withdrawal_payload(withdrawals[i]).encode();return trie_root(entries);
}

// Canonical Ethereum execution body: typed transactions are RLP byte strings,
// legacy transactions are already encoded lists. Nano's producer proof is separate.
size_t execution_rlp_size(const Block& block){
 auto add=[](size_t a,size_t b){if(b>SIZE_MAX-a)throw std::overflow_error("execution RLP size overflow");return a+b;};
 auto list_size=[&](size_t payload){size_t prefix=1;if(payload>55){for(auto n=payload;n;n>>=8)++prefix;}return add(payload,prefix);};
 size_t txs=0,uncles=0,withdrawals=0;
 for(const auto& tx:block.transactions){auto raw=tx.raw();size_t size=raw.size();if(tx.tx.type!=0)size=(raw.size()==1&&raw[0]<0x80)?1:list_size(raw.size());txs=add(txs,size);}
 for(const auto& uncle:block.uncles)uncles=add(uncles,uncle.encode().size());
 size_t body=add(block.header.encode().size(),add(list_size(txs),list_size(uncles)));
 if(block.header.withdrawals_root){for(const auto& w:block.withdrawals)withdrawals=add(withdrawals,withdrawal_payload(w).encode().size());body=add(body,list_size(withdrawals));}
 return list_size(body);
}

}
