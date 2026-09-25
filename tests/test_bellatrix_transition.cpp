#include "support/altair_transition_rules.hpp"
#include "nano/consensus_bellatrix_transition.hpp"
#include "nano/consensus_proposer.hpp"
#include "blst.h"
using namespace test;using namespace test::phase0_minimal;
namespace {
void put(Bytes& b,size_t at,uint64_t n,size_t width=8){for(size_t i=0;i<width;++i)b.at(at+i)=uint8_t(n>>(8*i));}
template<class A> void copy_to(A& dst,const Bytes& src){check(dst.size()==src.size(),"fixture width");std::copy(src.begin(),src.end(),dst.begin());}
BellatrixExecutionPayload payload(const Block& block){const auto& h=block.header;BellatrixExecutionPayload p;copy_to(p.parent_hash,h.parent_hash);copy_to(p.fee_recipient,unhex(h.coinbase));copy_to(p.state_root,h.state_root);copy_to(p.receipts_root,h.receipts_root);copy_to(p.logs_bloom,h.logs_bloom);copy_to(p.prev_randao,h.mix_hash);copy_to(p.block_hash,h.hash());auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());copy_to(p.base_fee_per_gas,fee);p.block_number=u64(h.number);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.timestamp=u64(h.timestamp);p.extra_data=h.extra_data;for(const auto& tx:block.transactions)p.transactions.push_back(tx.raw());return p;}
struct Key {
 blst_scalar secret;Bytes public_key;
 explicit Key(uint64_t index){uint64_t words[4]{index+1,0,0,0};blst_scalar_from_uint64(&secret,words);blst_p1 p;blst_sk_to_pk_in_g1(&p,&secret);public_key.resize(48);blst_p1_compress(public_key.data(),&p);}
 Bytes sign(const ConsensusRoot& root)const{static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),32,reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes out(96);blst_p2_compress(out.data(),&s);return out;}
};
Bytes signed_header(const ConsensusBeaconHeader& h,const Bytes& signature){Bytes out(208);put(out,0,h.slot);put(out,8,h.proposer_index);std::copy(h.parent_root.begin(),h.parent_root.end(),out.begin()+16);std::copy(h.state_root.begin(),h.state_root.end(),out.begin()+48);std::copy(h.body_root.begin(),h.body_root.end(),out.begin()+80);std::copy(signature.begin(),signature.end(),out.begin()+112);return out;}
}
int main(){try{
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",refs=fixtures/"consensus-bellatrix-containers-official",source=fixtures/"consensus-bellatrix-slots-official";
 const auto manifest=load(source/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","initial state revision");
 for(const auto& row:manifest.at("cases"))if(row.at("name")=="slots_1")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(source/path))).substr(2)==info.at("sha256").get<std::string>(),"initial state digest");
 auto common=altair_transition_rules(fixtures);const auto preset=read(refs/"minimal-bellatrix.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 for(const auto& row:load(refs/"manifest.json").at("presets"))check(hex(sha256(binary(refs/row.at("name").get<std::string>()))).substr(2)==row.at("sha256").get<std::string>(),"Bellatrix preset digest");
 common.epoch.rewards.inactivity_penalty_quotient=p("INACTIVITY_PENALTY_QUOTIENT_BELLATRIX");common.epoch.slashing.minimum_penalty_quotient=p("MIN_SLASHING_PENALTY_QUOTIENT_BELLATRIX");common.epoch.slashing.proportional_slashing_multiplier=p("PROPORTIONAL_SLASHING_MULTIPLIER_BELLATRIX");
 const auto clock_file=fixtures/"consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml";
 const auto clock_manifest=load(fixtures/"consensus-altair-fork-head-official/manifest.json");
 check(hex(sha256(binary(clock_file))).substr(2)==clock_manifest.at("referenceFiles").at("minimal-config-v1.6.0-beta.0.yaml").at("sha256").get<std::string>(),"clock config digest");
 const auto seconds=parameter(read(clock_file),"SECONDS_PER_SLOT");
 const BellatrixTransitionRules r{common,{uint32_t(p("MAX_EXTRA_DATA_BYTES")),uint32_t(p("MAX_BYTES_PER_TRANSACTION")),uint32_t(p("MAX_TRANSACTIONS_PER_PAYLOAD"))},4*1024*1024,seconds,1337,ReplayPolicy::RequireChainId};
 const auto& l=r.consensus.state;const auto& duties=r.consensus.attestation.duties;const uint64_t slots=duties.slots_per_epoch;
 const size_t hist=176+64*l.slots_per_historical_root,randao=hist+96,execution_field=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+229+96*l.sync_committee_size;
 BellatrixSlotState beacon{binary(source/"slots_1/pre.ssz"),std::nullopt};
 const auto genesis_time=little(beacon.ssz,0,8);const auto genesis_root=root_at(beacon.ssz,8);
 const auto key=bytes(Int(777),32);const std::string target="0x0000000000000000000000000000000000001234";
 Transaction tx;tx.type=2;tx.to=target;tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;
 const auto sender=SignedTransaction::sign(tx,key).sender();State state;state.get(sender).balance=Int("1000000000000000000");state.get(target).code=unhex("60005460010160005500");
 Block parent;parent.header.difficulty=0;parent.header.timestamp=Int(std::to_string(genesis_time));parent.header.state_root=state.root();
 const auto initial_header=encode_bellatrix_execution_header(bellatrix_execution_payload_header(payload(parent),r.execution),r.execution.extra_data_bytes,r.maximum_state_bytes);
 beacon.ssz.resize(little(beacon.ssz,execution_field,4));beacon.ssz.insert(beacon.ssz.end(),initial_header.begin(),initial_header.end());
 std::map<Int,Bytes> hashes{{0,parent.header.hash()}};std::function<Bytes(const Int&)> history=[&](const Int& n){return hashes.at(n);};
 size_t blocks=0,refusals=0,cancellations=0;Bytes first_proof;uint64_t offender=0;
 for(uint64_t slot=1;slot<=slots+1;++slot){
  const auto old_beacon=beacon;const auto old_state=state.root();
  const auto advanced=prepare_bellatrix_slots(beacon,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l,slot,64,100000);
  const auto epoch=slot/slots;ConsensusRandaoState randomness;randomness.epoch=epoch;randomness.last_reveal_slot=little(advanced.ssz,64,8);
  for(size_t i=0;i<l.epochs_per_historical_vector;++i)randomness.mixes.push_back(root_at(advanced.ssz,randao+32*i));
  const auto validators=little(advanced.ssz,hist+88,4),balances=little(advanced.ssz,hist+92,4);std::vector<Phase0ProposerValidator> registry;
  for(size_t i=0;i<(balances-validators)/121;++i){const auto at=validators+121*i;registry.push_back({i,little(advanced.ssz,at+80,8),little(advanced.ssz,at+97,8),little(advanced.ssz,at+105,8)});}
  check(registry.size()==64,"expected 64 validators");const auto proposer=phase0_proposer_index(registry,randomness,{slots,duties.min_seed_lookahead},slot,duties.maximum_effective_balance,duties.shuffle_rounds);
  Key signing(proposer);check(std::equal(signing.public_key.begin(),signing.public_key.end(),advanced.ssz.begin()+validators+121*proposer),"fixture BLS key mismatch");
  ConsensusBytes4 version{};const size_t version_at=epoch<little(advanced.ssz,56,8)?48:52;std::copy_n(advanced.ssz.begin()+version_at,4,version.begin());
  ConsensusRoot epoch_root{};for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
  const auto reveal=signing.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,genesis_root)));
  const ConsensusBeaconHeader previous{little(advanced.ssz,64,8),little(advanced.ssz,72,8),root_at(advanced.ssz,80),root_at(advanced.ssz,112),root_at(advanced.ssz,144)};
  const auto parent_root=consensus_beacon_header_root(previous);
  Block execution;execution.header.parent_hash=parent.header.hash();execution.header.number=parent.header.number+1;execution.header.timestamp=Int(std::to_string(genesis_time+slot*seconds));execution.header.difficulty=0;execution.header.base_fee=next_base_fee(parent.header);execution.header.mix_hash=Bytes(randomness.mixes[epoch%l.epochs_per_historical_vector].begin(),randomness.mixes[epoch%l.epochs_per_historical_vector].end());
  tx.nonce=Int(blocks);execution.transactions.push_back(SignedTransaction::sign(tx,key));State expected=state;ExecutionEngine(r.chain_id,history,EvmRevision::Paris,r.replay_policy).execute_block(expected,execution,false);
  const auto encoded=encode_bellatrix_execution_payload(payload(execution),r.execution,r.consensus.maximum_block_bytes);
  const size_t sync_bytes=(l.sync_committee_size+7)/8+96,fixed=224+sync_bytes;
  auto body=[&](const Bytes& proof){Bytes b(fixed,0);std::copy(reveal.begin(),reveal.end(),b.begin());std::copy_n(advanced.ssz.begin()+hist+4,72,b.begin()+96);b[220+(l.sync_committee_size+7)/8]=0xc0;put(b,200,fixed,4);for(size_t at:std::array<size_t,5>{204,208,212,216,220+sync_bytes})put(b,at,fixed+proof.size(),4);b.insert(b.end(),proof.begin(),proof.end());b.insert(b.end(),encoded.begin(),encoded.end());return b;};
  const auto contents=body(slot==slots+1?first_proof:Bytes{});
  const auto built=prepare_bellatrix_unsigned_block(beacon,slot,proposer,parent_root,contents,r,parent.header,state,history,64,100000);check(built.has_value(),"Bellatrix construction rejected");
  auto sign_message=[&](Bytes message,uint64_t index=UINT64_MAX){const auto owner=index==UINT64_MAX?proposer:index;Key k(owner);const auto bytes=std::span<const uint8_t>(message);const ConsensusBeaconHeader h{little(message,0,8),little(message,8,8),root_at(message,16),root_at(message,48),bellatrix_beacon_body_root(bytes.subspan(84),r.consensus.body,l.sync_committee_size,r.execution,r.consensus.maximum_block_bytes)};const auto sig=k.sign(consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},version,genesis_root)));Bytes wire(100,0);put(wire,0,100,4);std::copy(sig.begin(),sig.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());return wire;};
  auto wire=sign_message(built->message_ssz);size_t calls=0;
  const auto received=prepare_bellatrix_state_transition(beacon,wire,r,parent.header,state,history,64,100000,[&]{++calls;});
  check(received&&received->consensus==built->post.consensus&&received->execution.state.root()==expected.root(),"Bellatrix signed transition differs");
  check(received->execution.state.get_storage(target,0)==Int(blocks+1)&&received->execution.receipts[0].success,"EVM write missing");
  if(slot==1){
   offender=proposer;const ConsensusBeaconHeader h{slot,proposer,parent_root,root_at(built->message_ssz,48),bellatrix_beacon_body_root(contents,r.consensus.body,l.sync_committee_size,r.execution,r.consensus.maximum_block_bytes)};
   auto conflicting=h;conflicting.body_root[0]^=1;const auto sign_header=[&](const ConsensusBeaconHeader& header){return signing.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},version,genesis_root)));};first_proof=signed_header(h,sign_header(h));const auto other=signed_header(conflicting,sign_header(conflicting));first_proof.insert(first_proof.end(),other.begin(),other.end());
   auto deny=[&](const Bytes& bad){bool refused=false;try{refused=!prepare_bellatrix_state_transition(beacon,bad,r,parent.header,state,history,64,100000);}catch(const std::invalid_argument&){refused=true;}check(refused&&beacon==old_beacon&&state.root()==old_state,"bad signed block accepted or mutated parents");++refusals;};
   auto bad=wire;bad[4]^=1;deny(bad);
   auto message=built->message_ssz;message[48]^=1;deny(sign_message(message));
   message=built->message_ssz;message[16]^=1;deny(sign_message(message));
   message=built->message_ssz;message[84]^=1;deny(sign_message(message));
   message=built->message_ssz;message[84+fixed+472]^=1;deny(sign_message(message));
   message=built->message_ssz;std::copy(reveal.begin(),reveal.end(),message.begin()+84+220+(l.sync_committee_size+7)/8);deny(sign_message(message));
   message=built->message_ssz;const auto other_index=(proposer+1)%registry.size();put(message,8,other_index);Key other_key(other_index);const auto wrong_reveal=other_key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,genesis_root)));std::copy(wrong_reveal.begin(),wrong_reveal.end(),message.begin()+84);deny(sign_message(message,other_index));
   for(size_t stop:std::set<size_t>{1,calls/4,calls/2,3*calls/4,calls-1,calls}){size_t seen=0;bool stopped=false;try{prepare_bellatrix_state_transition(beacon,wire,r,parent.header,state,history,64,100000,[&]{if(++seen==stop)throw std::runtime_error("cancel full block");});}catch(const std::runtime_error& e){stopped=std::string(e.what())=="cancel full block";}check(stopped&&beacon==old_beacon&&state.root()==old_state,"full transition cancellation leaked state");++cancellations;}
   bool cancellation_type=false;try{prepare_bellatrix_state_transition(beacon,wire,r,parent.header,state,history,64,100000,[]{throw std::invalid_argument("cancel body hash");});}catch(const std::invalid_argument& e){cancellation_type=std::string(e.what())=="cancel body hash";}check(cancellation_type,"body cancellation treated as invalid peer block");++cancellations;
   auto small=r;small.maximum_state_bytes=beacon.ssz.size()-1;bool state_budget=false;try{prepare_bellatrix_state_transition(beacon,wire,small,parent.header,state,history,64,100000);}catch(const std::length_error&){state_budget=true;}check(state_budget,"signed transition state budget ignored");
   small=r;small.consensus.maximum_block_bytes=wire.size()-1;check(!prepare_bellatrix_state_transition(beacon,wire,small,parent.header,state,history,64,100000),"signed transition block budget ignored");
   bool slots_budget=false;try{prepare_bellatrix_state_transition(beacon,wire,r,parent.header,state,history,0,100000);}catch(const std::length_error&){slots_budget=true;}check(slots_budget,"signed transition slots budget ignored");
  }
  if(slot==slots+1){
   const auto control=prepare_bellatrix_unsigned_block(beacon,slot,proposer,parent_root,body({}),r,parent.header,state,history,64,100000);check(control.has_value(),"slashing control failed");
   const auto start=little(received->consensus.ssz,hist+88,4),balance_start=little(received->consensus.ssz,hist+92,4),control_balances=little(control->post.consensus.ssz,hist+92,4);
   check(received->consensus.ssz[start+121*offender+88]==1,"double proposal not slashed");
   const auto effective=little(advanced.ssz,validators+121*offender+80,8),penalty=effective/r.consensus.epoch.slashing.minimum_penalty_quotient,reward=effective/r.consensus.epoch.slashing.whistleblower_reward_quotient;
   for(size_t i=0;i<registry.size();++i){const __int128 want=__int128(little(control->post.consensus.ssz,control_balances+8*i,8))-(i==offender?penalty:0)+(i==proposer?reward:0);check(__int128(little(received->consensus.ssz,balance_start+8*i,8))==want,"Bellatrix slashing economics mismatch");}
  }
  check(beacon==old_beacon&&state.root()==old_state,"successful preparation changed parents");beacon=received->consensus;state=received->execution.state;parent=received->execution.block;hashes[parent.header.number]=parent.header.hash();++blocks;
 }
 check(beacon.last_processed_epoch==0,"epoch boundary not processed");
 std::cout<<Json{{"scenario","signed-bellatrix-evm-chain"},{"blocks",blocks},{"validators",64},{"invalidSignedBlocks",refusals},{"cancellations",cancellations},{"doubleProposalSlashed",true},{"durableCommit",false},{"networkActive",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
