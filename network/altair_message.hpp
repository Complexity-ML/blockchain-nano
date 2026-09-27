#pragma once
#include "json_frame.hpp"
#include "altair_base64.hpp"
#include "inbound_budget.hpp"
#include "json.hpp"
#include <optional>
#include <vector>
#include <functional>
#include <set>

// Nano-specific uncompressed JSON/hex envelope, NOT Ethereum gossipsub framing.
// Transport obtains peer identity from the authenticated libp2p stream, never
// from these fields. Fork/chain matching is a compatibility filter, not proof.
struct AltairWireProfile {
 std::string chain_id,genesis_hash,fork_digest;
 size_t frame_bytes,block_bytes,attestation_bytes,slashing_bytes;
 // Required full identity for Bellatrix; empty preserves the Altair v1 schema.
 std::string consensus_identity{};
 bool bls_changes=false;
 bool sync_messages=false;
};
enum class AltairWireKind {block,attestation,attester_slashing,voluntary_exit,bls_to_execution_change,sync_committee_message};
struct AltairWireMessage {AltairWireKind kind;std::vector<uint8_t> ssz;};
class AltairMessageIngress {
 AltairWireProfile profile_;
 InboundBudget& budget_;
 static int nibble(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
 static bool fixed_hex(std::string_view s,size_t bytes){return s.size()==2+2*bytes&&s.starts_with("0x")&&std::all_of(s.begin()+2,s.end(),[](char c){return nibble(c)>=0;});}
 static bool quantity(std::string_view s){return s.size()>=3&&s.size()<=66&&s.starts_with("0x")&&(s.size()==3||s[2]!='0')&&std::all_of(s.begin()+2,s.end(),[](char c){return nibble(c)>=0;});}
public:
 enum class Result {accepted,rejected,limited};
 using Handler=std::function<bool(const AltairWireMessage&)>;
 explicit AltairMessageIngress(AltairWireProfile profile,InboundBudget& shared_budget):profile_(std::move(profile)),budget_(shared_budget){
  if((!profile_.consensus_identity.empty()&&!fixed_hex(profile_.consensus_identity,32))||!quantity(profile_.chain_id)||!fixed_hex(profile_.genesis_hash,32)||!fixed_hex(profile_.fork_digest,4)||!profile_.frame_bytes||!profile_.block_bytes||!profile_.attestation_bytes||!profile_.slashing_bytes)throw std::invalid_argument("invalid Altair wire profile");
 }
 // Complete frame admission. Network callers must use the SAME frame limit
 // while reading fragments, before buffering the complete document. No Snappy
 // decompression, remote ticks or profile fallback is accepted by this protocol.
 // Shares the daemon budget; caller must keep it alive and not acquire twice.
 // Serial io_context use, as InboundBudget. Lease spans validation and handler;
 // handler must finish its durable commit before returning and not re-enter.
 size_t frame_limit()const{return profile_.frame_bytes;}
 std::shared_ptr<InboundBudget::Lease> acquire(const std::string& peer,InboundBudget::Clock::time_point now=InboundBudget::Clock::now()){
  return budget_.acquire(peer,now,InboundBudget::Traffic::bulk);
 }
 // Decode only. Async callers must already hold acquire() until processing ends.
 std::optional<AltairWireMessage> decode(std::string_view frame)const{
  nlohmann::json j;std::set<std::string> keys;bool duplicate=false;
  try{JsonFrame bounds(profile_.frame_bytes);if(!bounds.feed(frame))return std::nullopt;j=nlohmann::json::parse(frame,[&](int depth,nlohmann::json::parse_event_t event,nlohmann::json& value){if(depth==1&&event==nlohmann::json::parse_event_t::key&&!keys.insert(value.get<std::string>()).second)duplicate=true;return true;},false);}
  catch(const std::invalid_argument&){return std::nullopt;}
  if(duplicate||!j.is_object()||j.size()!=(profile_.consensus_identity.empty()?5:6)||!j.contains("chainId")||!j.contains("genesisHash")||!j.contains("forkDigest")||!j.contains("kind")||!j.contains("ssz"))return std::nullopt;
  if(!profile_.consensus_identity.empty()&&j.value("consensusIdentity",nlohmann::json())!=profile_.consensus_identity)return std::nullopt;
  if(j.at("chainId")!=profile_.chain_id||j.at("genesisHash")!=profile_.genesis_hash||j.at("forkDigest")!=profile_.fork_digest||!j.at("kind").is_string()||!j.at("ssz").is_string())return std::nullopt;
  AltairWireKind kind;size_t maximum;
  if(j.at("kind")=="block"){kind=AltairWireKind::block;maximum=profile_.block_bytes;}
  else if(j.at("kind")=="attestation"){kind=AltairWireKind::attestation;maximum=profile_.attestation_bytes;}
  else if(j.at("kind")=="attester_slashing"){kind=AltairWireKind::attester_slashing;maximum=profile_.slashing_bytes;}
  else if(j.at("kind")=="voluntary_exit"&&!profile_.consensus_identity.empty()){kind=AltairWireKind::voluntary_exit;maximum=112;}
  else if(j.at("kind")=="bls_to_execution_change"&&profile_.bls_changes&&!profile_.consensus_identity.empty()){kind=AltairWireKind::bls_to_execution_change;maximum=172;}
  else if(j.at("kind")=="sync_committee_message"&&profile_.sync_messages&&!profile_.consensus_identity.empty()){kind=AltairWireKind::sync_committee_message;maximum=144;}
  else return std::nullopt;
  const auto& encoded=j.at("ssz").get_ref<const std::string&>();
  if(encoded.size()<2||!encoded.starts_with("0x")||encoded.size()%2||(encoded.size()-2)/2>maximum)return std::nullopt;
  if((kind==AltairWireKind::voluntary_exit&&encoded.size()!=226)||(kind==AltairWireKind::bls_to_execution_change&&encoded.size()!=346)||(kind==AltairWireKind::sync_committee_message&&encoded.size()!=290))return std::nullopt;
  // Validate the entire alphabet before allocating binary payload memory.
  if(!std::all_of(encoded.begin()+2,encoded.end(),[](char c){return nibble(c)>=0;}))return std::nullopt;
  AltairWireMessage message{kind,{}};message.ssz.reserve((encoded.size()-2)/2);
  for(size_t i=2;i<encoded.size();i+=2)message.ssz.push_back(uint8_t((nibble(encoded[i])<<4)|nibble(encoded[i+1])));
  return message;
 }
 // Compact, authenticated local IPC; never used by the peer frame decoder.
 std::optional<AltairWireMessage> decode_compact(const nlohmann::json& j)const{
  if(!j.is_object()||(!profile_.consensus_identity.empty()&&j.value("consensusIdentity",nlohmann::json())!=profile_.consensus_identity))return std::nullopt;
  if(!j.is_object()||j.size()!=(profile_.consensus_identity.empty()?5:6)||!j.contains("chainId")||!j.contains("genesisHash")||!j.contains("forkDigest")||!j.contains("kind")||!j.contains("sszBase64")||!j.at("sszBase64").is_string()||j.at("chainId")!=profile_.chain_id||j.at("genesisHash")!=profile_.genesis_hash||j.at("forkDigest")!=profile_.fork_digest)return std::nullopt;
  AltairWireKind kind;size_t maximum;
  if(j.at("kind")=="block"){kind=AltairWireKind::block;maximum=profile_.block_bytes;}
  else if(j.at("kind")=="attestation"){kind=AltairWireKind::attestation;maximum=profile_.attestation_bytes;}
  else if(j.at("kind")=="attester_slashing"){kind=AltairWireKind::attester_slashing;maximum=profile_.slashing_bytes;}
  else if(j.at("kind")=="voluntary_exit"&&!profile_.consensus_identity.empty()){kind=AltairWireKind::voluntary_exit;maximum=112;}
  else if(j.at("kind")=="bls_to_execution_change"&&profile_.bls_changes&&!profile_.consensus_identity.empty()){kind=AltairWireKind::bls_to_execution_change;maximum=172;}
  else if(j.at("kind")=="sync_committee_message"&&profile_.sync_messages&&!profile_.consensus_identity.empty()){kind=AltairWireKind::sync_committee_message;maximum=144;}
  else return std::nullopt;
  auto envelope=nlohmann::json{{"chainId",profile_.chain_id},{"genesisHash",profile_.genesis_hash},{"forkDigest",profile_.fork_digest},{"kind",j.at("kind")},{"ssz","0x"}};
  if(!profile_.consensus_identity.empty())envelope["consensusIdentity"]=profile_.consensus_identity;
  const auto overhead=envelope.dump().size();
  if(overhead>profile_.frame_bytes)return std::nullopt;maximum=std::min(maximum,(profile_.frame_bytes-overhead)/2);
  auto bytes=altair_unbase64(j.at("sszBase64").get_ref<const std::string&>(),maximum);if(!bytes||(kind==AltairWireKind::voluntary_exit&&bytes->size()!=112)||(kind==AltairWireKind::bls_to_execution_change&&bytes->size()!=172)||(kind==AltairWireKind::sync_committee_message&&bytes->size()!=144))return std::nullopt;
  return AltairWireMessage{kind,std::move(*bytes)};
 }
 Result receive(const std::string& authenticated_peer,std::string_view frame,const Handler& handler,
  InboundBudget::Clock::time_point now=InboundBudget::Clock::now()){
  auto lease=acquire(authenticated_peer,now);if(!lease)return Result::limited;
  const auto message=decode(frame);if(!message)return Result::rejected;
  return handler(*message)?Result::accepted:Result::rejected;
 }
 size_t active()const{return budget_.active();}
 size_t tracked_peers()const{return budget_.tracked_peers();}
};
