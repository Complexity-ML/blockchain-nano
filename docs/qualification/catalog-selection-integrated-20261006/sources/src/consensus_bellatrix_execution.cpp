#include "nano/consensus_bellatrix_execution.hpp"
namespace nano {namespace {
using View=std::span<const uint8_t>;
uint64_t number(View b,size_t at,size_t width){uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
ConsensusRoot root_at(View b,size_t at){ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
}
BellatrixPayloadStateCandidate prepare_bellatrix_payload_state(
 View state,const ConsensusRoot& authenticated_root,const AltairStateLimits& s,
 const ExecutionPayloadLimits& e,size_t state_budget,uint64_t slots_per_epoch,uint64_t seconds_per_slot,
 View payload,size_t payload_budget,const Header& parent,const State& execution_state,
 const Int& chain_id,ReplayPolicy replay,const std::function<Bytes(const Int&)>& history,
 const std::function<void()>& check){
 if(!slots_per_epoch||!seconds_per_slot)throw std::invalid_argument("invalid Bellatrix clock rules");
 if(bellatrix_beacon_state_root(state,s,e,state_budget,check)!=authenticated_root)
  throw std::invalid_argument("Bellatrix state authentication failed");
 // These sizes and offsets have just passed the complete SSZ schema validator.
 const size_t randao=272+64*s.slots_per_historical_root;
 const size_t previous=randao+32*s.epochs_per_historical_vector+8*s.epochs_per_slashings_vector;
 const size_t header_offset=number(state,previous+229+96*s.sync_committee_size,4);
 const auto latest=decode_bellatrix_execution_header(state.subspan(header_offset),e.extra_data_bytes,state.size()-header_offset,check);
 if(!latest||Bytes(latest->block_hash.begin(),latest->block_hash.end())!=parent.hash())
  throw std::invalid_argument("Beacon execution parent mismatch or merge not completed");
 const uint64_t slot=number(state,40,8),genesis=number(state,0,8);
 if(slot>(UINT64_MAX-genesis)/seconds_per_slot)throw std::invalid_argument("Bellatrix timestamp overflow");
 const uint64_t timestamp=genesis+slot*seconds_per_slot;
 const auto mix=root_at(state,randao+32*((slot/slots_per_epoch)%s.epochs_per_historical_vector));
 auto candidate=prepare_bellatrix_execution(payload,e,payload_budget,parent,execution_state,timestamp,mix,chain_id,replay,history,check);
 const auto decoded=decode_bellatrix_execution_payload(payload,e,payload_budget,check);
 const auto header=bellatrix_execution_payload_header(*decoded,e,check);
 const auto encoded=encode_bellatrix_execution_header(header,e.extra_data_bytes,state_budget,check);
 if(header_offset>state_budget||encoded.size()>state_budget-header_offset||encoded.size()>UINT32_MAX-header_offset)
  throw std::invalid_argument("Bellatrix post-state budget");
 Bytes post(state.begin(),state.begin()+header_offset);post.insert(post.end(),encoded.begin(),encoded.end());
 const auto post_root=bellatrix_beacon_state_root(post,s,e,state_budget,check);
 if(check)check();
 return {std::move(post),post_root,std::move(candidate)};
}
}
