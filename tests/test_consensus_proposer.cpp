#include "support/process.hpp"
#include "nano/consensus_proposer.hpp"
#include "nano/committee.hpp"
using namespace test;
namespace {
uint64_t reference(const std::vector<Phase0ProposerValidator>& registry,const ConsensusRandaoState& state,const ConsensusRandaoRules& rules,uint64_t slot,uint64_t maximum){
 auto base=consensus_randao_seed(state,rules,state.epoch,{0,0,0,0});Bytes input(base.begin(),base.end());for(unsigned i=0;i<8;++i)input.push_back(uint8_t(slot>>(8*i)));
 auto raw=sha256(input);ConsensusRoot seed{};std::copy(raw.begin(),raw.end(),seed.begin());
 std::vector<Phase0ProposerValidator> active;for(auto v:registry)if(v.activation_epoch<=state.epoch&&state.epoch<v.exit_epoch)active.push_back(v);
 auto mapping=shuffled_validator_indices(active.size(),seed,10);
 for(uint64_t i=0;i<100000;++i){auto v=active[mapping[i%active.size()]];Bytes data(seed.begin(),seed.end());for(unsigned b=0;b<8;++b)data.push_back(uint8_t((i/32)>>(8*b)));
  auto random=sha256(data)[i%32];if(Int(std::to_string(v.effective_balance))*255>=Int(std::to_string(maximum))*random)return v.index;
 }
 throw std::runtime_error("bounded reference exhausted");
}
}
int main(){try{
 ConsensusRandaoState state;state.epoch=2;state.mixes.resize(8);ConsensusRandaoRules rules{8,1};
 std::vector<Phase0ProposerValidator> registry{{2,0,0,UINT64_MAX},{7,1,0,UINT64_MAX},{9,16,0,UINT64_MAX},{20,32,3,UINT64_MAX},{30,32,0,2}};
 unsigned comparisons=0,rejected=0;
 for(uint8_t seed=0;seed<32;++seed){state.mixes[0].fill(seed);for(uint64_t slot:{16,17,23}){auto expected=reference(registry,state,rules,slot,32);check(phase0_proposer_index(registry,state,rules,slot,32,10)==expected,"weighted reference mismatch");++comparisons;}}
 auto zero=registry;for(auto& v:zero)v.effective_balance=0;
 check(phase0_proposer_index(zero,state,rules,16,32,10)==reference(zero,state,rules,16,32),"zero-balance random byte boundary");++comparisons;
 auto wide=registry;wide[0].effective_balance=UINT64_MAX;wide[1].effective_balance=UINT64_MAX/2;wide[2].effective_balance=UINT64_MAX/3;
 check(phase0_proposer_index(wide,state,rules,16,UINT64_MAX,10)==reference(wide,state,rules,16,UINT64_MAX),"wide balance multiplication");++comparisons;
 auto refuse=[&](auto f){bool failed=false;try{f();}catch(const std::invalid_argument&){failed=true;}check(failed,"invalid proposer input accepted");++rejected;};
 refuse([&]{phase0_proposer_index({},state,rules,16,32,10);});
 auto bad=registry;bad[1].index=2;refuse([&]{phase0_proposer_index(bad,state,rules,16,32,10);});
 bad=registry;bad[0].effective_balance=33;refuse([&]{phase0_proposer_index(bad,state,rules,16,32,10);});
 bad=registry;bad[0].activation_epoch=4;bad[0].exit_epoch=3;refuse([&]{phase0_proposer_index(bad,state,rules,16,32,10);});
 bad=registry;for(auto& v:bad)v.activation_epoch=v.exit_epoch=3;refuse([&]{phase0_proposer_index(bad,state,rules,16,32,10);});
 refuse([&]{phase0_proposer_index(registry,state,rules,24,32,10);});
 refuse([&]{phase0_proposer_index(registry,state,rules,16,0,10);});
 refuse([&]{phase0_proposer_index(registry,state,rules,16,32,0);});
 refuse([&]{shuffled_validator_index(1,1,{},10);});refuse([&]{shuffled_validator_index(0,0,{},10);});
 bool cancelled=false;auto before=state;try{phase0_proposer_index(registry,state,rules,16,32,10,[]{throw std::runtime_error("cancel");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled&&state==before,"cancellation isolation");
 for(uint64_t count:{100000,1000000}){
  std::vector<Phase0ProposerValidator> large;large.reserve(count);for(uint64_t i=0;i<count;++i)large.push_back({i,32,0,UINT64_MAX});
  auto started=std::chrono::steady_clock::now();auto selected=phase0_proposer_index(large,state,rules,16,32,90);
  check(selected<count,"large registry selected outside set");
  std::cout<<Json{{"scenario","proposer-registry-scale"},{"validators",count},{"selected",selected},{"milliseconds",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()},{"distributedNetwork",false}}.dump()<<std::endl;
 }
 std::cout<<Json{{"scenario","phase0-proposer-selection"},{"referenceComparisons",comparisons},{"rejected",rejected},{"cancellation",cancelled},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
