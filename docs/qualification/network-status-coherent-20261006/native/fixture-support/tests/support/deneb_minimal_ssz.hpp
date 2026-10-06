#pragma once
#include "phase0_minimal_ssz.hpp"
namespace test::deneb_minimal {
// Test-only Deneb minimal BeaconState registry projection. Sync committee size
// is 32; this is not a production/network SSZ decoder or complete state-root check.
inline nano::ConsensusSlashingState decode(const nano::Bytes& b){
 using namespace phase0_minimal;
 constexpr size_t fixed=10253,validator_size=121;
 check(b.size()>=fixed,"short minimal Deneb BeaconState");
 std::array<size_t,10> offsets{size_t(little(b,4272,4)),size_t(little(b,4348,4)),size_t(little(b,4360,4)),size_t(little(b,4364,4)),size_t(little(b,6928,4)),size_t(little(b,6932,4)),size_t(little(b,7057,4)),size_t(little(b,10229,4)),size_t(little(b,10249,4)),b.size()};
 check(offsets[0]==fixed,"invalid Deneb fixed section offset");
 for(size_t i=1;i<offsets.size();++i)check(offsets[i]>=offsets[i-1]&&offsets[i]<=b.size(),"unordered Deneb offsets");
 check((offsets[1]-offsets[0])%32==0&&(offsets[2]-offsets[1])%72==0,"invalid Deneb history/eth1 lists");
 check((offsets[3]-offsets[2])%validator_size==0,"invalid Deneb validator list");const auto count=(offsets[3]-offsets[2])/validator_size;
 check(offsets[4]-offsets[3]==count*8&&offsets[5]-offsets[4]==count&&offsets[6]-offsets[5]==count&&offsets[7]-offsets[6]==count*8,"invalid Deneb parallel lists");
 check((offsets[9]-offsets[8])%64==0&&b[6936]<16,"invalid Deneb summaries/justification bits");
 const auto execution_size=offsets[8]-offsets[7];check(execution_size>=584&&execution_size<=616&&little(b,offsets[7]+436,4)==584,"invalid Deneb execution header extra-data");
 nano::ConsensusSlashingState state;state.epoch=little(b,40,8)/8;
 for(size_t i=0;i<64;++i)state.slashings.push_back(little(b,6416+i*8,8));
 for(size_t i=0;i<count;++i){auto at=offsets[2]+i*validator_size;nano::ConsensusSlashingValidator v;
  std::copy_n(b.begin()+at,48,v.public_key.begin());v.effective_balance=little(b,at+80,8);check(b[at+88]<=1,"invalid Deneb slashed boolean");v.slashed=b[at+88]!=0;
  std::copy_n(b.begin()+at+48,32,v.withdrawal_credentials.begin());v.activation_eligibility_epoch=little(b,at+89,8);v.activation_epoch=little(b,at+97,8);v.exit_epoch=little(b,at+105,8);v.withdrawable_epoch=little(b,at+113,8);v.balance=little(b,offsets[3]+8*i,8);state.validators.emplace(i,v);
 }
 return state;
}
}
