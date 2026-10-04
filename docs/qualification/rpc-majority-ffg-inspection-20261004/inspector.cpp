#include "nano/chain.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_message_ssz.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace nano;
static Json load(const char* p){std::ifstream f(p);if(!f)throw std::runtime_error("input unavailable");return Json::parse(f);}
static std::string raw(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("profile unavailable");return {std::istreambuf_iterator<char>(f),{}};}
static uint64_t number(const Json& x){return u64(quantity(x));}
static size_t offset(const Bytes& b,size_t p){if(p>b.size()||b.size()-p<4)throw std::runtime_error("truncated offset");size_t n=0;for(size_t i=0;i<4;++i)n|=size_t(b[p+i])<<(8*i);return n;}
int main(int argc,char** argv){try{
 if(argc!=5)throw std::runtime_error("usage: inspect genesis node stopped-database public-stop-receipt");
 const auto g=Genesis::from_json(load(argv[1]));const auto c=load(argv[2]).at("capella"),l=c.at("limits"),receipt=load(argv[4]);
 CapellaStoreSnapshotLimits limits{number(l.at("envelopeBytes")),number(l.at("blocks")),number(l.at("messages")),number(l.at("equivocations")),number(l.at("stateBytes")),number(l.at("totalStateBytes")),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 auto profile=raw(c.at("profile").at("file"));const auto rules=load_capella_fork_profile(profile,unhex(c.at("profile").at("sha256")),limits.envelope_bytes);
 ForkCheckpointToken expected{number(receipt.at("restart").at("sequence")),unhex(receipt.at("restart").at("digest"))};
 ChainStore disk(argv[3],ChainStore::OpenMode::ReadOnly);disk.bind_chain(g.chain_id,g.block.header.hash(),"beacon");
 const auto record=disk.fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes,expected);if(!record)throw std::runtime_error("checkpoint unavailable");
 CapellaDurableReactor restored(disk,g.chain_id,g.block.header.hash(),rules,limits,expected);const auto current=restored.current();const auto& store=*current.state;
 const auto& head=capella_fork_head(store,rules);const auto& t=rules.transition;const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);

 const auto& lstate=t.consensus.state;const auto& ffg=t.consensus.epoch.ffg;
 const CapellaEpochRules epoch_rules{t.consensus.epoch,t.execution,t.maximum_state_bytes};
 auto summarize=[&](const CapellaSlotState& state){
  const auto& b=state.ssz;
  auto n=[&](size_t at,size_t width=8){if(at>b.size()||width>b.size()-at)throw std::runtime_error("state bounds");uint64_t v=0;for(size_t i=0;i<width;++i)v|=uint64_t(b[at+i])<<(8*i);return v;};
  auto r=[&](size_t at){if(at>b.size()||32>b.size()-at)throw std::runtime_error("root bounds");return hex(Bytes(b.begin()+at,b.begin()+at+32));};
  const auto epoch=n(40)/ffg.slots_per_epoch;const auto hist=176+64*lstate.slots_per_historical_root;
  const auto fixed=hist+96+32*lstate.epochs_per_historical_vector+8*lstate.epochs_per_slashings_vector,cp=fixed+9;
  const auto validators=n(hist+88,4),balances=n(hist+92,4),previous=n(fixed,4),current=n(fixed+4,4),count=(balances-validators)/121;
  Int total=0,prev=0,now=0;Json previous_indices=Json::array(),current_indices=Json::array();
  for(size_t i=0;i<count;++i){const auto at=validators+121*i;const auto stake=Int(std::to_string(n(at+80)));const bool active=n(at+97)<=epoch&&epoch<n(at+105),was=epoch&&n(at+97)<=epoch-1&&epoch-1<n(at+105);
   if(active)total+=stake;if(!b[at+88]){if(was&&(b[previous+i]&2)){prev+=stake;previous_indices.push_back(i);}if(active&&(b[current+i]&2)){now+=stake;current_indices.push_back(i);}}
  }
  auto checkpoint=[&](size_t at){return Json{{"epoch",n(at)},{"root",r(at+8)}};};
  return Json{{"slot",n(40)},{"epoch",epoch},{"justificationBits",unsigned(b[fixed+8])},{"previousJustified",checkpoint(cp)},{"currentJustified",checkpoint(cp+40)},{"finalized",checkpoint(cp+80)},{"totalStake",quantity(total)},{"previousTargetStake",quantity(prev)},{"currentTargetStake",quantity(now)},{"previousIndices",previous_indices},{"currentIndices",current_indices},{"previousTargetRoot",r(176+32*(((epoch-1)*ffg.slots_per_epoch)%lstate.slots_per_historical_root))},{"currentTargetRoot",r(176+32*((epoch*ffg.slots_per_epoch)%lstate.slots_per_historical_root))}};
 };
 std::vector<const CapellaForkBlock*> canonical;auto root=consensus_beacon_header_root(head.header);
 while(store.blocks.contains(root)){const auto& b=store.blocks.at(root);canonical.push_back(&b);if(root==store.timed.view.anchor)break;root=b.header.parent_root;}
 Json observations=Json::array();
 for(uint64_t e=2;e<=7;++e){const auto boundary=(e+1)*ffg.slots_per_epoch;const CapellaForkBlock* before=nullptr;const CapellaForkBlock* after=nullptr;
  for(auto* b:canonical){if(b->header.slot<boundary&&(!before||b->header.slot>before->header.slot))before=b;if(b->header.slot>=boundary&&(!after||b->header.slot<after->header.slot))after=b;}
  if(!before||!after)continue;
  auto pre=before->state;if(before->header.slot<boundary-1)pre=prepare_capella_slots(pre,epoch_rules,lstate,boundary-1,limits.replay_slots,limits.sync_trials);
  const auto post=prepare_capella_slots(pre,epoch_rules,lstate,boundary,limits.replay_slots,limits.sync_trials);
  observations.push_back({{"boundary",boundary},{"parentSlot",before->header.slot},{"firstPostBoundaryBlockSlot",after->header.slot},{"pre",summarize(pre)},{"computedPost",summarize(post)},{"persistedPostBlock",summarize(after->state)}});
 }
 std::cout<<Json{{"externalCheckpointMatched",true},{"readOnly",true},{"historicalForkChoiceVerified",false},{"boundaries",observations}}.dump(2)<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
