#include "support/process.hpp"
#include "nano/consensus_slashing.hpp"
using namespace test;
namespace {
Bytes binary(const fs::path& path){auto s=read(path);return Bytes(s.begin(),s.end());}
uint64_t little(const Bytes& b,size_t at,size_t width){
 check(width<=8&&at<=b.size()&&width<=b.size()-at,"SSZ integer bounds");
 uint64_t value=0;for(size_t i=0;i<width;++i)value|=uint64_t(b[at+i])<<(i*8);return value;
}
uint64_t parameter(const std::string& yaml,const std::string& key){
 std::istringstream lines(yaml);std::string line;
 while(std::getline(lines,line))if(line.starts_with(key+":")){
  std::istringstream value(line.substr(key.size()+1));uint64_t n=0;check(bool(value>>n),"invalid preset parameter "+key);return n;
 }
 throw std::runtime_error("missing preset parameter "+key);
}
// Test-only reader of the Phase0 minimal BeaconState fields consumed by the
// slashing component. Offsets/layout are SSZ schema, not Nano snapshot fields.
ConsensusSlashingState decode(const Bytes& b){
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
  v.activation_epoch=little(b,at+97,8);v.exit_epoch=little(b,at+105,8);v.withdrawable_epoch=little(b,at+113,8);
  v.balance=little(b,offsets[3]+i*8,8);state.validators.emplace(i,v);
 }
 return state;
}
}
int main(int argc,char** argv){try{
 const fs::path root=argc==2?argv[1]:fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-slashing-official";
 auto manifest=load(root/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","unexpected fixture revision");
 for(const auto& [name,entry]:manifest.at("referenceFiles").items())check(hex(sha256(binary(root/name))).substr(2)==entry.at("sha256").get<std::string>(),"reference hash mismatch");
 const auto preset=read(root/"minimal-phase0.yaml"),config=read(root/"minimal-config.yaml");
 auto p=[&](const char* key){return parameter(preset,key);};auto c=[&](const char* key){return parameter(config,key);};
 check(p("SLOTS_PER_EPOCH")==8&&p("SLOTS_PER_HISTORICAL_ROOT")==64&&p("EPOCHS_PER_HISTORICAL_VECTOR")==64&&p("EPOCHS_PER_SLASHINGS_VECTOR")==64,"unsupported fixture SSZ preset");
 Phase0SlashingRules rules{c("MIN_PER_EPOCH_CHURN_LIMIT"),c("CHURN_LIMIT_QUOTIENT"),p("MAX_SEED_LOOKAHEAD"),c("MIN_VALIDATOR_WITHDRAWABILITY_DELAY"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("MIN_SLASHING_PENALTY_QUOTIENT"),p("WHISTLEBLOWER_REWARD_QUOTIENT"),p("PROPOSER_REWARD_QUOTIENT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("PROPORTIONAL_SLASHING_MULTIPLIER")};
 size_t cases=0,balances=0,changed=0;std::set<std::string> names;
 for(const auto& entry:manifest.at("cases")){
  auto name=entry.at("name").get<std::string>();check(names.insert(name).second,"duplicate fixture case");
  for(const auto& [path,info]:entry.at("files").items()){auto b=binary(root/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture hash mismatch "+path);}
  auto pre=decode(binary(root/name/"pre.ssz")),expected=decode(binary(root/name/"post.ssz")),actual=pre;
  for(const auto& [index,debit]:phase0_slashing_penalties(pre,rules))actual.validators.at(index).balance-=debit;
  for(const auto& [index,v]:expected.validators){
   check(actual.validators.at(index).balance==v.balance,"official balance mismatch: "+name+" index="+std::to_string(index));
   ++balances;if(v.balance!=pre.validators.at(index).balance)++changed;
  }
  // Compare the unmodified official post-state projection. Epoch rotation is
  // separately tested through the validated wrapper, not this component corpus.
  check(actual==expected,"official slashing state mismatch: "+name);++cases;
 }
 check(names==std::set<std::string>{"low_penalty","max_penalties","minimal_penalty","scaled_penalties","slashings_with_random_state"},"incomplete official fixture selection");
 std::cout<<Json{{"scenario","official-phase0-slashings"},{"cases",cases},{"balancesCompared",balances},{"balancesChanged",changed},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
