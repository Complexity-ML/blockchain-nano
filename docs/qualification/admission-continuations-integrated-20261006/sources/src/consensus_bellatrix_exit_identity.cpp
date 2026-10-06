#include "nano/consensus_bellatrix_reactor.hpp"
namespace nano {
BellatrixExitIdentity BellatrixSignedExitRecord::identity()const{
 if(ssz.size()!=112)throw std::invalid_argument("invalid archived exit width");
 uint64_t index=0;for(unsigned i=0;i<8;++i)index|=uint64_t(ssz[8+i])<<(8*i);return {index,public_key};
}
BellatrixExitMembership bellatrix_exit_membership(const BellatrixSignedExitRecord& exit,const ConsensusSlashingState& registry){
 const auto id=exit.identity();const auto member=registry.validators.find(id.index);
 if(member==registry.validators.end())return BellatrixExitMembership::Unknown;
 if(member->second.public_key!=id.public_key)return BellatrixExitMembership::DifferentKey;
 return member->second.exit_epoch==UINT64_MAX?BellatrixExitMembership::Matching:BellatrixExitMembership::Scheduled;
}
}
