#pragma once
#include "phase0_minimal_ssz.hpp"
#include "nano/consensus_ffg.hpp"
#include <bit>
namespace test::phase0_minimal {
inline ConsensusRoot root_at(const Bytes& b,size_t at){check(at<=b.size()&&b.size()-at>=32,"SSZ root bounds");ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
inline ConsensusCheckpoint checkpoint_at(const Bytes& b,size_t at){return {little(b,at,8),root_at(b,at+8)};}
inline std::vector<Phase0PendingAttestation> pending(const Bytes& b,size_t begin,size_t end){
 check(begin<=end&&end<=b.size(),"pending list bounds");if(begin==end)return {};
 const auto first=little(b,begin,4);check(first>=4&&first%4==0&&first<=end-begin,"pending list offset");std::vector<Phase0PendingAttestation> result;
 for(size_t i=0;i<first/4;++i){const auto start=begin+little(b,begin+4*i,4),stop=i+1==first/4?end:begin+little(b,begin+4*(i+1),4);check(start<=stop&&stop<=end&&stop-start>=149,"pending record bounds");check(little(b,start,4)==148&&b[stop-1]!=0,"pending bitlist encoding");
  Phase0PendingAttestation vote;auto& d=vote.data;d.slot=little(b,start+4,8);d.index=little(b,start+12,8);d.beacon_block_root=root_at(b,start+20);d.source=checkpoint_at(b,start+52);d.target=checkpoint_at(b,start+92);
  const auto bits=(stop-start-149)*8+std::bit_width(unsigned(b[stop-1]))-1;check(bits<=2048,"minimal pending committee limit");for(size_t k=0;k<bits;++k)vote.aggregation_bits.push_back(bool((b[start+148+k/8]>>(k%8))&1));result.push_back(std::move(vote));
 }return result;
}
inline Phase0FfgState ffg_state(const Bytes& b){
 check(b.size()>=7057,"short FFG state");Phase0FfgState s;s.slot=little(b,40,8);s.checkpoints={b[6936],checkpoint_at(b,6937),checkpoint_at(b,6977),checkpoint_at(b,7017)};
 for(size_t i=0;i<64;++i)s.block_roots.push_back(root_at(b,176+32*i));const auto previous=little(b,6928,4),current=little(b,6932,4);s.previous_attestations=pending(b,previous,current);s.current_attestations=pending(b,current,b.size());return s;
}
inline ConsensusRandaoState ffg_randao(const Bytes& b){ConsensusRandaoState r;r.epoch=little(b,40,8)/8;for(size_t i=0;i<64;++i)r.mixes.push_back(root_at(b,4368+32*i));r.last_reveal_slot=little(b,64,8);return r;}
}
