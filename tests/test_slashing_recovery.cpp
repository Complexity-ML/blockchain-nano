#include "support/process.hpp"
#include "nano/consensus_committee.hpp"
#include "nano/consensus_attestation.hpp"
#include "nano/consensus_slashing.hpp"
#include "blst.h"
#include "nano/chain.hpp"
using namespace test;
namespace {
Bytes sign(const blst_scalar& sk,const ConsensusRoot& root){
 static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
 blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);
 blst_sign_pk_in_g1(&s,&h,&sk);Bytes out(96);blst_p2_compress(out.data(),&s);return out;
}
Bytes aggregate(const std::vector<Bytes>& signatures){
 blst_p2 sum{};bool first=true;
 for(const auto& bytes:signatures){blst_p2_affine p;check(blst_p2_uncompress(&p,bytes.data())==BLST_SUCCESS,"decode test signature");if(first){blst_p2_from_affine(&sum,&p);first=false;}else blst_p2_add_or_double_affine(&sum,&sum,&p);}
 Bytes out(96);blst_p2_compress(out.data(),&sum);return out;
}
}
int main(int argc,char** argv){try{
 ConsensusBytes4 version{1,0,0,0};ConsensusRoot genesis{},object{};genesis[0]=7;ConsensusAttestationData vote;vote.slot=64;vote.index=1;vote.beacon_block_root[0]=42;vote.source.epoch=1;vote.target.epoch=2;object=consensus_attestation_root(vote);
 auto domain=consensus_domain({1,0,0,0},version,genesis);
 std::vector<ConsensusCommitteeMember> members;std::vector<blst_scalar> secrets(3);
 for(size_t i=0;i<3;++i){
  std::array<uint8_t,32> seed{};seed[0]=uint8_t(i+1);blst_keygen(&secrets[i],seed.data(),seed.size(),nullptr,0);
  blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&secrets[i]);ConsensusCommitteeMember m;m.validator_index=10+i*10;m.effective_weight=i==2?UINT64_MAX:10;
  blst_p1_compress(m.possession_message.public_key.data(),&pk);m.possession_message.amount_gwei=32'000'000'000;
  auto root=consensus_signing_root(consensus_deposit_root(m.possession_message),consensus_domain({3,0,0,0},version,{}));m.possession_signature=sign(secrets[i],root);members.push_back(m);
 }
 ConsensusCommittee committee(members,version,domain);
 auto signed_vote=[&](ConsensusAttestationData data,std::vector<uint64_t> indices,const ConsensusRoot& signing_domain){
  std::vector<Bytes> signatures;for(auto index:indices)signatures.push_back(sign(secrets.at((index-10)/10),consensus_signing_root(consensus_attestation_root(data),signing_domain)));
  return IndexedConsensusAttestation{data,std::move(indices),aggregate(signatures)};
 };
 auto other=vote;other.beacon_block_root[0]^=1;

 auto first=signed_vote(vote,{10,20},domain),second=signed_vote(other,{20,30},domain);
 Phase0SlashingRules rules{1,65536,1,2,8,32,512,8};
 ConsensusSlashingState initial;initial.epoch=3;initial.slashings.resize(8);
 for(const auto& member:members){ConsensusSlashingValidator v;v.public_key=member.possession_message.public_key;v.balance=v.effective_balance=32000;v.activation_epoch=0;initial.validators.emplace(member.validator_index,v);}

 auto punished=initial;
 check(process_phase0_attester_slashing(punished,rules,first,committee,second,committee,10,30).has_value(),"prepare authenticated sanction");
 const auto before=phase0_slashing_snapshot(initial,rules),after=phase0_slashing_snapshot(punished,rules);
 const auto execution_genesis=Genesis::from_json(Json{{"config",{{"chainId",1337},{"consensus","pow"}}},{"alloc",Json::object()}});
 auto block=execution_genesis.block;block.header.number=1;block.header.timestamp=1;block.header.parent_hash=execution_genesis.block.header.hash();
 block.header.transactions_root=transactions_root({});block.header.receipts_root=receipts_root({});
 if(argc==3&&std::string(argv[1])=="--commit-abrupt-exit"){
  ChainStore store(argv[2]);store.bind_chain(execution_genesis.chain_id,execution_genesis.block.header.hash(),"pow");
  store.commit(execution_genesis.block,execution_genesis.state,{},before);
  store.commit(block,execution_genesis.state,{},after);
  // Deliberately skip RocksDB destructors after its synchronous batch commits.
  // Exercises WAL recovery, not a physical power-loss guarantee.
  _exit(0);
 }
 unsigned rejected=0,checks=0;
 check(phase0_slashing_from_snapshot(Json::parse(after.dump()),rules)==punished,"snapshot JSON roundtrip");++checks;
 auto maximum=initial;maximum.epoch=UINT64_MAX;maximum.validators.at(10).balance=UINT64_MAX;maximum.validators.at(10).effective_balance=UINT64_MAX;maximum.slashings[0]=UINT64_MAX;
 check(phase0_slashing_from_snapshot(phase0_slashing_snapshot(maximum,rules),rules)==maximum,"uint64 maximum roundtrip");++checks;
 auto reject=[&](Json bad){bool failed=false;try{phase0_slashing_from_snapshot(bad,rules);}catch(const std::exception&){failed=true;}check(failed,"malformed snapshot accepted");++rejected;};
 auto bad=after;bad["format"]="nano-phase0-slashing-v2";reject(bad);
 bad=after;bad["extra"]=1;reject(bad);
 bad=after;bad.erase("epoch");reject(bad);
 bad=after;bad["rules"][0]="0x2";reject(bad);
 bad=after;bad["slashings"].erase(bad["slashings"].end()-1);reject(bad);
 bad=after;bad["validators"].push_back(bad["validators"][0]);reject(bad);
 bad=after;std::swap(bad["validators"][0],bad["validators"][1]);reject(bad);
 bad=after;bad["validators"][1]["publicKey"]=bad["validators"][0]["publicKey"];reject(bad);
 bad=after;bad["validators"][0]["publicKey"]="0x00";reject(bad);
 bad=after;bad["validators"][0]["slashed"]=1;reject(bad);
 bad=after;bad["validators"][0]["balance"]="0x10000000000000000";reject(bad);
 bad=after;bad["validators"][0]["balance"]="0x01";reject(bad);
 bad=after;bad["epoch"]=-1;reject(bad);
 bad=after;bad["epoch"]=1.5;reject(bad);
 bad=after;bad["epoch"]="-0x1";reject(bad);
 bad=after;bad["validators"][1]["activationEpoch"]="0x6";reject(bad);
 bad=after;bad["validators"][1]["withdrawableEpoch"]="0x0";reject(bad);
 bad=after;bad["validators"][0]["extra"]=true;reject(bad);
 bad=after;bad["slashings"][0]=false;reject(bad);
 auto location=fs::temp_directory_path()/("nano-slashing-reopen-"+std::to_string(getpid()));
 check(!fs::exists(location),"test directory already exists");
 struct Cleanup{fs::path p;~Cleanup(){fs::remove_all(p);}} cleanup{location};
 Process child({argv[0],"--commit-abrupt-exit",location.string()});auto child_result=child.wait(30);check(child_result.code==0,"abrupt writer failed: "+child_result.err);++checks;
 {
  ChainStore store(location);store.bind_chain(execution_genesis.chain_id,execution_genesis.block.header.hash(),"pow");
  check(store.latest()->header.hash()==block.header.hash()&&store.count()==2,"recovered canonical tip");++checks;
  check(store.state_at(1)->root()==execution_genesis.state.root(),"recovered execution state");++checks;
  check(phase0_slashing_from_snapshot(*store.consensus_at(0),rules)==initial,"historical snapshot retained");++checks;
  auto recovered=phase0_slashing_from_snapshot(*store.consensus_at(1),rules);check(recovered==punished,"recovered sanctions and rewards");++checks;
  check(!process_phase0_attester_slashing(recovered,rules,first,committee,second,committee,10,30)&&recovered==punished,"replay after abrupt exit changed penalty");++checks;
  auto invalid=block;invalid.header.number=2;invalid.header.parent_hash=block.header.hash();invalid.header.state_root=Bytes(32);
  bool failed=false;try{store.commit(invalid,execution_genesis.state,{},before);}catch(const std::exception&){failed=true;}
  check(failed&&!store.consensus_at(2)&&!store.block_by_number(2)&&store.latest()->header.hash()==block.header.hash(),"rejected block persisted registry");++checks;
 }
 {
  ChainStore store(location);check(phase0_slashing_from_snapshot(*store.consensus_at(1),rules)==punished&&!store.consensus_at(2),"failed commit survives reopen");++checks;
  bool failed=false;try{store.bind_chain(999,execution_genesis.block.header.hash(),"pow");}catch(const std::exception&){failed=true;}check(failed,"cross-chain database binding accepted");++checks;
 }
 std::cout<<Json{{"scenario","slashing-snapshot-recovery"},{"checks",checks},{"rejectedSnapshots",rejected},{"abruptWriterExit",true},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
