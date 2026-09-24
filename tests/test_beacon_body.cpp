#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_beacon_body.hpp"
#include <random>
using namespace test;
using namespace test::phase0_minimal;
namespace {
std::string root_hex(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
void put(Bytes& b,size_t at,uint32_t n){for(unsigned j=0;j<4;++j)b.at(at+j)=uint8_t(n>>(8*j));}
Bytes body(const std::array<Bytes,5>& lists){Bytes b(220);size_t start=220;for(size_t i=0;i<5;++i){put(b,200+4*i,start);b.insert(b.end(),lists[i].begin(),lists[i].end());start+=lists[i].size();}return b;}
Bytes indexed(size_t count){Bytes b(228+8*count);put(b,0,228);return b;}
Bytes attestation(const Bytes& bits){Bytes b(228);put(b,0,228);b.insert(b.end(),bits.begin(),bits.end());return b;}
Bytes variable(const std::vector<Bytes>& values){Bytes result(4*values.size());size_t start=result.size();for(size_t i=0;i<values.size();++i){put(result,4*i,start);result.insert(result.end(),values[i].begin(),values[i].end());start+=values[i].size();}return result;}
}
int main(int argc,char** argv){try{
 const fs::path fixtures=argc==2?fs::path(argv[1]):fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-beacon-body";
 const Phase0BodyLimits limits{16,2,128,16,16,2048};const auto manifest=load(fixtures/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","fixture revision");
 unsigned official=0,mutations=0,rejected=0,boundaries=0,byte_mutations=0;std::mt19937 random(20260924);std::array<size_t,5> operations{};
 for(const auto& entry:manifest.at("cases")){
  for(const auto& [path,info]:entry.at("files").items()){auto b=binary(fixtures/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture hash mismatch");}
  auto b=binary(fixtures/entry.at("file").get<std::string>());auto expected=entry.at("root").get<std::string>();check(root_hex(phase0_beacon_body_root(b,limits))==expected,"official body root mismatch");++official;
  for(unsigned i=0;i<64;++i){auto changed=b;changed[random()%changed.size()]^=uint8_t(1U<<(random()%8));try{check(root_hex(phase0_beacon_body_root(changed,limits))!=expected,"mutated bytes retain root");}catch(const std::invalid_argument&){}++byte_mutations;}
  for(size_t field=0;field<5;++field){size_t begin=little(b,200+4*field,4),end=field==4?b.size():little(b,204+4*field,4);size_t count=field==0?(end-begin)/416:field==3?(end-begin)/1240:field==4?(end-begin)/112:begin==end?0:little(b,begin,4)/4;operations[field]+=count;}
  for(size_t at: {size_t(0),size_t(96),size_t(128),size_t(136),size_t(168)}){auto changed=b;changed[at]^=1;check(root_hex(phase0_beacon_body_root(changed,limits))!=expected,"uncommitted fixed field");++mutations;}
 }
 for(auto count:operations)check(count>0,"official corpus must exercise each operation kind");
 auto refuse=[&](const Bytes& bytes,const Phase0BodyLimits& rules){bool threw=false;try{phase0_beacon_body_root(bytes,rules);}catch(const std::invalid_argument&){threw=true;}check(threw,"malformed body accepted");++rejected;};
 auto empty=body({});const auto empty_root=phase0_beacon_body_root(empty,limits);++boundaries;
 for(size_t n=0;n<220;++n)refuse(Bytes(empty.begin(),empty.begin()+n),limits);
 for(size_t at=200;at<220;at+=4)for(uint32_t value:{0U,219U,221U,UINT32_MAX}){auto b=empty;put(b,at,value);refuse(b,limits);}
 auto zero=limits;zero.attestations=0;refuse(empty,zero);
 for(size_t kind:{size_t(0),size_t(3),size_t(4)}){
  const size_t width=kind==0?416:kind==3?1240:112;std::array<Bytes,5> lists{};lists[kind]=Bytes(width*16);phase0_beacon_body_root(body(lists),limits);++boundaries;
  lists[kind].push_back(0);refuse(body(lists),limits);lists[kind]=Bytes(width*17);refuse(body(lists),limits);
 }
 const auto slash=variable({indexed(2048),indexed(2048)});
 std::array<Bytes,5> lists{};lists[1]=variable({slash,slash});phase0_beacon_body_root(body(lists),limits);++boundaries;
 lists[1]=variable({slash,slash,slash});refuse(body(lists),limits);
 lists[1]=variable({variable({indexed(2049),indexed(0)})});refuse(body(lists),limits);
 lists[1]=variable({variable({indexed(0),indexed(0)})});phase0_beacon_body_root(body(lists),limits);++boundaries; // structurally valid, semantically invalid indices deferred
 auto bad=indexed(0);bad.push_back(0);lists[1]=variable({variable({bad,indexed(0)})});refuse(body(lists),limits);
 lists={};Bytes bits(257);bits.back()=1;const auto full=attestation(bits);lists[2]=variable(std::vector<Bytes>(128,full));phase0_beacon_body_root(body(lists),limits);++boundaries;
 lists[2]=variable(std::vector<Bytes>(129,full));refuse(body(lists),limits);
 for(Bytes invalid:{Bytes{},Bytes{0},Bytes(257,2),Bytes(258,1)}){lists[2]=variable({attestation(invalid)});refuse(body(lists),limits);}
 for(size_t n:{size_t(0),size_t(7),size_t(8),size_t(255),size_t(256),size_t(2048)}){Bytes bits(n/8+1);bits.back()=1U<<(n%8);lists[2]=variable({attestation(bits)});check(phase0_beacon_body_root(body(lists),limits)!=empty_root,"list length not committed");++boundaries;}
 lists={};for(Bytes invalid:{Bytes{1,0,0,0},Bytes{5,0,0,0,0},Bytes{8,0,0,0},Bytes{4,0,0,0}}){lists[2]=invalid;refuse(body(lists),limits);}
 // Nested offsets must be canonical too, not just the outer list.
 auto bad_att=attestation({1});put(bad_att,0,227);lists[2]=variable({bad_att});refuse(body(lists),limits);
 lists={};auto bad_slash=slash;put(bad_slash,4,7);lists[1]=variable({bad_slash});refuse(body(lists),limits);
 check(official==5,"official corpus count");
 std::cout<<Json{{"scenario","phase0-beacon-body"},{"officialRoots",official},{"fixedFieldMutations",mutations},{"operationCounts",operations},{"malformedRejected",rejected},{"boundaries",boundaries},{"byteMutations",byte_mutations},{"operationSemanticsValidated",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
