#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_snapshot.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto rules=altair_transition_rules(fixtures);const auto encoded=altair_transition_profile(rules);
 const auto file=encoded.dump(2);const auto digest=sha256(Bytes(file.begin(),file.end()));
 const auto decoded=load_altair_transition_profile(file,digest,file.size());
 check(altair_transition_profile(decoded)==encoded,"profile round trip changed rules");
 unsigned rejected=0,quantities=0;
 auto refuse=[&](const Json& value){bool failed=false;try{altair_transition_from_profile(value);}catch(const std::exception&){failed=true;}check(failed,"malformed profile accepted");++rejected;};
 for(const auto& [key,value]:encoded.items()){
  auto missing=encoded;missing.erase(key);refuse(missing);
  if(value.is_array()){
   auto extra=encoded;extra[key].push_back("0x0");refuse(extra);
   for(size_t i=0;i<value.size();++i){
    for(const Json bad:{Json(1),Json(true),Json("0x00"),Json("-1"),Json("0x10000000000000000")}){auto altered=encoded;altered[key][i]=bad;refuse(altered);}
    ++quantities;
   }
  }
 }
 auto refuse_file=[&](const std::string& text,const Bytes& hash,size_t limit){bool failed=false;try{load_altair_transition_profile(text,hash,limit);}catch(const std::exception&){failed=true;}check(failed,"invalid profile file accepted");++rejected;};
 refuse_file(file,digest,file.size()-1);refuse_file(file,Bytes(31,0),file.size());
 auto wrong_digest=digest;wrong_digest[0]^=1;refuse_file(file,wrong_digest,file.size());
 refuse_file("invalid JSON",digest,file.size());
 const auto duplicate=std::string("{\"ffg\":")+encoded.at("ffg").dump()+","+encoded.dump().substr(1);
 refuse_file(duplicate,sha256(Bytes(duplicate.begin(),duplicate.end())),duplicate.size());
 auto extra=encoded;extra["unexpected"]=true;refuse(extra);
 auto overflow=encoded;overflow["duties"][6]="0x10000";refuse(overflow);
 auto incompatible=encoded;incompatible["ffg"][0]="0x9";refuse(incompatible);
 incompatible=encoded;incompatible["history"][0]="0x9";refuse(incompatible);
 for(const auto* bad:{"0x0000000","0x0000000000","0x0000000A"}){auto fork=encoded;fork["genesisForkVersion"]=bad;refuse(fork);}
 const auto corpus=fixtures/"consensus-altair-full-blocks-official",dir=corpus/"attestation";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="attestation")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"profile fixture digest");
 AltairSlotState state{binary(dir/"pre.ssz"),std::nullopt};const auto epoch=little(state.ssz,40,8)/decoded.epoch.history.slots_per_epoch;if(epoch)state.last_processed_epoch=epoch-1;
 check(altair_slot_snapshot(state,decoded)==altair_slot_snapshot(state,rules),"profile migration changed snapshot bytes");
 for(unsigned i=0;i<2;++i){auto next=prepare_altair_state_transition(state,binary(dir/("blocks_"+std::to_string(i)+".ssz")),decoded,64,100000);check(bool(next),"loaded profile rejects official block");state=std::move(*next);}
 check(state.ssz==binary(dir/"post.ssz"),"loaded profile official post-state mismatch");
 std::cout<<Json{{"scenario","altair-explicit-profile"},{"quantities",quantities},{"rejected",rejected},{"officialBlocks",2},{"networkActive",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
