#include "support/process.hpp"
#include "nano/consensus_block_operations.hpp"
#include "blst.h"
using namespace test;
namespace {
void put(Bytes& b,size_t at,uint64_t value,size_t width){for(size_t i=0;i<width;++i)b.at(at+i)=uint8_t(value>>(8*i));}
Bytes sign(const blst_scalar& key,const ConsensusRoot& root){static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&key);Bytes bytes(96);blst_p2_compress(bytes.data(),&s);return bytes;}
Bytes encode(const SignedConsensusBeaconHeader& h){Bytes bytes(208);put(bytes,0,h.message.slot,8);put(bytes,8,h.message.proposer_index,8);std::copy(h.message.parent_root.begin(),h.message.parent_root.end(),bytes.begin()+16);std::copy(h.message.state_root.begin(),h.message.state_root.end(),bytes.begin()+48);std::copy(h.message.body_root.begin(),h.message.body_root.end(),bytes.begin()+80);check(h.signature.size()==96,"test signature length");std::copy(h.signature.begin(),h.signature.end(),bytes.begin()+112);return bytes;}
}
int main(){try{
 const Phase0BodyLimits limits{16,2,128,16,16,2048};const ConsensusRandaoRules randao_rules{8,1};const Phase0SlashingRules slash_rules{2,32,4,256,64,64,512,8,1000000000,2};
 const Phase0StateFork fork{{1,0,0,0},{2,0,0,0},1};Phase0BlockContext context{16,{},fork.current_version,{},32000000000ULL,10,65536};context.parent_root[0]=8;context.genesis_root[0]=17;
 ConsensusSlashingState registry;registry.epoch=2;registry.slashings.resize(64);std::array<blst_scalar,3> keys;
 for(size_t i=0;i<keys.size();++i){std::array<uint8_t,32> seed{};seed[0]=i+11;blst_keygen(&keys[i],seed.data(),seed.size(),nullptr,0);blst_p1 pub;blst_sk_to_pk_in_g1(&pub,&keys[i]);ConsensusSlashingValidator v;blst_p1_compress(v.public_key.data(),&pub);v.balance=v.effective_balance=32000000000ULL;v.activation_epoch=0;registry.validators.emplace(i,v);}
 ConsensusRandaoState randao;randao.epoch=2;randao.mixes.resize(64);randao.last_reveal_slot=15;
 std::vector<Phase0ProposerValidator> candidates;for(const auto& [i,v]:registry.validators)candidates.push_back({i,v.effective_balance,v.activation_epoch,v.exit_epoch});
 const auto proposer=phase0_proposer_index(candidates,randao,randao_rules,context.slot,context.maximum_effective_balance,context.shuffle_rounds);const auto victim=(proposer+1)%3,other=(proposer+2)%3;
 using Proof=std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>;
 auto proof=[&](uint64_t index,uint64_t slot,const ConsensusBytes4& version){Proof out;out.first.message={slot,index,{},{},{}};out.second.message=out.first.message;out.second.message.body_root[0]=1;
  for(auto* header:{&out.first,&out.second})header->signature=sign(keys[index],consensus_signing_root(consensus_beacon_header_root(header->message),consensus_domain({0,0,0,0},version,context.genesis_root)));return out;};
 auto make=[&](const std::vector<Proof>& proofs,bool exit_pending=false){Bytes bytes(404);put(bytes,0,100,4);put(bytes,100,context.slot,8);put(bytes,108,proposer,8);put(bytes,180,84,4);std::copy(context.parent_root.begin(),context.parent_root.end(),bytes.begin()+116);
  ConsensusRoot epoch{};epoch[0]=2;auto reveal=sign(keys[proposer],consensus_signing_root(epoch,consensus_domain({2,0,0,0},context.fork_version,context.genesis_root)));std::copy(reveal.begin(),reveal.end(),bytes.begin()+184);
  put(bytes,384,220,4);for(size_t at=388;at<404;at+=4)put(bytes,at,220+proofs.size()*416,4);
  for(const auto& pair:proofs)for(const auto* h:{&pair.first,&pair.second}){auto raw=encode(*h);bytes.insert(bytes.end(),raw.begin(),raw.end());}
  if(exit_pending)bytes.resize(bytes.size()+112); // well-formed but unvalidated exit
  ConsensusBeaconHeader h{context.slot,proposer,context.parent_root,{},phase0_beacon_body_root(std::span<const uint8_t>(bytes).subspan(184),limits)};
  auto signature=sign(keys[proposer],consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},context.fork_version,context.genesis_root)));std::copy(signature.begin(),signature.end(),bytes.begin()+4);return bytes;};
 auto run=[&](const Bytes& bytes,const auto& reg,const auto& state,const std::function<void()>& checkpoint={}){return prepare_phase0_proposer_operations(bytes,context,limits,randao_rules,slash_rules,fork,reg,state,checkpoint);};
 const auto original=registry;const auto original_randao=randao;unsigned positive=0,rejected=0,cancelled=0,overflows=0;
 const auto old=proof(victim,1,fork.previous_version),current=proof(other,16,fork.current_version),self=proof(proposer,16,fork.current_version);
 auto valid=make({old,current});auto result=run(valid,registry,randao);check(result&&result->proposer_slashings_processed==2&&result->unprocessed_operations==std::array<size_t,4>{},"two valid proofs in signed block");
 auto expected=registry;check(bool(process_phase0_proposer_slashing(expected,slash_rules,old.first,old.second,fork.previous_version,context.genesis_root,proposer,proposer)),"expected old proof");check(bool(process_phase0_proposer_slashing(expected,slash_rules,current.first,current.second,fork.current_version,context.genesis_root,proposer,proposer)),"expected current proof");check(result->registry_after_proposer_slashings==expected&&result->authenticated.randao_after!=randao,"composed candidate mismatch");++positive;
 auto self_result=run(make({self}),registry,randao);check(self_result&&self_result->registry_after_proposer_slashings.validators.at(proposer).slashed,"self-slashing block semantics");++positive;
 auto empty=run(make({}),registry,randao);check(empty&&empty->registry_after_proposer_slashings==registry&&empty->proposer_slashings_processed==0,"empty operation candidate");++positive;
 auto pending=run(make({old},true),registry,randao);check(pending&&pending->unprocessed_operations==std::array<size_t,4>{0,0,0,1},"unvalidated exit silently discarded");++positive;
 auto refuse=[&](const Bytes& block){check(!run(block,registry,randao)&&registry==original&&randao==original_randao,"invalid operation leaked partial registry or mix");++rejected;};
 refuse(make({old,old})); // First applies privately; duplicate invalidates whole candidate.
 auto bad=current;bad.second.signature[0]^=1;refuse(make({old,bad}));
 refuse(make({proof(victim,1,fork.current_version)})); // Must use historical domain.
 auto corrupted=valid;corrupted.back()^=1;refuse(corrupted);corrupted=valid;corrupted[4]^=1;refuse(corrupted);
 auto replay=run(valid,result->registry_after_proposer_slashings,result->authenticated.randao_after);check(!replay,"same block replay");++rejected;
 // A failed late penalty must not publish the first penalty or the RANDAO mix.
 auto overflow_registry=registry;overflow_registry.slashings[2]=UINT64_MAX-context.maximum_effective_balance;const auto before=overflow_registry;bool overflow=false;
 try{run(valid,overflow_registry,randao);}catch(const std::overflow_error&){overflow=true;}check(overflow&&overflow_registry==before&&randao==original_randao,"late overflow partially committed block");++overflows;
 size_t total=0;check(bool(run(valid,registry,randao,[&]{++total;})),"checkpoint control");
 for(size_t stop=1;stop<=total;++stop){size_t calls=0;bool aborted=false;try{run(valid,registry,randao,[&]{if(++calls==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&registry==original&&randao==original_randao,"cancelled candidate published");++cancelled;}
 auto wrong_fork=fork;wrong_fork.current_version[0]^=1;bool mismatch=false;try{prepare_phase0_proposer_operations(valid,context,limits,randao_rules,slash_rules,wrong_fork,registry,randao);}catch(const std::invalid_argument&){mismatch=true;}check(mismatch,"inconsistent block/evidence domain accepted");++rejected;
 check(registry==original&&randao==original_randao,"canonical source changed on success");
 std::cout<<Json{{"scenario","block-proposer-operations"},{"positive",positive},{"rejected",rejected},{"cancellations",cancelled},{"lateOverflowRejections",overflows},{"canonicalInputsUnchanged",true},{"fullStateTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
