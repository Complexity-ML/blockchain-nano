#include "support/process.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include <set>
namespace nano {
// Exercise precisely the durable reservation functions used by the signer.
// Upstream vectors contain signing roots, not headers or private keys.
struct BeaconSigningJournalFixtureAccess {
 static void block(BeaconSigningJournal& j,uint64_t slot,const ConsensusRoot& root){std::lock_guard lock(j.mutex_);j.reserve_proposal(slot,root);}
 static void vote(BeaconSigningJournal& j,uint64_t source,uint64_t target,const ConsensusRoot& root){std::lock_guard lock(j.mutex_);j.reserve_attestation(source,target,root);}
};
}
using namespace test;
namespace {
ConsensusRoot root(const Json& v){auto bytes=unhex(v.get<std::string>());check(bytes.size()==32,"fixture root");ConsensusRoot out{};std::copy(bytes.begin(),bytes.end(),out.begin());return out;}
uint64_t number(const Json& v){const auto text=v.get<std::string>();size_t consumed=0;auto n=std::stoull(text,&consumed);check(consumed==text.size(),"fixture number");return n;}
struct FixtureValidator {
 ChainStore disk;std::unique_ptr<BeaconSigningJournal> journal;ConsensusRoot genesis;Bytes key;
 FixtureValidator(const fs::path& path,ConsensusRoot g,Bytes k):disk(path),genesis(g),key(std::move(k)){journal=std::make_unique<BeaconSigningJournal>(disk,1337,Bytes(32,9),genesis,key,8);}
 void reopen(){const auto token=journal->commitment();journal.reset();journal=std::make_unique<BeaconSigningJournal>(disk,1337,Bytes(32,9),genesis,key,8,token);}
};
}
int main(int argc,char** argv){try{
 check(argc==2,"usage: nano-beacon-interchange-tests /path/to/upstream-repository");
 const fs::path upstream=argv[1];const auto manifest=load(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/slashing-interchange-manifest.json");
 check(manifest.at("files").size()==38,"incomplete pinned corpus");
 unsigned cases=0,imports=0,attempts=0,accepted=0,refused=0,optional_repeats=0,slashable_imports_refused=0,reopens=0;
 for(const auto& [file,digest]:manifest.at("files").items()){
  const auto text=read(upstream/file);check(hex(sha256(Bytes(text.begin(),text.end())))==digest.get<std::string>(),"fixture hash changed: "+file);const auto doc=Json::parse(text);const auto name=doc.at("name").get<std::string>();
  Temp temp;std::set<std::string> pubkeys;
  for(const auto& step:doc.at("steps")){for(const auto& entry:step.at("interchange").at("data"))pubkeys.insert(entry.at("pubkey").get<std::string>());for(const auto* field:{"blocks","attestations"})for(const auto& entry:step.at(field))pubkeys.insert(entry.at("pubkey").get<std::string>());}
  // Empty data still requires metadata validation against a real journal.
  if(pubkeys.empty())pubkeys.insert("0xa99a76ed7796f7be22d5b7e85deeb7c5677e88e511e0b337618f8c4eb61349b4bf2d153f649f7b53359fe8b94a38e44c");
  std::map<std::string,std::unique_ptr<FixtureValidator>> validators;
  for(const auto& key:pubkeys)validators[key]=std::make_unique<FixtureValidator>(temp/key,root(doc.at("genesis_validators_root")),unhex(key));
  for(const auto& step:doc.at("steps")){
   bool failed=false;const auto encoded=step.at("interchange").dump();
   for(auto& [key,validator]:validators){
    const auto before=validator->journal->commitment();bool imported=true;
    try{validator->journal->import_interchange(encoded,1024*1024);}catch(const std::invalid_argument&){imported=false;}catch(const Json::exception&){imported=false;}
    ++imports;
    if(!imported){check(before==validator->journal->commitment(),"failed import mutated "+name);failed=true;}
    if(!step.at("should_succeed").get<bool>())check(!imported,"invalid interchange accepted "+name);
    else if(!step.at("contains_slashable_data").get<bool>())check(imported,"valid interchange rejected "+name);
    validator->reopen();++reopens;
   }
   if(failed){if(step.at("should_succeed").get<bool>()){++slashable_imports_refused;break;}continue;}
   for(const auto* field:{"blocks","attestations"})for(const auto& attempt:step.at(field)){
    auto& v=*validators.at(attempt.at("pubkey").get<std::string>());const auto before=v.journal->commitment();bool success=true;
    try{if(std::string_view(field)=="blocks")BeaconSigningJournalFixtureAccess::block(*v.journal,number(attempt.at("slot")),root(attempt.at("signing_root")));
     else BeaconSigningJournalFixtureAccess::vote(*v.journal,number(attempt.at("source_epoch")),number(attempt.at("target_epoch")),root(attempt.at("signing_root")));}
    catch(const std::runtime_error&){success=false;}catch(const std::invalid_argument&){success=false;}
    const bool minimal=attempt.at("should_succeed").get<bool>(),complete=attempt.at("should_succeed_complete").get<bool>();
    // Nano is minimal with exact latest-root repeats. For the ambiguous interval
    // require any extra acceptance to be an unchanged, authenticated repeat.
    check(!minimal||success,"minimal-safe attempt rejected "+name+": "+attempt.dump());
    check(complete||!success,"slashable attempt accepted "+name+": "+attempt.dump());
    if(success&&!minimal){check(before==v.journal->commitment(),"extra acceptance was not a repeat "+name);++optional_repeats;}
    if(!success)check(before==v.journal->commitment(),"refused attempt mutated "+name);
    ++attempts;if(success)++accepted;else ++refused;v.reopen();++reopens;
   }
  }
  ++cases;
 }
 std::cout<<Json{{"scenario","eip3076-official-reservations"},{"revision",manifest.at("revision")},{"cases",cases},{"perKeyImports",imports},{"reservationAttempts",attempts},{"accepted",accepted},{"refused",refused},{"exactRepeatExceptions",optional_repeats},{"slashableImportCasesRejected",slashable_imports_refused},{"journalReopens",reopens},{"signaturesProduced",false},{"validatorDutiesActive",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
