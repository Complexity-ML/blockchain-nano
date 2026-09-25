#include "support/process.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/beacon_journal_cli.hpp"
#include <sys/stat.h>
#ifdef NANO_TEST_SYNC_FAULT
extern "C" void nano_test_arm_future_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
extern "C" void nano_test_disarm_sync_fault();
#endif
using namespace test;
int main(int argc,char** argv){try{
 check(argc==2,"CLI test requires nano executable");const std::string binary=argv[1];Temp temp;
 const auto identity=temp/"identity.json",history=temp/"history.json",db=temp/"source",init=temp/"init";
 const std::string public_key="0xa99a76ed7796f7be22d5b7e85deeb7c5677e88e511e0b337618f8c4eb61349b4bf2d153f649f7b53359fe8b94a38e44c";
 const Bytes execution(32,9);ConsensusRoot validators{};validators[0]=3;
 Json id{{"chainId","0x539"},{"executionGenesis",hex(execution)},{"genesisValidatorsRoot",hex(Bytes(validators.begin(),validators.end()))},{"publicKey",public_key},{"slotsPerEpoch","0x8"}};save(identity,id);
 Json interchange{{"metadata",{{"interchange_format_version","5"},{"genesis_validators_root",id.at("genesisValidatorsRoot")}}},{"data",Json::array({Json{{"pubkey",public_key},{"signed_blocks",Json::array({Json{{"slot","10"}}})},{"signed_attestations",Json::array({Json{{"source_epoch","3"},{"target_epoch","5"}}})}}})}};save(history,interchange);
 unsigned commands=0,refusals=0;
 auto command=[&](std::vector<std::string> args,bool success=true){args.insert(args.begin(),{binary,"beacon-journal"});const auto result=run(args);++commands;check((result.code==0)==success,"unexpected CLI status: "+result.err);if(!success){++refusals;return Json();}return Json::parse(result.out);};
 auto token=[&](const fs::path& directory){return (directory/"commitment.json").string();};
 auto result=command({"init",identity.string(),db.string(),init.string()});check(!result.at("retired").get<bool>()&&load(init/"complete.json")==result&&load(init/"commitment.json")==result.at("commitment"),"initial receipt missing or inconsistent");
 for(const auto& path:{init,init/"intent.json",init/"commitment.json",init/"complete.json"}){struct stat info{};check(::stat(path.c_str(),&info)==0&&(info.st_mode&0777)==(path==init?0700:0600),"receipt permissions");}
 command({"init",identity.string(),db.string(),(temp/"overwrite-db").string()},false);check(!fs::exists(temp/"overwrite-db"),"invalid initialization touched output");
 check(command({"status",identity.string(),db.string(),token(init)}).at("commitment")==result.at("commitment"),"status changed journal");
 command({"status",identity.string(),(temp/"missing").string(),token(init)},false);check(!fs::exists(temp/"missing"),"status initialized missing database");
 const auto imported=temp/"import";result=command({"import",identity.string(),db.string(),token(init),history.string(),"65536",imported.string()});check(result.at("matchedEntries")==1&&result.at("commitment").at("sequence")=="0x2","import receipt incorrect");
 command({"status",identity.string(),db.string(),token(init)},false);
 check(command({"status",identity.string(),db.string(),token(imported)}).at("commitment")==result.at("commitment"),"external commitment not reusable");
 command({"import",identity.string(),db.string(),token(imported),history.string(),"1",(temp/"too-large").string()},false);check(!fs::exists(temp/"too-large"),"oversize input created artifacts");
 command({"import",identity.string(),db.string(),token(imported),history.string(),"-1",(temp/"bad-limit").string()},false);
 command({"export",identity.string(),db.string(),token(imported),imported.string()},false); // no overwrite or retirement
 check(command({"status",identity.string(),db.string(),token(imported)}).at("commitment")==result.at("commitment"),"existing output retired journal");
 auto bad=id;bad["genesisValidatorsRoot"]=hex(Bytes(32,7));save(temp/"wrong-identity.json",bad);command({"export",(temp/"wrong-identity.json").string(),db.string(),token(imported),(temp/"wrong-output").string()},false);check(!fs::exists(temp/"wrong-output"/"complete.json"),"failed identity marked complete");
 const auto link=temp/"token-link.json";fs::create_symlink(imported/"commitment.json",link);command({"status",identity.string(),db.string(),link.string()},false);
 const auto outlink=temp/"output-link";fs::create_directory_symlink(imported,outlink);command({"export",identity.string(),db.string(),token(imported),outlink.string()},false);
 command({"export",identity.string(),db.string(),token(imported),(db/"receipt").string()},false);check(!fs::exists(db/"receipt"),"receipt placed inside DB");
 // A live RocksDB owner prevents a second process from using the journal.
 {ChainStore owner(db);command({"status",identity.string(),db.string(),token(imported)},false);}
 auto invalid=interchange;invalid["data"][0]["signed_blocks"][0]["slot"]="-1";save(temp/"bad-history.json",invalid);
 command({"import",identity.string(),db.string(),token(imported),(temp/"bad-history.json").string(),"65536",(temp/"failed-import").string()},false);
 check(!fs::exists(temp/"failed-import"/"complete.json")&&command({"status",identity.string(),db.string(),token(imported)}).at("commitment")==result.at("commitment"),"failed import mutated state or marked complete");
 const auto exported=temp/"export";result=command({"export",identity.string(),db.string(),token(imported),exported.string()});
 check(result.at("retired").get<bool>()&&result.at("commitment").at("sequence")=="0x3"&&load(exported/"interchange.json")==interchange&&load(exported/"complete.json")==result,"export lost reservations");
 const auto exported_bytes=read(exported/"interchange.json");check(result.at("interchangeSha256")==hex(sha256(Bytes(exported_bytes.begin(),exported_bytes.end()))),"export artifact digest mismatch");
 command({"status",identity.string(),db.string(),token(imported)},false);
 command({"import",identity.string(),db.string(),token(exported),history.string(),"65536",(temp/"retired-import").string()},false);
 const auto repeated=temp/"export-again";check(command({"export",identity.string(),db.string(),token(exported),repeated.string()}).at("commitment")==result.at("commitment")&&load(repeated/"interchange.json")==interchange,"repeat export changed retirement");
 // Destination uses its own database/commitment; the source cannot sign again.
 const auto dest=temp/"destination",dest_init=temp/"destination-init",dest_import=temp/"destination-import";
 command({"init",identity.string(),dest.string(),dest_init.string()});auto destination=command({"import",identity.string(),dest.string(),token(dest_init),(exported/"interchange.json").string(),"65536",dest_import.string()});
 auto commitment=[](const Json& value){return ForkCheckpointToken{u64(quantity(value.at("sequence"))),unhex(value.at("digest").get<std::string>())};};
 {ChainStore source_store(db),destination_store(dest);BeaconSigningJournal source(source_store,1337,execution,validators,unhex(public_key),8,commitment(result.at("commitment"))),target(destination_store,1337,execution,validators,unhex(public_key),8,commitment(destination.at("commitment")));
 unsigned calls=0;auto provider=[&](const auto&){++calls;return Bytes{};};ConsensusBeaconHeader header{100,0,{},{},{}};
 bool failed=false;try{source.sign_proposal(header,{1,0,0,0},provider);}catch(const std::exception&){failed=true;}check(failed&&calls==0,"retired CLI source reached provider");
 header.slot=10;failed=false;try{target.sign_proposal(header,{1,0,0,0},provider);}catch(const std::exception&){failed=true;}check(failed&&calls==0,"destination lost unknown-root protection");}
 unsigned artifact_faults=0;
#ifdef NANO_TEST_SYNC_FAULT
 for(const auto* name:{"commitment.json","interchange.json"}){
  const auto fault_db=temp/(std::string("fault-db-")+name),fault_init=temp/(std::string("fault-init-")+name),fault_output=temp/(std::string("fault-output-")+name);
  command({"init",identity.string(),fault_db.string(),fault_init.string()});
  const auto target_path=fs::weakly_canonical(fault_output/name);nano_test_arm_future_sync_fault(target_path.c_str());
  std::vector<std::string> args{binary,"beacon-journal","export",identity.string(),fault_db.string(),token(fault_init),fault_output.string()};std::vector<char*> pointers;for(auto& arg:args)pointers.push_back(arg.data());
  bool failed=false;try{beacon_journal_command(int(pointers.size()),pointers.data());}catch(const std::exception&){failed=true;}
  const auto failures=nano_test_sync_failures();nano_test_disarm_sync_fault();check(failed&&failures>0&&!fs::exists(fault_output/"complete.json"),"artifact sync failure was not fail-closed");++artifact_faults;
  command({"status",identity.string(),fault_db.string(),token(fault_init)},false); // no automatic token reset
  if(std::string_view(name)=="interchange.json"){
   // The commitment was synced before the failed interchange; re-export works.
   const auto recovered=command({"export",identity.string(),fault_db.string(),token(fault_output),(temp/"recovered-export").string()});check(recovered.at("retired").get<bool>(),"failed artifact write resumed signing");
  }else{
   // Inspect after failure for this test only. The CLI deliberately offers no
   // recovery that trusts an unauthenticated token read from the database.
   ChainStore observed(fault_db);auto state=observed.fork_checkpoint(1337,execution,4096);check(state&&Json::parse(state->snapshot).at("retired")==true,"receipt failure erased retirement");
  }
 }
#endif
 std::cout<<Json{{"scenario","beacon-journal-cli"},{"commands",commands},{"artifactSyncFailures",artifact_faults},{"refusals",refusals},{"offlineMigration",true},{"retiredSourceRefusesSigning",true},{"destinationPreservesUnknownRoots",true},{"fundsMoved",false},{"validatorDutiesActive",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
