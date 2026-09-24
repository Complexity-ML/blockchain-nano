#pragma once
#include "process.hpp"
#include "nano/consensus_slashing.hpp"
namespace test::phase0_minimal {
inline Bytes binary(const fs::path& path){auto s=read(path);return Bytes(s.begin(),s.end());}
inline uint64_t little(const Bytes& b,size_t at,size_t width){
 check(width<=8&&at<=b.size()&&width<=b.size()-at,"SSZ integer bounds");
 uint64_t value=0;for(size_t i=0;i<width;++i)value|=uint64_t(b[at+i])<<(i*8);return value;
}
inline uint64_t parameter(const std::string& yaml,const std::string& key){
 std::istringstream lines(yaml);std::string line;
 while(std::getline(lines,line))if(line.starts_with(key+":")){
  std::istringstream value(line.substr(key.size()+1));uint64_t n=0;check(bool(value>>n),"invalid preset parameter "+key);return n;
 }
 throw std::runtime_error("missing preset parameter "+key);
}
// Test-only reader of the Phase0 minimal BeaconState fields consumed by the
// slashing component. Offsets/layout are SSZ schema, not Nano snapshot fields.
inline ConsensusSlashingState decode(const Bytes& b){
 constexpr size_t fixed=7057,validator_size=121;
 check(b.size()>=fixed,"short minimal Phase0 BeaconState");
 std::array<size_t,7> offsets{size_t(little(b,4272,4)),size_t(little(b,4348,4)),size_t(little(b,4360,4)),size_t(little(b,4364,4)),size_t(little(b,6928,4)),size_t(little(b,6932,4)),b.size()};
 check(offsets[0]==fixed,"invalid SSZ fixed section offset");
 for(size_t i=1;i<offsets.size();++i)check(offsets[i]>=offsets[i-1]&&offsets[i]<=b.size(),"unordered SSZ offsets");
 check((offsets[1]-offsets[0])%32==0&&(offsets[2]-offsets[1])%72==0,"invalid history/eth1 list");
 check((offsets[3]-offsets[2])%validator_size==0,"invalid validator list length");
 auto count=(offsets[3]-offsets[2])/validator_size;
 check(offsets[4]-offsets[3]==count*8,"validator/balance count mismatch");
 ConsensusSlashingState state;state.epoch=little(b,40,8)/8;
 for(size_t i=0;i<64;++i)state.slashings.push_back(little(b,6416+i*8,8));
 for(size_t i=0;i<count;++i){
  auto at=offsets[2]+i*validator_size;ConsensusSlashingValidator v;
  std::copy_n(b.begin()+at,48,v.public_key.begin());v.effective_balance=little(b,at+80,8);
  check(b[at+88]<=1,"invalid SSZ boolean");v.slashed=b[at+88]!=0;
  std::copy_n(b.begin()+at+48,32,v.withdrawal_credentials.begin());v.activation_eligibility_epoch=little(b,at+89,8);v.activation_epoch=little(b,at+97,8);v.exit_epoch=little(b,at+105,8);v.withdrawable_epoch=little(b,at+113,8);
  v.balance=little(b,offsets[3]+i*8,8);state.validators.emplace(i,v);
 }
 return state;
}
}
