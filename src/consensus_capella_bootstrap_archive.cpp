#include "nano/consensus_capella_bootstrap_archive.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_beacon_header.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>
namespace nano {
namespace {
void require(bool ok){if(!ok)throw std::invalid_argument("Invalid Capella signed bootstrap archive");}
uint64_t number(std::span<const uint8_t> bytes,size_t at,size_t width=8){
 require(width<=8&&at<=bytes.size()&&width<=bytes.size()-at);uint64_t value=0;
 for(size_t i=0;i<width;++i)value|=uint64_t(bytes[at+i])<<(8*i);return value;
}
ConsensusRoot root(std::span<const uint8_t> bytes,size_t at){require(at<=bytes.size()&&32<=bytes.size()-at);ConsensusRoot value{};std::copy_n(bytes.begin()+at,32,value.begin());return value;}
}
CapellaSignedBootstrapArchive::CapellaSignedBootstrapArchive(Int chain,Bytes genesis,Json profile,
 ConsensusBeaconHeader header,Bytes wire,CapellaBlockCandidate post)
 :chain_(std::move(chain)),genesis_(std::move(genesis)),profile_(std::move(profile)),header_(header),wire_(std::move(wire)),post_(std::move(post)){}
CheckpointArchiveEntry CapellaSignedBootstrapArchive::entry()const{
 const auto id=consensus_beacon_header_root(header_);return {Bytes(id.begin(),id.end()),header_.slot,wire_};
}
bool CapellaSignedBootstrapArchive::matches(const CapellaForkStore& store,
 const CapellaForkBlockRules& rules,const std::function<void()>& check)const{
 if(check)check();const auto id=consensus_beacon_header_root(header_);
 if(profile_!=capella_fork_profile(rules)||chain_!=rules.transition.chain_id||
  header_.slot!=1||capella_network_anchor(store)!=id||!store.blocks.contains(id))return false;
 const auto block=store.blocks.at(id).lease(check);
 if(block->header!=header_||block->state!=post_.consensus||
  block->execution_block.to_json(true)!=post_.execution.block.to_json(true)||
  block->execution_state.root()!=post_.execution.state.root()||
  block->execution_state.to_json()!=post_.execution.state.to_json()||
  block->receipts.size()!=post_.execution.receipts.size())return false;
 for(size_t i=0;i<block->receipts.size();++i){if(check)check();if(block->receipts[i].to_json()!=post_.execution.receipts[i].to_json())return false;}
 if(check)check();return true;
}
std::optional<CapellaSignedBootstrapArchive> prepare_capella_signed_bootstrap_archive(
 Genesis genesis,CapellaSlotState pre,Bytes wire,CapellaForkBlockRules rules,
 uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 if(!maximum_slots||!trials)throw std::invalid_argument("Invalid Capella bootstrap continuation bounds");
 if(check)check();const auto profile=capella_fork_profile(rules);const auto encoded=profile.dump();
 (void)load_capella_fork_profile(encoded,sha256(Bytes(encoded.begin(),encoded.end())),1024*1024);
 const auto& r=rules.transition;const auto& limits=r.consensus.state;
 require(genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai&&
  genesis.chain_id==r.chain_id&&r.replay_policy==ReplayPolicy::RequireChainId&&
  genesis.block.header.number==0&&genesis.block.header.difficulty==0&&
  genesis.block.transactions.empty()&&genesis.block.uncles.empty()&&
  genesis.block.header.state_root==genesis.state.root());
 require(pre.ssz.size()<=r.maximum_state_bytes&&number(pre.ssz,40)==0&&!pre.last_processed_epoch&&
  genesis.block.header.timestamp==Int(std::to_string(number(pre.ssz,0))));
 const auto pre_root=capella_beacon_state_root(pre.ssz,limits,r.execution,r.maximum_state_bytes,check);
 const ConsensusBeaconHeader parent{number(pre.ssz,64),number(pre.ssz,72),root(pre.ssz,80),pre_root,root(pre.ssz,144)};
 (void)capella_fork_block_context(pre,parent,{r.consensus.epoch,r.execution,r.maximum_state_bytes},limits,check);
 if(wire.size()<404||wire.size()>r.consensus.maximum_block_bytes||number(wire,0,4)!=100||number(wire,180,4)!=84)return std::nullopt;
 const ConsensusBeaconHeader h{number(wire,100),number(wire,108),root(wire,116),root(wire,148),
  capella_beacon_body_root(std::span<const uint8_t>(wire).subspan(184),r.consensus.body,limits.sync_committee_size,
   r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,check)};
 if(h.slot!=1)return std::nullopt;
 const auto identity=prepare_capella_proposer_identity(pre,parent,1,{r.consensus.epoch,r.execution,r.maximum_state_bytes},
  limits,r.consensus.attestation.duties,maximum_slots,trials,check);
 if(!identity||identity->index!=h.proposer_index||!verify_consensus_beacon_header(h,identity->fork_version,
  identity->genesis_root,identity->public_key,std::span<const uint8_t>(wire).subspan(4,96)))return std::nullopt;
 const auto ancestor=[&](const Int& height){if(height==0)return genesis.block.header.hash();throw std::runtime_error("Unavailable bootstrap ancestor");};
 auto post=prepare_capella_state_transition(pre,wire,r,genesis.block.header,genesis.state,ancestor,maximum_slots,trials,check);
 if(!post)return std::nullopt;require(post->execution.block.header.number==1&&post->execution.block.header.parent_hash==genesis.block.header.hash());
 if(check)check();return CapellaSignedBootstrapArchive(genesis.chain_id,genesis.block.header.hash(),profile,h,std::move(wire),std::move(*post));
}
CapellaBootstrapArchivePlan::CapellaBootstrapArchivePlan(CapellaSignedBootstrapArchive proof,
 std::string snapshot,ForkCheckpointToken primary_before,ForkCheckpointToken witness_before,
 ForkCheckpointToken primary_after,ForkCheckpointToken witness_after,bool work)
 :proof_(std::move(proof)),snapshot_(std::move(snapshot)),primary_before_(std::move(primary_before)),
  witness_before_(std::move(witness_before)),primary_after_(std::move(primary_after)),witness_after_(std::move(witness_after)),work_(work){}
}
