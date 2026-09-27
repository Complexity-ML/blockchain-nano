#include "nano/consensus_capella_fork.hpp"
#include "consensus_bellatrix_projection.hpp"
#include "consensus_capella_projection.hpp"
namespace nano {
CapellaSlotState upgrade_capella_state(const BellatrixSlotState& input,const ConsensusRoot& root,
 const AltairStateLimits& l,const ExecutionPayloadLimits& e,size_t budget,uint64_t slots,
 const ConsensusBytes4& version,const std::function<void()>& check){
 if(!slots)throw std::invalid_argument("invalid Capella epoch length");
 if(bellatrix_beacon_state_root(input.ssz,l,e,budget,check)!=root)throw std::invalid_argument("Capella upgrade parent authentication failed");
 auto projected=bellatrix_detail::project(input.ssz,l,e,budget,check);
 const auto previous=decode_bellatrix_execution_header(projected.execution_header,e.extra_data_bytes,budget,check);
 if(!previous)throw std::invalid_argument("invalid Bellatrix execution header");
 CapellaExecutionPayloadHeader next;static_cast<BellatrixExecutionPayloadHeader&>(next)=*previous;
 // Fork upgrade uses a zero Root, not the SSZ root of an empty withdrawal list.
 const auto header=encode_capella_execution_header(next,e.extra_data_bytes,budget,check);
 auto& common=projected.common;uint64_t slot=0;for(unsigned i=0;i<8;++i)slot|=uint64_t(common[40+i])<<(8*i);
 std::copy_n(common.begin()+52,4,common.begin()+48);std::copy(version.begin(),version.end(),common.begin()+52);
 for(unsigned i=0;i<8;++i)common[56+i]=uint8_t((slot/slots)>>(8*i));
 auto post=capella_detail::restore(common,header,Bytes(16),{},l,e,budget,check);
 if(check)check();return {std::move(post),input.last_processed_epoch};
}
CapellaSlotState prepare_capella_fork_slots(const BellatrixSlotState& input,const ConsensusRoot& root,
 const BellatrixEpochRules& rules,const AltairStateLimits& limits,const CapellaForkSchedule& schedule,
 uint64_t target,uint64_t maximum_slots,uint64_t trials,const std::function<void()>& check){
 validate_altair_epoch_rules(rules.operations,limits);
 const auto slots=rules.operations.ffg.slots_per_epoch;
 if(!slots||schedule.epoch>UINT64_MAX/slots||schedule.bellatrix_version==schedule.capella_version)throw std::invalid_argument("invalid Capella fork schedule");
 if(bellatrix_beacon_state_root(input.ssz,limits,rules.execution,rules.maximum_state_bytes,check)!=root)throw std::invalid_argument("Capella fork parent authentication failed");
 // The exact-boundary path does not call prepare_bellatrix_slots; retain its
 // semantic checks even when no Bellatrix slot needs to be processed.
 const auto number=[&](size_t at,unsigned width=8){uint64_t n=0;for(unsigned i=0;i<width;++i)n|=uint64_t(input.ssz.at(at+i))<<(8*i);return n;};
 const auto fields=capella_detail::offsets(limits);
 const auto validators=number(fields[2],4),balances=number(fields[3],4),previous=number(fields[4],4),current=number(fields[5],4),scores=number(fields[6],4),end=number(capella_detail::execution_field(limits),4),count=(balances-validators)/121;
 if(!rules.execution.extra_data_bytes||!rules.execution.transaction_bytes||!rules.execution.transactions||rules.execution.transactions>UINT32_MAX/4||number(64)>number(40)||(previous-balances)/8!=count||current-previous!=count||scores-current!=count||(end-scores)/8!=count)throw std::invalid_argument("incoherent Bellatrix fork input");
 uint64_t start=0,fork_epoch=0;for(unsigned i=0;i<8;++i){start|=uint64_t(input.ssz[40+i])<<(8*i);fork_epoch|=uint64_t(input.ssz[56+i])<<(8*i);}
 const uint64_t boundary=schedule.epoch*slots;
 if(start>boundary||target<boundary||target<start||fork_epoch>start/slots||!std::equal(schedule.bellatrix_version.begin(),schedule.bellatrix_version.end(),input.ssz.begin()+52))throw std::invalid_argument("state outside scheduled Capella boundary");
 if(target-start>maximum_slots)throw std::length_error("Capella fork slot budget exceeded");
 if(input.last_processed_epoch&&(!(start/slots)||*input.last_processed_epoch!=start/slots-1))throw std::invalid_argument("invalid fork epoch marker");
 auto at=input;if(start<boundary)at=prepare_bellatrix_slots(input,rules,limits,boundary,boundary-start,trials,check);
 const auto at_root=bellatrix_beacon_state_root(at.ssz,limits,rules.execution,rules.maximum_state_bytes,check);
 auto upgraded=upgrade_capella_state(at,at_root,limits,rules.execution,rules.maximum_state_bytes,slots,schedule.capella_version,check);
 if(target>boundary)upgraded=prepare_capella_slots(upgraded,{rules.operations,rules.execution,rules.maximum_state_bytes},limits,target,target-boundary,trials,check);
 if(check)check();return upgraded;
}
}
