#include "nano/consensus_capella_bls_change.hpp"
#include "nano/consensus_bls.hpp"
namespace nano {
namespace {
uint64_t number(std::span<const uint8_t> b,size_t at,size_t n){
 uint64_t v=0;for(size_t i=0;i<n;++i)v|=uint64_t(b[at+i])<<(8*i);return v;
}
}
std::optional<SignedCapellaBlsChange> decode_capella_bls_change(std::span<const uint8_t> b){
 if(b.size()!=172)return std::nullopt;
 SignedCapellaBlsChange out;out.validator_index=number(b,0,8);
 std::copy_n(b.begin()+8,48,out.from_bls_pubkey.begin());
 std::copy_n(b.begin()+56,20,out.to_execution_address.begin());
 std::copy_n(b.begin()+76,96,out.signature.begin());return out;
}
Bytes encode_capella_bls_change(const SignedCapellaBlsChange& c){
 Bytes out(172);for(unsigned i=0;i<8;++i)out[i]=uint8_t(c.validator_index>>(8*i));
 std::copy(c.from_bls_pubkey.begin(),c.from_bls_pubkey.end(),out.begin()+8);
 std::copy(c.to_execution_address.begin(),c.to_execution_address.end(),out.begin()+56);
 std::copy(c.signature.begin(),c.signature.end(),out.begin()+76);return out;
}
ConsensusRoot capella_bls_change_root(const SignedCapellaBlsChange& c){
 ConsensusRoot index{},key0{},key1{},address{},zero{};
 for(unsigned i=0;i<8;++i)index[i]=uint8_t(c.validator_index>>(8*i));
 std::copy_n(c.from_bls_pubkey.begin(),32,key0.begin());
 std::copy_n(c.from_bls_pubkey.begin()+32,16,key1.begin());
 std::copy(c.to_execution_address.begin(),c.to_execution_address.end(),address.begin());
 return consensus_signing_root(consensus_signing_root(index,consensus_signing_root(key0,key1)),consensus_signing_root(address,zero));
}
std::optional<CapellaWithdrawalSszCandidate> prepare_capella_bls_changes(
 std::span<const uint8_t> state,const ConsensusRoot& authenticated_root,
 const AltairStateLimits& l,const ExecutionPayloadLimits& execution,size_t budget,
 const ConsensusBytes4& genesis_version,size_t maximum_changes,
 std::span<const SignedCapellaBlsChange> changes,const std::function<void()>& check){
 if(!maximum_changes)throw std::invalid_argument("invalid Capella change limit");
 if(changes.size()>maximum_changes)return std::nullopt;
 if(capella_beacon_state_root(state,l,execution,budget,check)!=authenticated_root)
  throw std::invalid_argument("Capella change state authentication failed");
 // Structural validation above bounds every variable section and these offsets.
 const size_t historical=176+64*l.slots_per_historical_root;
 const size_t start=number(state,historical+88,4),end=number(state,historical+92,4);
 const auto count=(end-start)/121;
 ConsensusRoot genesis{};std::copy_n(state.begin()+8,32,genesis.begin());
 const auto domain=consensus_domain({10,0,0,0},genesis_version,genesis);
 Bytes post(state.begin(),state.end());
 for(const auto& change:changes){
  if(check)check();
  if(change.validator_index>=count)return std::nullopt;
  const auto at=start+121*size_t(change.validator_index)+48;
  if(post[at]!=0)return std::nullopt;
  const auto key_hash=sha256(Bytes(change.from_bls_pubkey.begin(),change.from_bls_pubkey.end()));
  if(!std::equal(key_hash.begin()+1,key_hash.end(),post.begin()+at+1))return std::nullopt;
  const auto signing=consensus_signing_root(capella_bls_change_root(change),domain);
  if(!verify_consensus_bls(change.from_bls_pubkey,signing,change.signature))return std::nullopt;
  if(check)check();
  post[at]=1;std::fill_n(post.begin()+at+1,11,0);
  std::copy(change.to_execution_address.begin(),change.to_execution_address.end(),post.begin()+at+12);
 }
 const auto root=capella_beacon_state_root(post,l,execution,budget,check);
 if(check)check();return CapellaWithdrawalSszCandidate{std::move(post),root};
}
}
