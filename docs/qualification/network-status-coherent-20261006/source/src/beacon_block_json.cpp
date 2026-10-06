#include "beacon_block_json.hpp"
namespace nano {
namespace {
using View=std::span<const uint8_t>;
View cut(View b,size_t p,size_t n){if(p>b.size()||n>b.size()-p)throw std::invalid_argument("Beacon JSON bounds");return b.subspan(p,n);}
uint64_t num(View b,size_t p,size_t n=8){auto v=cut(b,p,n);uint64_t out=0;for(size_t i=0;i<n;++i)out|=uint64_t(v[i])<<(8*i);return out;}
std::string dec(View b,size_t p){return std::to_string(num(b,p));}
std::string raw(View b,size_t p,size_t n){auto v=cut(b,p,n);return hex(Bytes(v.begin(),v.end()));}
Json cp(View b,size_t p){return {{"epoch",dec(b,p)},{"root",raw(b,p+8,32)}};}
Json attdata(View b){return {{"slot",dec(b,0)},{"index",dec(b,8)},{"beacon_block_root",raw(b,16,32)},{"source",cp(b,48)},{"target",cp(b,88)}};}
Json header(View b){return {{"message",{{"slot",dec(b,0)},{"proposer_index",dec(b,8)},{"parent_root",raw(b,16,32)},{"state_root",raw(b,48,32)},{"body_root",raw(b,80,32)}}},{"signature",raw(b,112,96)}};}
Json attestation(View b,bool indexed){
 const auto at=num(b,0,4);auto tail=cut(b,at,b.size()-std::min<size_t>(at,b.size()));
 Json out={{"data",attdata(cut(b,4,128))},{"signature",raw(b,132,96)}};
 if(indexed){if(tail.size()%8)throw std::invalid_argument("Beacon indices width");Json indices=Json::array();for(size_t i=0;i<tail.size();i+=8)indices.push_back(dec(tail,i));out["attesting_indices"]=std::move(indices);}
 else out["aggregation_bits"]=raw(tail,0,tail.size());return out;
}
Json list(View b,size_t width,const std::function<Json(View)>& render,const std::function<void()>& check){
 Json out=Json::array();if(b.empty())return out;
 if(width){if(b.size()%width)throw std::invalid_argument("Beacon fixed list width");for(size_t i=0;i<b.size();i+=width){if(check)check();out.push_back(render(cut(b,i,width)));}}
 else{const auto first=num(b,0,4);if(first%4||first>b.size())throw std::invalid_argument("Beacon list offsets");for(size_t p=0;p<first;p+=4){if(check)check();const auto start=num(b,p,4),end=p+4<first?num(b,p+4,4):b.size();if(end<start)throw std::invalid_argument("Beacon offset order");out.push_back(render(cut(b,start,end-start)));}}
 return out;
}
}
Json beacon_block_json(const Bytes& wire,const BellatrixForkBlockRules& rules,const std::function<void()>& check){
 View all(wire);auto message=cut(all,100,all.size()-100),b=cut(message,84,message.size()-84);
 const auto sync_size=(rules.transition.consensus.state.sync_committee_size+7)/8;
 const std::array<size_t,6> positions{200,204,208,212,216,220+sync_size+96};
 auto section=[&](size_t i){const auto start=num(b,positions.at(i),4),end=i+1<positions.size()?num(b,positions.at(i+1),4):b.size();if(end<start)throw std::invalid_argument("Beacon body offset order");return cut(b,start,end-start);};
 Json body={{"randao_reveal",raw(b,0,96)},{"eth1_data",{{"deposit_root",raw(b,96,32)},{"deposit_count",dec(b,128)},{"block_hash",raw(b,136,32)}}},{"graffiti",raw(b,168,32)}};
 body["proposer_slashings"]=list(section(0),416,[](View v){return Json{{"signed_header_1",header(cut(v,0,208))},{"signed_header_2",header(cut(v,208,208))}};},check);
 body["attester_slashings"]=list(section(1),0,[](View v){const auto a=num(v,0,4),z=num(v,4,4);if(z<a||z>v.size())throw std::invalid_argument("Beacon slashing offsets");return Json{{"attestation_1",attestation(cut(v,a,z-a),true)},{"attestation_2",attestation(cut(v,z,v.size()-z),true)}};},check);
 body["attestations"]=list(section(2),0,[](View v){return attestation(v,false);},check);
 body["deposits"]=list(section(3),1240,[](View v){Json proof=Json::array();for(size_t i=0;i<33;++i)proof.push_back(raw(v,i*32,32));return Json{{"proof",std::move(proof)},{"data",{{"pubkey",raw(v,1056,48)},{"withdrawal_credentials",raw(v,1104,32)},{"amount",dec(v,1136)},{"signature",raw(v,1144,96)}}}};},check);
 body["voluntary_exits"]=list(section(4),112,[](View v){return Json{{"message",{{"epoch",dec(v,0)},{"validator_index",dec(v,8)}}},{"signature",raw(v,16,96)}};},check);
 body["sync_aggregate"]={{"sync_committee_bits",raw(b,220,sync_size)},{"sync_committee_signature",raw(b,220+sync_size,96)}};
 const auto payload=decode_bellatrix_execution_payload(section(5),rules.transition.execution,rules.transition.consensus.maximum_block_bytes,check);if(!payload)throw std::invalid_argument("Beacon execution payload unavailable");const auto& p=*payload;
 auto h=[](const auto& a){return hex(Bytes(a.begin(),a.end()));};Json txs=Json::array();for(const auto& tx:p.transactions){if(check)check();txs.push_back(hex(tx));}
 Bytes fee(p.base_fee_per_gas.rbegin(),p.base_fee_per_gas.rend());
 body["execution_payload"]={{"parent_hash",h(p.parent_hash)},{"fee_recipient",h(p.fee_recipient)},{"state_root",h(p.state_root)},{"receipts_root",h(p.receipts_root)},{"logs_bloom",h(p.logs_bloom)},{"prev_randao",h(p.prev_randao)},{"block_number",std::to_string(p.block_number)},{"gas_limit",std::to_string(p.gas_limit)},{"gas_used",std::to_string(p.gas_used)},{"timestamp",std::to_string(p.timestamp)},{"extra_data",hex(p.extra_data)},{"base_fee_per_gas",integer(fee).get_str()},{"block_hash",h(p.block_hash)},{"transactions",std::move(txs)}};
 return {{"message",{{"slot",dec(message,0)},{"proposer_index",dec(message,8)},{"parent_root",raw(message,16,32)},{"state_root",raw(message,48,32)},{"body",std::move(body)}}},{"signature",raw(all,4,96)}};
}
}
