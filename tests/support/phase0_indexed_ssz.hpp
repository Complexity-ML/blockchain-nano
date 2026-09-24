#pragma once
#include "phase0_ffg_ssz.hpp"
namespace test::phase0_minimal {
inline IndexedConsensusAttestation indexed_attestation(const Bytes& b,size_t start,size_t end){
 check(start<=end&&end<=b.size()&&end-start>=228,"indexed attestation bounds");check(little(b,start,4)==228&&(end-start-228)%8==0,"indexed attestation offset");IndexedConsensusAttestation a;auto& d=a.data;d.slot=little(b,start+4,8);d.index=little(b,start+12,8);d.beacon_block_root=root_at(b,start+20);d.source=checkpoint_at(b,start+52);d.target=checkpoint_at(b,start+92);a.signature=Bytes(b.begin()+start+132,b.begin()+start+228);for(size_t at=start+228;at<end;at+=8)a.indices.push_back(little(b,at,8));return a;
}
inline std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation> attester_slashing(const Bytes& b){check(b.size()>=8&&little(b,0,4)==8,"attester slashing offset");const auto second=little(b,4,4);return {indexed_attestation(b,8,second),indexed_attestation(b,second,b.size())};}
}
