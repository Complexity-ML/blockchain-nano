#include "../src/consensus_capella_audit_ancestry.hpp"
#include <iostream>
using namespace nano;
static void check(bool ok){if(!ok)throw std::runtime_error("ancestry assertion failed");}
static ConsensusRoot root(unsigned n){ConsensusRoot r{};r[0]=n;return r;}
int main(){try{
 CapellaForkStore replay;for(auto [id,slot,parent]:{std::tuple{1u,0u,0u},{2u,7u,1u},{3u,8u,2u},{4u,17u,3u},{5u,24u,4u}}){CapellaForkBlock block{};block.header.slot=slot;block.header.parent_root=root(parent);replay.blocks.emplace(root(id),std::move(block));}
 const ConsensusCheckpoint finalized{3,root(5)};unsigned passed=0;
 auto accept=[&](ConsensusCheckpoint observed){audit_detail::require_finalized_ancestor(replay,finalized,observed,8);++passed;};
 auto reject=[&](ConsensusCheckpoint observed,uint64_t slots=8){bool refused=false;try{audit_detail::require_finalized_ancestor(replay,finalized,observed,slots);}catch(const std::invalid_argument&){refused=true;}check(refused);++passed;};
 accept({3,root(5)});accept({2,root(3)});accept({1,root(3)});accept({0,root(1)});
 reject({4,root(5)});reject({2,root(4)});reject({1,root(2)});reject({2,root(99)});reject({1,root(3)},0);
 {auto original=replay.blocks.at(root(4)).header;replay.blocks.at(root(4)).header.parent_root=root(4);reject({1,root(3)});replay.blocks.at(root(4)).header=original;}
 {auto original=replay.blocks.at(root(4)).header;replay.blocks.at(root(4)).header.parent_root=root(99);reject({1,root(3)});replay.blocks.at(root(4)).header=original;}
 bool cancelled=false;try{audit_detail::require_finalized_ancestor(replay,finalized,{1,root(3)},8,[]{throw ExecutionCancelled("cancel audit");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled);++passed;
 check(passed==12);std::cout<<passed<<" actual ancestry-helper cases passed; no signature or full replay qualification\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
