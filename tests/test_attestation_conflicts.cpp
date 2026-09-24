#include "support/process.hpp"
#include "nano/consensus_attestation.hpp"
using namespace test;
int main(){try{
 ConsensusAttestationData a;a.slot=96;a.index=2;a.source.epoch=1;a.target.epoch=3;a.beacon_block_root[0]=7;
 unsigned checked=0;auto expect=[&](const auto& first,const auto& second,bool wanted){check(is_slashable_attestation_data(first,second)==wanted,"attestation conflict predicate");++checked;};
 expect(a,a,false);
 // At equal target epoch, changing any other signed field is a double vote.
 for(unsigned field=0;field<6;++field){auto b=a;switch(field){case 0:b.slot++;break;case 1:b.index++;break;case 2:b.beacon_block_root[0]^=1;break;case 3:b.source.epoch++;break;case 4:b.source.root[0]^=1;break;case 5:b.target.root[0]^=1;break;}expect(a,b,true);expect(b,a,true);}
 auto b=a;b.source.epoch=2;b.target.epoch=4;expect(a,b,false);expect(b,a,false); // overlapping, not surrounding
 b=a;b.source.epoch=2;b.target.epoch=2;expect(a,b,true);expect(b,a,false); // directional surround
 b=a;b.target.epoch=2;expect(a,b,false); // equal source is not strict surround
 b=a;b.source.epoch=0;b.target.epoch=4;expect(b,a,true);expect(a,b,false);
 a.source.epoch=0;a.target.epoch=UINT64_MAX;b=a;b.source.epoch=1;b.target.epoch=UINT64_MAX-1;expect(a,b,true);expect(b,a,false);
 b=a;b.source.epoch=UINT64_MAX;b.target.epoch=UINT64_MAX;expect(a,b,true); // equal target is double vote, no arithmetic overflow
 b=a;b.source.root[31]^=1;expect(a,b,true);
 std::cout<<Json{{"scenario","attestation-conflicts"},{"cases",checked},{"signaturesAuthenticated",false},{"penaltiesApplied",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
