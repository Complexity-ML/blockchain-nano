#include "nano/consensus_attestation.hpp"
namespace nano {
namespace {
ConsensusRoot uint64_root(uint64_t value){ConsensusRoot result{};for(unsigned i=0;i<8;++i)result[i]=static_cast<uint8_t>(value>>(8*i));return result;}
}
bool is_slashable_attestation_data(const ConsensusAttestationData& first,const ConsensusAttestationData& second){
 return (first!=second&&first.target.epoch==second.target.epoch)||
        (first.source.epoch<second.source.epoch&&second.target.epoch<first.target.epoch);
}
ConsensusRoot consensus_checkpoint_root(const ConsensusCheckpoint& checkpoint){
 return consensus_signing_root(uint64_root(checkpoint.epoch),checkpoint.root);
}
ConsensusRoot consensus_attestation_root(const ConsensusAttestationData& data){
 // A five-field SSZ container is padded to eight leaves, without length mixing.
 std::array<ConsensusRoot,8> nodes{uint64_root(data.slot),uint64_root(data.index),data.beacon_block_root,
  consensus_checkpoint_root(data.source),consensus_checkpoint_root(data.target),ConsensusRoot{},ConsensusRoot{},ConsensusRoot{}};
 for(std::size_t width=8;width>1;width/=2)for(std::size_t i=0;i<width/2;++i)nodes[i]=consensus_signing_root(nodes[2*i],nodes[2*i+1]);
 return nodes[0];
}
}
