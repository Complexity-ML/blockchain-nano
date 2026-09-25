#include "nano/consensus_altair_reactor.hpp"
#include <type_traits>
#include "nano/consensus_message_ssz.hpp"
namespace nano {
namespace {
void archive_require(bool ok){if(!ok)throw std::invalid_argument("Invalid Altair durable archive");}
uint64_t archive_number(std::span<const uint8_t> b,size_t at,size_t width=8){uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
template<size_t N>std::array<uint8_t,N> archive_array(std::span<const uint8_t> b,size_t at){std::array<uint8_t,N> r{};std::copy_n(b.begin()+at,N,r.begin());return r;}
void verify_archived_block(const ConsensusRoot& id,const Bytes& bytes,const AltairForkStore& store,const AltairForkBlockRules& rules,const std::function<void()>& cancel){
 archive_require(id!=store.timed.view.anchor&&store.blocks.contains(id)&&bytes.size()>=404&&bytes.size()<=rules.transition.maximum_block_bytes);
 archive_require(archive_number(bytes,0,4)==100&&archive_number(bytes,180,4)==84);
 const ConsensusBeaconHeader header{archive_number(bytes,100),archive_number(bytes,108),archive_array<32>(bytes,116),archive_array<32>(bytes,148),altair_beacon_body_root(std::span(bytes).subspan(184),rules.transition.body,rules.transition.state.sync_committee_size)};
 const auto& block=store.blocks.at(id);archive_require(header==block.header&&consensus_beacon_header_root(header)==id);
 const auto context=altair_fork_block_context(block.state,header,rules.transition.epoch,rules.transition.state,cancel);
 const auto proposer=context.registry.validators.find(header.proposer_index);archive_require(proposer!=context.registry.validators.end());
 const auto& state=block.state.ssz;const auto version=archive_array<4>(state,context.registry.epoch<archive_number(state,56)?48:52);
 archive_require(verify_consensus_beacon_header(header,version,archive_array<32>(state,8),proposer->second.public_key,std::span(bytes).subspan(4,96)));
 if(cancel)cancel();
}
std::string durable_snapshot(const AltairForkStore& store,const AltairSignedBlockArchive& archive,const AltairSignedVoteArchive& votes,const AltairForkBlockRules& rules,const AltairStoreSnapshotLimits& limits,const std::function<void()>& cancel){
 // Retain byte-for-byte v1 encoding while both signed archives are empty.
 auto encoded=altair_store_snapshot(store,rules,limits,cancel);if(archive.empty()&&votes.empty())return encoded;
 archive_require(archive.size()<store.blocks.size());size_t total=0;Json blocks=Json::array();
 for(const auto& [id,bytes]:archive){
  if(cancel)cancel();archive_require(bytes&&id!=store.timed.view.anchor&&store.blocks.contains(id)&&bytes->size()<=rules.transition.maximum_block_bytes&&bytes->size()<=limits.envelope_bytes/2-total);total+=bytes->size();
  blocks.push_back(Json::array({hex(Bytes(id.begin(),id.end())),hex(*bytes)}));
 }
 Json document{{"format",votes.empty()?"nano-altair-durable-v2":"nano-altair-durable-v3"},{"store",Json::parse(encoded)},{"signedBlocks",std::move(blocks)}};
 if(!votes.empty()){
  archive_require(votes.size()<=limits.messages);Json messages=Json::array();
  const auto epoch=store.timed.view.current_epoch,oldest=epoch?epoch-1:0;
  for(const auto& [id,bytes]:votes){
   if(cancel)cancel();archive_require(bytes&&bytes->size()>=229&&bytes->size()<=228+uint64_t(rules.transition.body.validators_per_committee)/8+1&&bytes->size()<=limits.envelope_bytes/2-total);
   const auto target=archive_number(*bytes,92);archive_require(target>=oldest&&target<=epoch);total+=bytes->size();
   messages.push_back(Json::array({hex(Bytes(id.begin(),id.end())),hex(*bytes)}));
  }
  document["signedVotes"]=std::move(messages);
 }
 auto result=document.dump();archive_require(result.size()<=limits.envelope_bytes);if(cancel)cancel();return result;
}
struct Restored {AltairForkStore store;std::shared_ptr<const AltairSignedBlockArchive> blocks;std::shared_ptr<const AltairSignedVoteArchive> votes;};
Restored restore_durable_snapshot(std::string_view bytes,const Bytes& digest,const AltairForkBlockRules& rules,const AltairStoreSnapshotLimits& limits,const std::function<void()>& cancel){
 archive_require(bytes.size()<=limits.envelope_bytes&&digest.size()==32);if(cancel)cancel();archive_require(sha256(Bytes(bytes.begin(),bytes.end()))==digest);
 const auto document=Json::parse(bytes);
 if(document.value("format",Json())=="nano-altair-store-v1")return {altair_store_from_snapshot(bytes,digest,rules,limits,cancel),std::make_shared<const AltairSignedBlockArchive>(),std::make_shared<const AltairSignedVoteArchive>()};
 const bool with_votes=document.value("format",Json())=="nano-altair-durable-v3";
 archive_require(document.is_object()&&document.size()==(with_votes?4:3)&&(with_votes||document.at("format")=="nano-altair-durable-v2")&&document.at("signedBlocks").is_array()&&(with_votes||!document.at("signedBlocks").empty())&&document.at("signedBlocks").size()<=limits.blocks);
 const auto serialized=document.at("store").dump();auto store=altair_store_from_snapshot(serialized,sha256(Bytes(serialized.begin(),serialized.end())),rules,limits,cancel);
 auto archive=std::make_shared<AltairSignedBlockArchive>();size_t total=0;
 for(const auto& entry:document.at("signedBlocks")){
  if(cancel)cancel();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
  const auto& root=entry[0].get_ref<const std::string&>();const auto& payload=entry[1].get_ref<const std::string&>();
  archive_require(root.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=rules.transition.maximum_block_bytes&&(payload.size()-2)/2<=limits.envelope_bytes/2-total);
  const auto decoded_root=unhex(root);archive_require(decoded_root.size()==32&&hex(decoded_root)==root);const auto id=archive_array<32>(decoded_root,0);
  auto data=std::make_shared<const Bytes>(unhex(payload));archive_require(hex(*data)==payload);total+=data->size();verify_archived_block(id,*data,store,rules,cancel);archive_require(archive->emplace(id,std::move(data)).second);
 }
 auto votes=std::make_shared<AltairSignedVoteArchive>();
 if(with_votes){
  const auto& list=document.at("signedVotes");archive_require(list.is_array()&&!list.empty()&&list.size()<=limits.messages);
  for(const auto& entry:list){
   if(cancel)cancel();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
   const auto& root=entry[0].get_ref<const std::string&>();const auto& payload=entry[1].get_ref<const std::string&>();
   archive_require(root.size()==66&&payload.size()>=460&&payload.size()%2==0&&(payload.size()-2)/2<=228+uint64_t(rules.transition.body.validators_per_committee)/8+1&&(payload.size()-2)/2<=limits.envelope_bytes/2-total);
   auto data=std::make_shared<const Bytes>(unhex(payload));const auto digest=sha256(*data);archive_require(hex(digest)==root&&hex(*data)==payload);total+=data->size();
   const auto decoded=decode_consensus_attestation_ssz(*data,rules.transition.body.validators_per_committee,cancel);archive_require(decoded.has_value());
   // Local restore revalidates signature, target ancestry and gossip window;
   // it never publishes the candidate or re-applies a vote to the restored state.
   archive_require(prepare_altair_store_vote(store,*decoded,rules,limits.replay_slots,limits.sync_trials,cancel).has_value());
   archive_require(votes->emplace(archive_array<32>(digest,0),std::move(data)).second);
  }
 }
 archive_require(durable_snapshot(store,*archive,*votes,rules,limits,cancel)==bytes);return {std::move(store),std::move(archive),std::move(votes)};
}
}
static_assert(std::is_nothrow_move_assignable_v<std::optional<ForkCheckpointToken>>);
static_assert(std::is_nothrow_move_assignable_v<std::shared_ptr<const AltairForkStore>>);
void AltairDurableReactor::require_available()const{if(unavailable_||!state_||!token_)throw std::runtime_error("Altair reactor unavailable; reopen and resolve checkpoint required");}
void AltairDurableReactor::publish(AltairForkStore candidate,std::shared_ptr<const AltairSignedBlockArchive> archive,std::shared_ptr<const AltairSignedVoteArchive> votes,const std::function<void()>& checkpoint){
 auto next=std::make_shared<const AltairForkStore>(std::move(candidate));
 const auto oldest=next->timed.view.current_epoch?next->timed.view.current_epoch-1:0;
 if(std::any_of(votes->begin(),votes->end(),[&](const auto& entry){return archive_number(*entry.second,92)<oldest;})){
  auto retained=std::make_shared<AltairSignedVoteArchive>();for(const auto& [id,payload]:*votes){if(checkpoint)checkpoint();if(archive_number(*payload,92)>=oldest)retained->emplace(id,payload);}votes=std::move(retained);
 }
 const auto encoded=durable_snapshot(*next,*archive,*votes,rules_,limits_,checkpoint);
 if(checkpoint)checkpoint();
 // All candidate preparation/allocation/cancellation occurs before the write.
 // After a successful write, publishing shared_ptr and vector-backed token moves
 // cannot throw. No callback is invoked between commit and publication.
 ForkCheckpointToken committed;
 try{committed=disk_.commit_fork_checkpoint(chain_id_,genesis_,token_,encoded,limits_.envelope_bytes);}
 catch(...){unavailable_=true;throw;}
 token_=std::move(committed);state_=std::move(next);signed_blocks_=std::move(archive);signed_votes_=std::move(votes);
}
AltairDurableReactor::AltairDurableReactor(ChainStore& disk,Int chain,Bytes genesis,AltairForkBlockRules rules,
 AltairStoreSnapshotLimits limits,AltairForkStore initial,const std::function<void()>& checkpoint):
 disk_(disk),chain_id_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits){publish(std::move(initial),signed_blocks_,signed_votes_,checkpoint);}
AltairDurableReactor::AltairDurableReactor(ChainStore& disk,Int chain,Bytes genesis,AltairForkBlockRules rules,
 AltairStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint):
 disk_(disk),chain_id_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits){
 const auto record=disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes,expected);
 if(!record)throw std::runtime_error("missing Altair restart checkpoint");
 auto restored=restore_durable_snapshot(record->snapshot,expected.digest,rules_,limits_,checkpoint);
 state_=std::make_shared<const AltairForkStore>(std::move(restored.store));signed_blocks_=std::move(restored.blocks);signed_votes_=std::move(restored.votes);token_=expected;
}
AltairPublishedStore AltairDurableReactor::current()const{std::lock_guard lock(mutex_);require_available();return {state_,*token_,signed_blocks_,signed_votes_};}
void AltairDurableReactor::tick(uint64_t ms,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();publish(prepare_altair_store_tick(*state_,ms,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint),signed_blocks_,signed_votes_,checkpoint);
}
bool AltairDurableReactor::block(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_altair_received_block(*state_,bytes,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint);if(!next)return false;
 auto archive=signed_blocks_;
 if(next->blocks.size()>state_->blocks.size()){
  archive_require(next->blocks.size()==state_->blocks.size()+1);auto updated=std::make_shared<AltairSignedBlockArchive>(*signed_blocks_);bool added=false;
  for(const auto& [id,_]:next->blocks)if(!state_->blocks.contains(id)){archive_require(!added&&updated->emplace(id,std::make_shared<const Bytes>(bytes.begin(),bytes.end())).second);added=true;}
  archive_require(added);archive=std::move(updated);
 }
 // A duplicate root can bypass signature verification in fork-choice. Never
 // overwrite archived bytes, or backfill a legacy checkpoint from that path.
 publish(std::move(*next),std::move(archive),signed_votes_,checkpoint);return true;
}
bool AltairDurableReactor::vote(const Phase0SignedAttestation& vote,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_altair_store_vote(*state_,vote,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint);if(!next)return false;
 auto votes=signed_votes_;
 if(next->timed.view.latest_messages!=state_->timed.view.latest_messages){
  auto bytes=std::make_shared<const Bytes>(encode_consensus_attestation_ssz(vote,rules_.transition.body.validators_per_committee,checkpoint));
  auto updated=std::make_shared<AltairSignedVoteArchive>(*signed_votes_);updated->emplace(archive_array<32>(sha256(*bytes),0),std::move(bytes));votes=std::move(updated);
 }
 publish(std::move(*next),signed_blocks_,std::move(votes),checkpoint);return true;
}
bool AltairDurableReactor::slashing(const IndexedConsensusAttestation& a,const IndexedConsensusAttestation& b,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_altair_store_slashing(*state_,a,b,rules_,checkpoint);if(!next)return false;publish(std::move(*next),signed_blocks_,signed_votes_,checkpoint);return true;
}
}

namespace nano {
bool AltairDurableReactor::vote_ssz(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 auto decoded=decode_consensus_attestation_ssz(bytes,rules_.transition.body.validators_per_committee,checkpoint);
 if(!decoded){std::lock_guard lock(mutex_);require_available();return false;}
 return vote(*decoded,checkpoint);
}
bool AltairDurableReactor::slashing_ssz(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 auto decoded=decode_consensus_attester_slashing_ssz(bytes,rules_.transition.body.validators_per_committee,checkpoint);
 if(!decoded){std::lock_guard lock(mutex_);require_available();return false;}
 return slashing(decoded->first,decoded->second,checkpoint);
}
}
