#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_block_auth.hpp"
#include "blst.h"
using namespace test;
using namespace test::phase0_minimal;
namespace {
ConsensusRandaoState projection(const Bytes& bytes){ConsensusRandaoState state;state.epoch=decode(bytes).epoch;state.mixes.resize(64);for(size_t i=0;i<64;++i)std::copy_n(bytes.begin()+4368+32*i,32,state.mixes[i].begin());state.last_reveal_slot=little(bytes,64,8);return state;}
void put(Bytes& bytes,size_t at,uint64_t value,size_t width){for(size_t i=0;i<width;++i)bytes.at(at+i)=uint8_t(value>>(8*i));}
Bytes sign(const blst_scalar& sk,const ConsensusRoot& root){static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&sk);Bytes out(96);blst_p2_compress(out.data(),&s);return out;}
}
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-randao-official";const auto manifest=load(fixtures/"manifest.json");
 check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","official revision");
 const Phase0BodyLimits limits{16,2,128,16,16,2048};const ConsensusRandaoRules rules{8,1};unsigned official=0,rejected=0,cancellations=0,local=0,config_rejections=0,truncated=0;
 for(const auto& row:manifest.at("cases")){
  for(const auto& [path,info]:row.at("files").items()){auto b=binary(fixtures/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  check(row.at("blockCount")==1,"single block scenario");const auto name=row.at("name").get<std::string>();const auto pre=binary(fixtures/name/"pre.ssz"),post=binary(fixtures/name/"post.ssz"),block=binary(fixtures/name/"blocks_0.ssz");
  auto state=projection(pre);const auto expected=projection(post);auto registry=decode(pre);Phase0BlockContext context{};context.slot=little(block,100,8);context.maximum_effective_balance=32000000000ULL;context.shuffle_rounds=10;context.maximum_serialized_bytes=1024*1024;
  // Trusted official slot-processing result supplies the parent commitment.
  check(context.slot>0,"non-genesis block");std::copy_n(post.begin()+176+((context.slot-1)%64)*32,32,context.parent_root.begin());
  std::copy_n(pre.begin()+8,32,context.genesis_root.begin());std::copy_n(pre.begin()+(context.slot/8<little(pre,56,8)?48:52),4,context.fork_version.begin());
  while(state.epoch<context.slot/8)advance_consensus_randao_epoch(state,rules,state.epoch);
  const auto post_registry=decode(post);for(const auto& [index,v]:registry.validators){const auto& w=post_registry.validators.at(index);check(v.public_key==w.public_key&&v.effective_balance==w.effective_balance&&v.activation_epoch==w.activation_epoch&&v.exit_epoch==w.exit_epoch&&v.slashed==w.slashed,"fixture proposer fields changed");}registry.epoch=state.epoch;
  const auto saved=state;const auto saved_registry=registry;
  auto result=authenticate_phase0_block(block,context,limits,rules,registry,state);check(result&&result->randao_after==expected&&state==saved&&registry==saved_registry,"official authenticated projection or input isolation");++official;
  auto refuse=[&](const Bytes& b,const auto& c,const auto& reg,const auto& r){check(!authenticate_phase0_block(b,c,limits,rules,reg,r),"invalid block authenticated");check(state==saved&&registry==saved_registry,"failed block mutated canonical inputs");++rejected;};
  for(size_t at:{size_t(0),size_t(4),size_t(100),size_t(108),size_t(116),size_t(148),size_t(180),size_t(184),size_t(352),size_t(384)}){auto b=block;b[at]^=1;refuse(b,context,registry,state);}
  auto b=block;b.pop_back();refuse(b,context,registry,state);b=block;b.push_back(0);refuse(b,context,registry,state);
  auto c=context;c.parent_root[0]^=1;refuse(block,c,registry,state);c=context;c.genesis_root[0]^=1;refuse(block,c,registry,state);c=context;c.fork_version[0]^=1;refuse(block,c,registry,state);
  c=context;c.maximum_serialized_bytes=404; // All selected official blocks are larger than an empty no-operation body only if applicable.
  if(block.size()>404)refuse(block,c,registry,state);
  auto reg=registry;reg.validators.at(result->header.proposer_index).slashed=true;refuse(block,context,reg,state);
  reg=registry;reg.validators.erase(result->header.proposer_index);refuse(block,context,reg,state);
  refuse(block,context,registry,result->randao_after);
  // Cancel at every actual checkpoint, including after the candidate mix was computed.
  size_t checkpoints=0;check(bool(authenticate_phase0_block(block,context,limits,rules,registry,state,[&]{++checkpoints;})),"checkpoint count control");
  for(size_t stop=1;stop<=checkpoints;++stop){size_t calls=0;bool aborted=false;try{authenticate_phase0_block(block,context,limits,rules,registry,state,[&]{if(++calls==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&state==saved&&registry==saved_registry,"cancellation mutated inputs");++cancellations;}
 }
 // Locally sign blocks to distinguish a valid block signature from a valid duty/reveal.
 std::array<blst_scalar,2> keys;ConsensusSlashingState registry;registry.epoch=0;
 for(size_t i=0;i<2;++i){std::array<uint8_t,32> seed{};seed[0]=i+1;blst_keygen(&keys[i],seed.data(),seed.size(),nullptr,0);blst_p1 pub;blst_sk_to_pk_in_g1(&pub,&keys[i]);ConsensusSlashingValidator v;blst_p1_compress(v.public_key.data(),&pub);v.activation_epoch=0;v.effective_balance=v.balance=32000000000ULL;registry.validators.emplace(i,v);}
 ConsensusRandaoState state;state.mixes.resize(64);state.last_reveal_slot=0;const auto saved=state;
 Phase0BlockContext context{1,{}, {},{},32000000000ULL,10,1024};std::vector<Phase0ProposerValidator> candidates;for(const auto& [i,v]:registry.validators)candidates.push_back({i,v.effective_balance,v.activation_epoch,v.exit_epoch});
 auto selected=phase0_proposer_index(candidates,state,rules,1,context.maximum_effective_balance,10);const auto other=1-selected;
 auto make=[&](uint64_t index,bool valid_reveal){Bytes b(404);put(b,0,100,4);put(b,100,1,8);put(b,108,index,8);put(b,180,84,4);for(size_t at=384;at<404;at+=4)put(b,at,220,4);
  auto reveal=sign(keys[valid_reveal?index:1-index],consensus_signing_root({},consensus_domain({2,0,0,0},context.fork_version,context.genesis_root)));std::copy(reveal.begin(),reveal.end(),b.begin()+184);
  ConsensusBeaconHeader h{1,index,{},{},phase0_beacon_body_root(std::span<const uint8_t>(b).subspan(184),limits)};auto signature=sign(keys[index],consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},context.fork_version,context.genesis_root)));std::copy(signature.begin(),signature.end(),b.begin()+4);
  check(verify_consensus_beacon_header(h,context.fork_version,context.genesis_root,registry.validators.at(index).public_key,signature),"local genuine signature control");return b;};
 check(bool(authenticate_phase0_block(make(selected,true),context,limits,rules,registry,state)),"local selected valid block");++local;
 check(!authenticate_phase0_block(make(other,true),context,limits,rules,registry,state),"signed non-proposer accepted");++local;
 check(!authenticate_phase0_block(make(selected,false),context,limits,rules,registry,state),"signed block with invalid reveal accepted");++local;
 auto oversized=make(selected,true);oversized.resize(1025);check(!authenticate_phase0_block(oversized,context,limits,rules,registry,state),"transport byte budget ignored");++local;
 const auto valid=make(selected,true);
 for(size_t n=0;n<404;++n){check(!authenticate_phase0_block(std::span<const uint8_t>(valid).first(n),context,limits,rules,registry,state),"truncated envelope accepted");++truncated;}
 auto configuration_error=[&](const auto& c,const auto& body_limits,const auto& randao_rules,const auto& reg){bool failed=false;try{authenticate_phase0_block(valid,c,body_limits,randao_rules,reg,state);}catch(const std::invalid_argument&){failed=true;}check(failed&&state==saved,"configuration failure not isolated");++config_rejections;};
 auto c=context;c.maximum_effective_balance=0;configuration_error(c,limits,rules,registry);
 c=context;c.shuffle_rounds=257;configuration_error(c,limits,rules,registry);
 c=context;c.maximum_serialized_bytes=403;configuration_error(c,limits,rules,registry);
 c=context;c.slot=8;configuration_error(c,limits,rules,registry);
 auto wrong_limits=limits;wrong_limits.validators_per_committee=0;configuration_error(context,wrong_limits,rules,registry);
 auto wrong_rules=rules;wrong_rules.slots_per_epoch=0;configuration_error(context,limits,wrong_rules,registry);
 auto wrong_registry=registry;wrong_registry.epoch=1;configuration_error(context,limits,rules,wrong_registry);
 check(state==saved,"local canonical mix changed");check(official==3,"official case count");
 std::cout<<Json{{"scenario","phase0-block-authentication"},{"officialBlocks",official},{"rejected",rejected},{"cancellations",cancellations},{"localControls",local},{"configurationErrors",config_rejections},{"truncatedRejected",truncated},{"canonicalInputsUnchanged",true},{"fullStateTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
