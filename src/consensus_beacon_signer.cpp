#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_bls.hpp"
#include "consensus_beacon_journal_codec.hpp"
#include <type_traits>
#include <charconv>
#include <set>
namespace nano {
static_assert(std::is_nothrow_move_assignable_v<std::optional<ForkCheckpointToken>>);
namespace {
// Fixed schema: two reservations and identities, independent of chain age.
constexpr size_t journal_bytes=4096;
constexpr size_t witness_bytes=16384;
Json token_json(const std::optional<ForkCheckpointToken>& t){return t?Json{{"sequence",quantity(Int(std::to_string(t->sequence)))},{"digest",hex(t->digest)}}:Json(nullptr);}
std::optional<ForkCheckpointToken> witness_token(const Json& j){
 if(j.is_null())return std::nullopt;
 if(!j.is_object()||j.size()!=2)throw std::invalid_argument("Invalid signing witness token");
 const auto n=u64(quantity(j.at("sequence")));const auto b=unhex(j.at("digest").get<std::string>());
 const ForkCheckpointToken t{n,b};
 if(!n||b.size()!=32||token_json(t)!=j)throw std::invalid_argument("Noncanonical signing witness token");return t;
}
void require(bool ok){if(!ok)throw std::invalid_argument("Invalid beacon signing journal");}
std::string root_text(const ConsensusRoot& root){return hex(Bytes(root.begin(),root.end()));}
ConsensusRoot parse_root(const Json& value){require(value.is_string());const auto encoded=value.get<std::string>();require(encoded.size()==66);const auto bytes=unhex(encoded);require(bytes.size()==32&&hex(bytes)==encoded);ConsensusRoot out{};std::copy(bytes.begin(),bytes.end(),out.begin());return out;}
std::string number(uint64_t n){return quantity(Int(std::to_string(n)));}
uint64_t parse_number(const Json& value){const auto n=u64(quantity(value));require(value==number(n));return n;}
}
std::string BeaconSigningJournal::encode(const std::optional<Proposal>& proposal,const std::optional<Vote>& vote,bool retired)const{
 BeaconJournalReservations r;r.retired=retired;
 if(proposal)r.proposal=BeaconJournalProposal{proposal->slot,proposal->root};
 if(vote)r.attestation=BeaconJournalAttestation{vote->source,vote->target,vote->root};
 return beacon_journal_detail::encode({public_key_,validators_genesis_,slots_per_epoch_},r);
}
void BeaconSigningJournal::commit(const std::optional<Proposal>& proposal,const std::optional<Vote>& vote,bool retired){
 if(unavailable_)throw std::runtime_error("Beacon signer unavailable; reopen required");
 const auto encoded=encode(proposal,vote,retired);ForkCheckpointToken next;
 try{
  if(recovery_){
   if(token_&&token_->sequence==UINT64_MAX)throw std::overflow_error("Signing witness sequence exhausted");
   const ForkCheckpointToken candidate{token_?token_->sequence+1:1,sha256(Bytes(encoded.begin(),encoded.end()))};
   const auto intent=Json{{"format","nano-beacon-signing-witness-v1"},{"previous",token_json(token_)},{"next",token_json(candidate)},{"snapshot",encoded}}.dump();
   recovery_token_=recovery_->commit_fork_checkpoint(chain_,execution_genesis_,recovery_token_,intent,witness_bytes);
  }
  next=disk_.commit_fork_checkpoint(chain_,execution_genesis_,token_,encoded,journal_bytes);
 }catch(...){unavailable_=true;throw;}
 // All publication after the synchronous write is non-throwing.
 static_assert(std::is_nothrow_copy_assignable_v<std::optional<Proposal>>);
 static_assert(std::is_nothrow_copy_assignable_v<std::optional<Vote>>);
 token_=std::move(next);proposal_=proposal;vote_=vote;retired_=retired;
}
void BeaconSigningJournal::ready()const{
 if(unavailable_||!token_)throw std::runtime_error("Beacon signer unavailable; reopen required");
 // Also fence another instance using this same DB, including exact-repeat calls.
 disk_.fork_checkpoint(chain_,execution_genesis_,journal_bytes,token_);
 if(recovery_)recovery_->fork_checkpoint(chain_,execution_genesis_,witness_bytes,recovery_token_);
}
BeaconSigningJournal::BeaconSigningJournal(ChainStore& disk,Int chain,Bytes execution_genesis,
 ConsensusRoot validators_genesis,Bytes public_key,uint64_t slots,const std::optional<ForkCheckpointToken>& expected,ChainStore* recovery):
 disk_(disk),chain_(std::move(chain)),execution_genesis_(std::move(execution_genesis)),public_key_(std::move(public_key)),validators_genesis_(validators_genesis),slots_per_epoch_(slots),mutex_(disk.beacon_signing_mutex_),recovery_(recovery){
 std::lock_guard lock(mutex_);
 require(chain_>0&&execution_genesis_.size()==32&&slots_per_epoch_&&aggregate_consensus_bls_keys(std::vector<Bytes>{public_key_}).has_value());
 disk_.bind_chain(chain_,execution_genesis_,"nano-beacon-signing-v1");
 auto effective=expected;
 if(recovery_){
  require(recovery_!=&disk_);
  recovery_->bind_chain(chain_,execution_genesis_,"nano-beacon-signing-witness-v1");
  const auto witness=recovery_->fork_checkpoint(chain_,execution_genesis_,witness_bytes);
  const auto primary=disk_.fork_checkpoint(chain_,execution_genesis_,journal_bytes);
  if(!witness){require(!primary&&!expected);}
  else{
   const auto intent=Json::parse(witness->snapshot);
   require(intent.is_object()&&intent.size()==4&&intent.at("format")=="nano-beacon-signing-witness-v1"&&intent.dump()==witness->snapshot);
   const auto previous=witness_token(intent.at("previous")),next=witness_token(intent.at("next"));
   const auto snapshot=intent.at("snapshot").get<std::string>();
   require(next&&(!previous||previous->sequence<UINT64_MAX)&&next->sequence==(previous?previous->sequence+1:1)&&!snapshot.empty()&&snapshot.size()<=journal_bytes&&sha256(Bytes(snapshot.begin(),snapshot.end()))==next->digest);
   // Bind the trusted intent to this key before modifying the primary store.
   const auto identity=Json::parse(snapshot);
   require(identity.at("publicKey")==hex(public_key_)&&parse_root(identity.at("genesisValidatorsRoot"))==validators_genesis_&&parse_number(identity.at("slotsPerEpoch"))==slots_per_epoch_);
   require(!expected||*expected==*next);
   const auto actual=primary?std::optional<ForkCheckpointToken>(primary->token):std::nullopt;
   require(actual==next||actual==previous);
   if(actual!=next){const auto recovered=disk_.commit_fork_checkpoint(chain_,execution_genesis_,previous,snapshot,journal_bytes);require(recovered==*next);}
   recovery_token_=witness->token;effective=next;
  }
 }
 const auto saved=disk_.fork_checkpoint(chain_,execution_genesis_,journal_bytes,effective);
 if(!saved){require(!effective);commit(std::nullopt,std::nullopt);return;}
 require(effective.has_value());const auto decoded=beacon_journal_detail::decode(saved->snapshot,{public_key_,validators_genesis_,slots_per_epoch_});
 if(decoded.proposal)proposal_=Proposal{decoded.proposal->slot,decoded.proposal->root};
 if(decoded.attestation)vote_=Vote{decoded.attestation->source,decoded.attestation->target,decoded.attestation->root};
 retired_=decoded.retired;token_=saved->token;
}
ForkCheckpointToken BeaconSigningJournal::commitment()const{std::lock_guard lock(mutex_);ready();return *token_;}
bool BeaconSigningJournal::is_retired()const{std::lock_guard lock(mutex_);ready();return retired_;}
std::string BeaconSigningJournal::retire_and_export_interchange(){
 std::lock_guard lock(mutex_);ready();
 Json blocks=Json::array(),attestations=Json::array();
 if(proposal_){Json block{{"slot",std::to_string(proposal_->slot)}};if(proposal_->root)block["signing_root"]=root_text(*proposal_->root);blocks.push_back(std::move(block));}
 if(vote_){Json vote{{"source_epoch",std::to_string(vote_->source)},{"target_epoch",std::to_string(vote_->target)}};if(vote_->root)vote["signing_root"]=root_text(*vote_->root);attestations.push_back(std::move(vote));}
 // Allocate the export before retiring; no export escapes until the durable CAS.
 auto output=Json{{"metadata",{{"interchange_format_version","5"},{"genesis_validators_root",root_text(validators_genesis_)}}},
  {"data",Json::array({Json{{"pubkey",hex(public_key_)},{"signed_blocks",blocks},{"signed_attestations",attestations}}})}}.dump();
 if(!retired_)commit(proposal_,vote_,true);
 return output;
}
size_t BeaconSigningJournal::import_interchange(std::string_view input,size_t max_bytes){
 require(max_bytes&&input.size()<=max_bytes);
 // Depth bound applies even to ignored extensions, before constructing deep DOMs.
 std::vector<std::set<std::string>> keys;
 const auto doc=Json::parse(input.begin(),input.end(),[&](int depth,Json::parse_event_t event,Json& value){
  require(depth<=16);
  if(event==Json::parse_event_t::object_start)keys.emplace_back();
  else if(event==Json::parse_event_t::object_end)keys.pop_back();
  else if(event==Json::parse_event_t::key)require(keys.back().insert(value.get<std::string>()).second);
  return true;
 });
 auto bytes=[](const Json& value,size_t n){require(value.is_string());const auto text=value.get<std::string>();require(text.size()==2+2*n&&text.starts_with("0x"));auto out=unhex(text);require(out.size()==n);return out;};
 auto root=[&](const Json& value){const auto b=bytes(value,32);ConsensusRoot out{};std::copy(b.begin(),b.end(),out.begin());return out;};
 auto decimal=[](const Json& value){require(value.is_string());const auto s=value.get<std::string>();require(!s.empty()&&std::all_of(s.begin(),s.end(),[](char c){return c>='0'&&c<='9';}));uint64_t n=0;const auto result=std::from_chars(s.data(),s.data()+s.size(),n);require(result.ec==std::errc{}&&result.ptr==s.data()+s.size());return n;};
 auto signing_root=[&](const Json& value)->std::optional<ConsensusRoot>{if(!value.contains("signing_root"))return std::nullopt;return root(value.at("signing_root"));};
 require(doc.is_object());const auto& metadata=doc.at("metadata");require(metadata.is_object()&&metadata.at("interchange_format_version")=="5"&&root(metadata.at("genesis_validators_root"))==validators_genesis_);
 const auto& entries=doc.at("data");require(entries.is_array());
 std::lock_guard lock(mutex_);ready();require(!retired_);
 auto proposal=proposal_;auto vote=vote_;size_t matches=0;
 for(const auto& entry:entries){
  require(entry.is_object());const bool match=bytes(entry.at("pubkey"),48)==public_key_;
  const auto& blocks=entry.at("signed_blocks");const auto& votes=entry.at("signed_attestations");require(blocks.is_array()&&votes.is_array());if(match)++matches;
  for(const auto& b:blocks){
   require(b.is_object());Proposal incoming{decimal(b.at("slot")),signing_root(b)};
   if(!match)continue;
   if(!proposal||incoming.slot>proposal->slot)proposal=incoming;
   else if(incoming.slot==proposal->slot&&incoming.root!=proposal->root)proposal->root.reset();
  }
  for(const auto& v:votes){
   require(v.is_object());Vote incoming{decimal(v.at("source_epoch")),decimal(v.at("target_epoch")),signing_root(v)};require(incoming.source<=incoming.target);
   if(!match)continue;
   if(!vote){vote=incoming;continue;}
   // Max source and max target may come from different messages. Such a
   // synthetic watermark has NO signing root and can never authorize repeats.
   if(incoming.target>vote->target){if(incoming.source<vote->source){incoming.source=vote->source;incoming.root.reset();}vote=incoming;}
   else if(incoming.target==vote->target){if(incoming.source!=vote->source||incoming.root!=vote->root)vote->root.reset();vote->source=std::max(vote->source,incoming.source);}
   else if(incoming.source>vote->source){vote->source=incoming.source;vote->root.reset();}
  }
 }
 if(proposal!=proposal_||vote!=vote_)commit(proposal,vote);
 return matches;
}
Bytes BeaconSigningJournal::sign(const ConsensusRoot& root,const Signer& provider)const{
 const auto signature=provider(root);if(!verify_consensus_bls(public_key_,root,signature))throw std::runtime_error("Beacon signing provider returned an invalid signature");return signature;
}
void BeaconSigningJournal::reserve_proposal(uint64_t slot,const ConsensusRoot& root){
 ready();if(retired_)throw std::runtime_error("Beacon signing journal permanently retired");
 if(proposal_){if(slot<proposal_->slot||(slot==proposal_->slot&&root!=proposal_->root))throw std::runtime_error("Conflicting or old beacon proposal refused");if(slot==proposal_->slot)return;}
 commit(Proposal{slot,root},vote_);
}
void BeaconSigningJournal::reserve_attestation(uint64_t source,uint64_t target,const ConsensusRoot& root){
 require(source<=target);ready();if(retired_)throw std::runtime_error("Beacon signing journal permanently retired");
 if(vote_){
  if(source<vote_->source||target<vote_->target||(target==vote_->target&&(source!=vote_->source||root!=vote_->root)))throw std::runtime_error("Conflicting or old beacon attestation refused");
  if(target==vote_->target)return;
 }
 commit(proposal_,Vote{source,target,root});
}
Bytes BeaconSigningJournal::sign_proposal(const ConsensusBeaconHeader& header,const ConsensusBytes4& version,const Signer& provider){
 if(!provider)throw std::invalid_argument("Missing beacon signing provider");
 const auto root=consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},version,validators_genesis_));
 std::lock_guard lock(mutex_);reserve_proposal(header.slot,root);return sign(root,provider);
}
Bytes BeaconSigningJournal::sign_attestation(const ConsensusAttestationData& data,const ConsensusBytes4& version,const Signer& provider){
 if(!provider||data.source.epoch>data.target.epoch||data.target.epoch!=data.slot/slots_per_epoch_)throw std::invalid_argument("Invalid beacon attestation signing request");
 const auto root=consensus_signing_root(consensus_attestation_root(data),consensus_domain({1,0,0,0},version,validators_genesis_));
 std::lock_guard lock(mutex_);reserve_attestation(data.source.epoch,data.target.epoch,root);return sign(root,provider);
}
}

namespace nano {
std::optional<uint64_t> BeaconSigningJournal::latest_proposal_slot()const{std::lock_guard lock(mutex_);ready();return proposal_?std::optional<uint64_t>(proposal_->slot):std::nullopt;}
std::optional<uint64_t> BeaconSigningJournal::latest_attestation_target()const{std::lock_guard lock(mutex_);ready();return vote_?std::optional<uint64_t>(vote_->target):std::nullopt;}
}
