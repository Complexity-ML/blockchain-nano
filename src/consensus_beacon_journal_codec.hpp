#pragma once
#include "nano/consensus_beacon_journal_auth.hpp"
#include <set>
#include <algorithm>
#include <stdexcept>
#include <vector>
namespace nano::beacon_journal_detail {
inline constexpr size_t journal_bytes=4096,witness_bytes=16384;
inline void require(bool ok){if(!ok)throw std::invalid_argument("Invalid beacon signing journal");}
inline std::string number(uint64_t n){return quantity(Int(std::to_string(n)));}
inline uint64_t parse_number(const Json& j){const auto n=u64(quantity(j));require(j==number(n));return n;}
inline std::string root_text(const ConsensusRoot& root){return hex(Bytes(root.begin(),root.end()));}
inline ConsensusRoot parse_root(const Json& j){require(j.is_string());const auto text=j.get<std::string>();const auto b=unhex(text);require(text.size()==66&&b.size()==32&&hex(b)==text);ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
inline Json parse(std::string_view bytes,size_t maximum){
 require(!bytes.empty()&&bytes.size()<=maximum);std::vector<std::set<std::string>> keys;
 return Json::parse(bytes,[&](int depth,Json::parse_event_t event,Json& v){
  require(depth<=8);if(event==Json::parse_event_t::object_start)keys.emplace_back();
  else if(event==Json::parse_event_t::object_end)keys.pop_back();
  else if(event==Json::parse_event_t::key)require(!keys.empty()&&keys.back().insert(v.get<std::string>()).second);return true;});
}
inline std::string encode(const BeaconJournalIdentity& id,const BeaconJournalReservations& r){
 const bool unknown=(r.proposal&&!r.proposal->root)||(r.attestation&&!r.attestation->root);
 auto j=Json{{"format",unknown?"nano-beacon-signing-v3":r.retired?"nano-beacon-signing-v2":"nano-beacon-signing-v1"},
  {"publicKey",hex(id.public_key)},{"genesisValidatorsRoot",root_text(id.genesis_validators_root)},
  {"slotsPerEpoch",number(id.slots_per_epoch)},
  {"proposal",r.proposal?Json{{"slot",number(r.proposal->slot)},{"root",r.proposal->root?Json(root_text(*r.proposal->root)):Json(nullptr)}}:Json(nullptr)},
  {"attestation",r.attestation?Json{{"source",number(r.attestation->source)},{"target",number(r.attestation->target)},{"root",r.attestation->root?Json(root_text(*r.attestation->root)):Json(nullptr)}}:Json(nullptr)}};
 if(r.retired||unknown)j["retired"]=r.retired;return j.dump();
}
inline BeaconJournalReservations decode(std::string_view bytes,const BeaconJournalIdentity& id){
 const auto j=parse(bytes,journal_bytes);require(j.is_object());const bool v3=j.at("format")=="nano-beacon-signing-v3";
 BeaconJournalReservations r;if(v3)require(j.at("retired").is_boolean());r.retired=v3?j.at("retired").get<bool>():j.at("format")=="nano-beacon-signing-v2";
 require((v3?j.size()==7:r.retired?(j.size()==7&&j.at("retired")==true):(j.size()==6&&j.at("format")=="nano-beacon-signing-v1"))&&j.at("publicKey")==hex(id.public_key)&&parse_root(j.at("genesisValidatorsRoot"))==id.genesis_validators_root&&parse_number(j.at("slotsPerEpoch"))==id.slots_per_epoch);
 const auto& p=j.at("proposal");if(!p.is_null()){require(p.is_object()&&p.size()==2);r.proposal=BeaconJournalProposal{parse_number(p.at("slot")),(v3&&p.at("root").is_null())?std::nullopt:std::optional<ConsensusRoot>(parse_root(p.at("root")))};}
 const auto& v=j.at("attestation");if(!v.is_null()){require(v.is_object()&&v.size()==3);r.attestation=BeaconJournalAttestation{parse_number(v.at("source")),parse_number(v.at("target")),(v3&&v.at("root").is_null())?std::nullopt:std::optional<ConsensusRoot>(parse_root(v.at("root")))};require(r.attestation->source<=r.attestation->target);}
 require(encode(id,r)==bytes);return r;
}
inline Json token_json(const ForkCheckpointToken& t){return {{"sequence",number(t.sequence)},{"digest",hex(t.digest)}};}
inline void token_shape(const ForkCheckpointToken& t){require(t.sequence&&t.digest.size()==32);}
inline std::optional<ForkCheckpointToken> token(const Json& j){
 if(j.is_null())return std::nullopt;require(j.is_object()&&j.size()==2);ForkCheckpointToken t{parse_number(j.at("sequence")),unhex(j.at("digest").get<std::string>())};token_shape(t);require(token_json(t)==j);return t;
}
}
