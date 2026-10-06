#include "nano/consensus_beacon_header.hpp"
#include "nano/consensus_bls.hpp"
namespace nano {
namespace {
ConsensusRoot integer_root(uint64_t value){ConsensusRoot root{};for(unsigned i=0;i<8;++i)root[i]=static_cast<uint8_t>(value>>(8*i));return root;}
}
ConsensusRoot consensus_beacon_header_root(const ConsensusBeaconHeader& h){
 std::array<ConsensusRoot,8> nodes{integer_root(h.slot),integer_root(h.proposer_index),h.parent_root,h.state_root,h.body_root,{},{},{}};
 for(size_t width=8;width>1;width/=2)for(size_t i=0;i<width/2;++i)nodes[i]=consensus_signing_root(nodes[2*i],nodes[2*i+1]);
 return nodes[0];
}
bool verify_consensus_beacon_header(const ConsensusBeaconHeader& h,const ConsensusBytes4& version,const ConsensusRoot& genesis,std::span<const uint8_t> key,std::span<const uint8_t> signature){
 const auto root=consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},version,genesis));
 return verify_consensus_bls(key,root,signature);
}
}
