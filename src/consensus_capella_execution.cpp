#include "nano/consensus_capella_execution.hpp"
namespace nano {namespace {
uint64_t number(std::span<const uint8_t> b,size_t at,size_t width=8){uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
ConsensusRoot root_at(std::span<const uint8_t> b,size_t at){ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
}
BellatrixPayloadStateCandidate prepare_capella_payload_state(
 std::span<const uint8_t> state,const ConsensusRoot& authenticated_root,
 const AltairStateLimits& s,const ExecutionPayloadLimits& e,size_t state_budget,
 uint64_t slots,uint64_t seconds,const CapellaWithdrawalRules& rules,
 std::span<const uint8_t> payload,size_t payload_budget,
 const Header& parent,const State& execution_state,const Int& chain_id,
 ReplayPolicy replay,const std::function<Bytes(const Int&)>& history,const std::function<void()>& check){
 if(!slots||!seconds)throw std::invalid_argument("invalid Capella clock rules");
 if(capella_beacon_state_root(state,s,e,state_budget,check)!=authenticated_root)throw std::invalid_argument("Capella state authentication failed");
 const size_t randao=272+64*s.slots_per_historical_root;
 const size_t previous=randao+32*s.epochs_per_historical_vector+8*s.epochs_per_slashings_vector;
 const size_t field=previous+229+96*s.sync_committee_size;
 const size_t header_at=number(state,field,4),summaries=number(state,field+20,4);
 const auto latest=decode_capella_execution_header(state.subspan(header_at,summaries-header_at),e.extra_data_bytes,state_budget,check);
 if(!latest||Bytes(latest->block_hash.begin(),latest->block_hash.end())!=parent.hash())throw std::invalid_argument("Capella beacon execution parent mismatch");
 // A migrated Paris header has the zero SSZ withdrawal root. After the first
 // Capella payload it holds a real list root, including for an empty list.
 if(!parent.withdrawals_root&&latest->withdrawals_root!=ConsensusRoot{})throw std::invalid_argument("Paris parent without migrated Capella header");
 const uint64_t slot=number(state,40),genesis=number(state,0);
 if(slot>(UINT64_MAX-genesis)/seconds)throw std::invalid_argument("Capella timestamp overflow");
 const auto mix=root_at(state,randao+32*((slot/slots)%s.epochs_per_historical_vector));
 const auto decoded=decode_capella_execution_payload(payload,e,rules.maximum_per_payload,payload_budget,check);
 if(!decoded)throw std::invalid_argument("invalid Capella payload");
 auto debited=prepare_capella_withdrawal_state(state,authenticated_root,s,e,state_budget,slots,rules,decoded->withdrawals,check);
 if(!debited)throw std::invalid_argument("unauthorized Capella withdrawals");
 auto execution=prepare_capella_execution(payload,e,rules.maximum_per_payload,payload_budget,parent,execution_state,genesis+slot*seconds,mix,chain_id,replay,history,check);
 const auto header=encode_capella_execution_header(capella_execution_payload_header(*decoded,e,rules.maximum_per_payload,check),e.extra_data_bytes,state_budget,check);
 const auto tail=state.size()-summaries;
 if(header_at>state_budget||header.size()>state_budget-header_at||tail>state_budget-header_at-header.size()||header_at+header.size()+tail>UINT32_MAX)throw std::invalid_argument("Capella post-state budget");
 auto& post=debited->state;post.resize(header_at);post.insert(post.end(),header.begin(),header.end());post.insert(post.end(),state.begin()+summaries,state.end());
 const auto new_offset=header_at+header.size();for(unsigned i=0;i<4;++i)post[field+20+i]=uint8_t(new_offset>>(8*i));
 const auto root=capella_beacon_state_root(post,s,e,state_budget,check);
 if(check)check();return {std::move(post),root,std::move(execution)};
}
}
