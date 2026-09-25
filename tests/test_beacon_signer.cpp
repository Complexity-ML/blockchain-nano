#include "support/process.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_bls.hpp"
#include "blst.h"
#include <atomic>
#include <future>
#ifdef NANO_TEST_SYNC_FAULT
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
extern "C" void nano_test_disarm_sync_fault();
#endif
using namespace test;
int main(int argc,char** argv){try{
 std::array<uint8_t,32> seed{};seed[0]=1;blst_scalar secret;blst_keygen(&secret,seed.data(),seed.size(),nullptr,0);blst_p1 public_point;blst_sk_to_pk_in_g1(&public_point,&secret);Bytes key(48);blst_p1_compress(key.data(),&public_point);
 const Bytes genesis(32,9);ConsensusRoot validators{};validators[0]=3;const ConsensusBytes4 version{1,0,0,0};
 unsigned calls=0;BeaconSigningJournal::Signer sign=[&](const ConsensusRoot& root){++calls;static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes out(96);blst_p2_compress(out.data(),&s);return out;};
 ConsensusBeaconHeader header{10,0,{},{},{}};header.body_root[0]=1;
 if(argc==3&&std::string(argv[1])=="--crash"){
  ChainStore disk(argv[2]);BeaconSigningJournal journal(disk,1337,genesis,validators,key,8);
  journal.sign_proposal(header,version,[&](const auto&)->Bytes{const auto saved=disk.fork_checkpoint(1337,genesis,4096);check(saved.has_value(),"signature requested before durable reservation");std::cout<<Json{{"event","reserved-before-crash"},{"sequence",saved->token.sequence},{"digest",hex(saved->token.digest)}}.dump()<<std::endl;raise(SIGKILL);throw std::runtime_error("SIGKILL returned");});return 1;
 }
 check(argc==1,"unexpected test arguments");Temp temp;const auto path=temp/"journal";auto disk=std::make_unique<ChainStore>(path);auto journal=std::make_unique<BeaconSigningJournal>(*disk,1337,genesis,validators,key,8);
 unsigned rejected=0;
 auto refuse=[&](auto action){const auto before=calls;bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed&&calls==before,"unsafe request reached signing provider");++rejected;};
 const auto initial=journal->commitment();
 auto signature=journal->sign_proposal(header,version,[&](const auto& root){const auto saved=disk->fork_checkpoint(1337,genesis,4096);check(saved&&saved->token.sequence==initial.sequence+1&&Json::parse(saved->snapshot).at("proposal").at("root")==hex(Bytes(root.begin(),root.end())),"provider called before durable proposal reservation");return sign(root);});
 check(verify_consensus_beacon_header(header,version,validators,key,signature),"proposal signature has wrong protocol domain");
 check(signature==journal->sign_proposal(header,version,sign),"exact repeat changed signature");
 auto conflicting=header;conflicting.body_root[0]^=1;refuse([&]{journal->sign_proposal(conflicting,version,sign);});
 auto fork=version;fork[0]^=1;refuse([&]{journal->sign_proposal(header,fork,sign);});
 ConsensusAttestationData data;data.slot=40;data.source.epoch=3;data.target.epoch=5;data.beacon_block_root[0]=1;
 const auto vote_sig=journal->sign_attestation(data,version,sign);check(verify_consensus_bls(key,consensus_signing_root(consensus_attestation_root(data),consensus_domain({1,0,0,0},version,validators)),vote_sig),"attestation signature has wrong protocol domain");check(vote_sig==journal->sign_attestation(data,version,sign),"repeat vote changed signature");
 auto bad=data;bad.beacon_block_root[0]^=1;refuse([&]{journal->sign_attestation(bad,version,sign);});
 bad=data;bad.source.epoch=2;bad.target.epoch=6;bad.slot=48;refuse([&]{journal->sign_attestation(bad,version,sign);}); // surrounds prior vote
 bad=data;bad.source.epoch=4;bad.target.epoch=4;bad.slot=32;refuse([&]{journal->sign_attestation(bad,version,sign);}); // surrounded by prior vote
 bad=data;bad.target.epoch=4;refuse([&]{journal->sign_attestation(bad,version,sign);}); // inconsistent slot
 data.source.epoch=4;data.target.epoch=6;data.slot=48;journal->sign_attestation(data,version,sign);
 auto token=journal->commitment();journal.reset();disk.reset();disk=std::make_unique<ChainStore>(path);
 refuse([&]{BeaconSigningJournal missing(*disk,1337,genesis,validators,key,8);});
 refuse([&]{BeaconSigningJournal stale(*disk,1337,genesis,validators,key,8,initial);});
 auto other_genesis=validators;other_genesis[0]^=1;refuse([&]{BeaconSigningJournal other(*disk,1337,genesis,other_genesis,key,8,token);});
 refuse([&]{BeaconSigningJournal other(*disk,1337,genesis,validators,key,16,token);});
 journal=std::make_unique<BeaconSigningJournal>(*disk,1337,genesis,validators,key,8,token);
 refuse([&]{journal->sign_proposal(conflicting,version,sign);});
 bad=data;bad.source.epoch=3;bad.target.epoch=7;bad.slot=56;refuse([&]{journal->sign_attestation(bad,version,sign);});
 // A second object cannot sign from a stale view, even in the same process.
 {BeaconSigningJournal stale_writer(*disk,1337,genesis,validators,key,8,token);
 header.slot=11;journal->sign_proposal(header,version,sign);refuse([&]{stale_writer.sign_proposal(header,version,sign);});}
 // Provider failure does not erase the reservation or permit a conflicting retry.
 header.slot=12;bool provider_failed=false;try{journal->sign_proposal(header,version,[](const auto&){return Bytes{};});}catch(const std::runtime_error&){provider_failed=true;}check(provider_failed,"invalid provider signature escaped");
 conflicting=header;conflicting.body_root[0]^=1;refuse([&]{journal->sign_proposal(conflicting,version,sign);});journal->sign_proposal(header,version,sign);
 // Concurrent requests for the same target epoch: only one provider invocation.
 data.slot=56;data.target.epoch=7;auto alternative=data;alternative.beacon_block_root[0]^=1;std::atomic<unsigned> wins=0;const auto old_calls=calls;
 auto request=[&](ConsensusAttestationData d){try{journal->sign_attestation(d,version,sign);++wins;}catch(const std::runtime_error&){}};
 std::thread first(request,data),second(request,alternative);first.join();second.join();check(wins==1&&calls==old_calls+1,"concurrent conflicting signatures escaped");
 bool sync_tested=false;
#ifdef NANO_TEST_SYNC_FAULT
 fs::path wal;for(const auto& entry:fs::directory_iterator(path))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"missing signing WAL");nano_test_arm_sync_fault(wal.c_str());header.slot=13;refuse([&]{journal->sign_proposal(header,version,sign);});check(nano_test_sync_failures()>0,"signing sync failure not reached");nano_test_disarm_sync_fault();refuse([&]{journal->sign_proposal(header,version,sign);});
 journal.reset();disk.reset();disk=std::make_unique<ChainStore>(path);const auto recovered=disk->fork_checkpoint(1337,genesis,4096);check(recovered.has_value(),"journal lost after sync failure");journal=std::make_unique<BeaconSigningJournal>(*disk,1337,genesis,validators,key,8,recovered->token);journal->sign_proposal(header,version,sign);conflicting=header;conflicting.body_root[0]^=1;refuse([&]{journal->sign_proposal(conflicting,version,sign);});sync_tested=true;
#endif
 // Kill a separate process exactly after reservation, before returning any signature.
 Process child({argv[0],"--crash",(temp/"crash-journal").string()});const auto saved=child.event("reserved-before-crash");check(child.wait().code==-SIGKILL,"child did not crash before signature");
 ChainStore crash_disk(temp/"crash-journal");const ForkCheckpointToken retained{saved.at("sequence").get<uint64_t>(),unhex(saved.at("digest").get<std::string>())};BeaconSigningJournal resumed(crash_disk,1337,genesis,validators,key,8,retained);
 header.slot=10;conflicting=header;conflicting.body_root[0]^=1;refuse([&]{resumed.sign_proposal(conflicting,version,sign);});resumed.sign_proposal(header,version,sign);
 // Export retires the source durably and fences other in-process instances.
 const auto before_export=journal->commitment();
 BeaconSigningJournal export_stale(*disk,1337,genesis,validators,key,8,before_export);
 const auto local=Json::parse(disk->fork_checkpoint(1337,genesis,4096,before_export)->snapshot);
 const auto exported=journal->retire_and_export_interchange();const auto interchange=Json::parse(exported);
 check(interchange.at("metadata").at("interchange_format_version")=="5"&&interchange.at("metadata").at("genesis_validators_root")==hex(Bytes(validators.begin(),validators.end())),"export metadata incorrect");
 const auto& entry=interchange.at("data").at(0);
 check(entry.at("pubkey")==hex(key)&&entry.at("signed_blocks").size()==1&&entry.at("signed_attestations").size()==1,"export missing reservations");
 check(entry.at("signed_blocks").at(0).at("slot")==std::to_string(u64(quantity(local.at("proposal").at("slot"))))&&entry.at("signed_blocks").at(0).at("signing_root")==local.at("proposal").at("root"),"export lost block guard");
 const auto& att=entry.at("signed_attestations").at(0);
 check(att.at("source_epoch")==std::to_string(u64(quantity(local.at("attestation").at("source"))))&&att.at("target_epoch")==std::to_string(u64(quantity(local.at("attestation").at("target"))))&&att.at("signing_root")==local.at("attestation").at("root"),"export lost vote guard");
 const auto retired_token=journal->commitment();check(retired_token.sequence==before_export.sequence+1,"retirement not committed");
 header.slot=100;data.slot=800;data.source.epoch=99;data.target.epoch=100;
 refuse([&]{journal->sign_proposal(header,version,sign);});refuse([&]{journal->sign_attestation(data,version,sign);});refuse([&]{export_stale.sign_proposal(header,version,sign);});
 check(exported==journal->retire_and_export_interchange()&&retired_token==journal->commitment(),"repeat export changed retired state");
 journal.reset();disk.reset();disk=std::make_unique<ChainStore>(path);
 journal=std::make_unique<BeaconSigningJournal>(*disk,1337,genesis,validators,key,8,retired_token);
 refuse([&]{journal->sign_proposal(header,version,sign);});refuse([&]{journal->sign_attestation(data,version,sign);});
 check(exported==journal->retire_and_export_interchange(),"reopened retirement lost export");
 {ChainStore empty_disk(temp/"empty-export");BeaconSigningJournal empty(empty_disk,1337,genesis,validators,key,8);
 const auto doc=Json::parse(empty.retire_and_export_interchange());check(doc.at("data").at(0).at("signed_blocks").empty()&&doc.at("data").at(0).at("signed_attestations").empty(),"empty history invented signatures");refuse([&]{empty.sign_proposal(header,version,sign);});}
#ifdef NANO_TEST_SYNC_FAULT
 {const auto fault_path=temp/"export-fault";ChainStore fault_disk(fault_path);BeaconSigningJournal fault(fault_disk,1337,genesis,validators,key,8);
 fs::path fault_wal;for(const auto& entry:fs::directory_iterator(fault_path))if(entry.path().extension()==".log"&&(fault_wal.empty()||entry.path().filename()>fault_wal.filename()))fault_wal=entry.path();check(!fault_wal.empty(),"missing export WAL");
 nano_test_arm_sync_fault(fault_wal.c_str());refuse([&]{fault.retire_and_export_interchange();});check(nano_test_sync_failures()>0,"export sync failure not reached");nano_test_disarm_sync_fault();refuse([&]{fault.sign_proposal(header,version,sign);});refuse([&]{fault.retire_and_export_interchange();});}
#endif
 // Import an actual export, then preserve protections across unknown roots.
 const auto import_path=temp/"import";auto import_disk=std::make_unique<ChainStore>(import_path);
 auto imported=std::make_unique<BeaconSigningJournal>(*import_disk,1337,genesis,validators,key,8);
 check(imported->import_interchange(exported,exported.size())==1,"export did not import");
 const auto imported_token=imported->commitment();check(imported->import_interchange(exported,exported.size())==1&&imported_token==imported->commitment(),"repeat import wrote or weakened guards");
 header.slot=u64(quantity(local.at("proposal").at("slot")));header.body_root[0]=1;
 imported->sign_proposal(header,version,sign); // exact known root survives migration
 auto unknown=interchange;unknown["data"][0]["signed_blocks"][0].erase("signing_root");unknown["data"][0]["signed_attestations"][0].erase("signing_root");
 const auto unknown_text=unknown.dump();imported->import_interchange(unknown_text,unknown_text.size());
 refuse([&]{imported->sign_proposal(header,version,sign);});
 data.slot=56;data.target.epoch=7;data.source.epoch=4;refuse([&]{imported->sign_attestation(data,version,sign);});
 // Known roots cannot erase an unknown reservation at the same watermarks.
 const auto unknown_token=imported->commitment();imported->import_interchange(exported,exported.size());check(unknown_token==imported->commitment(),"known reimport erased uncertainty");
 imported.reset();import_disk.reset();import_disk=std::make_unique<ChainStore>(import_path);
 imported=std::make_unique<BeaconSigningJournal>(*import_disk,1337,genesis,validators,key,8,unknown_token);
 refuse([&]{imported->sign_proposal(header,version,sign);});
 // Validate all entries before committing, including entries for other keys.
 auto reject_import=[&](const std::string& text,size_t bound){const auto before=imported->commitment();refuse([&]{imported->import_interchange(text,bound);});check(before==imported->commitment(),"invalid import mutated journal");};
 reject_import(exported,exported.size()-1);
 auto invalid=interchange;invalid["metadata"]["interchange_format_version"]="4";reject_import(invalid.dump(),100000);
 invalid=interchange;invalid["metadata"]["genesis_validators_root"]=hex(Bytes(32,7));reject_import(invalid.dump(),100000);
 for(const Json& value:std::vector<Json>{Json(12),Json("-1"),Json("18446744073709551616"),Json(" 12"),Json("0xc"),Json("")}){invalid=interchange;invalid["data"][0]["signed_blocks"][0]["slot"]=value;reject_import(invalid.dump(),100000);}
 invalid=interchange;invalid["data"][0]["signed_blocks"][0]["signing_root"]=nullptr;reject_import(invalid.dump(),100000);
 invalid=interchange;invalid["data"][0]["signed_blocks"][0]["slot"]="1000";auto foreign=invalid["data"][0];foreign["pubkey"]=hex(Bytes(48,1));foreign["signed_attestations"][0]["target_epoch"]="bad";invalid["data"].push_back(foreign);reject_import(invalid.dump(),100000);
 invalid=interchange;Json nested=0;for(unsigned i=0;i<20;++i)nested=Json::array({nested});invalid["extension"]=nested;reject_import(invalid.dump(),100000);
 auto duplicate_json=exported;duplicate_json.insert(1,"\"metadata\":{},");reject_import(duplicate_json,100000);
 // Ignore valid other-key entries explicitly; do not pretend to import them.
 invalid=interchange;invalid["data"][0]["pubkey"]=hex(Bytes(48,1));const auto unmatched=imported->commitment();check(imported->import_interchange(invalid.dump(),100000)==0&&unmatched==imported->commitment(),"foreign key affected journal");
 // Different messages supply maxima: combined (source=9,target=12) has no root.
 auto merge=interchange;merge["data"][0]["signed_blocks"]=Json::array();
 merge["data"][0]["signed_attestations"]=Json::array({Json{{"source_epoch","9"},{"target_epoch","10"},{"signing_root",hex(Bytes(32,1))}}});
 auto second_entry=merge["data"][0];second_entry["signed_attestations"]=Json::array({Json{{"source_epoch","8"},{"target_epoch","12"},{"signing_root",hex(Bytes(32,2))}}});merge["data"].push_back(second_entry);
 check(imported->import_interchange(merge.dump(),100000)==2,"duplicate key entries not merged");
 data.source.epoch=8;data.target.epoch=13;data.slot=104;refuse([&]{imported->sign_attestation(data,version,sign);});
 data.source.epoch=9;data.target.epoch=12;data.slot=96;refuse([&]{imported->sign_attestation(data,version,sign);});
 const auto merged_export=Json::parse(imported->retire_and_export_interchange());const auto& merged_vote=merged_export.at("data").at(0).at("signed_attestations").at(0);
 check(merged_vote.at("source_epoch")=="9"&&merged_vote.at("target_epoch")=="12"&&!merged_vote.contains("signing_root"),"synthetic watermark acquired signing root");
 check(!merged_export.at("data").at(0).at("signed_blocks").at(0).contains("signing_root"),"unknown block root fabricated");
 refuse([&]{imported->import_interchange(exported,100000);});
 // Round trip the synthetic guard and progress only beyond both maxima.
 ChainStore roundtrip_disk(temp/"import-roundtrip");BeaconSigningJournal roundtrip(roundtrip_disk,1337,genesis,validators,key,8);
 roundtrip.import_interchange(merged_export.dump(),100000);refuse([&]{roundtrip.sign_attestation(data,version,sign);});data.target.epoch=13;data.slot=104;roundtrip.sign_attestation(data,version,sign);
 header.slot=100;roundtrip.sign_proposal(header,version,sign);
#ifdef NANO_TEST_SYNC_FAULT
 {const auto fault_path=temp/"import-fault";ChainStore fault_disk(fault_path);BeaconSigningJournal fault(fault_disk,1337,genesis,validators,key,8);
 fs::path fault_wal;for(const auto& entry:fs::directory_iterator(fault_path))if(entry.path().extension()==".log"&&(fault_wal.empty()||entry.path().filename()>fault_wal.filename()))fault_wal=entry.path();check(!fault_wal.empty(),"missing import WAL");
 nano_test_arm_sync_fault(fault_wal.c_str());refuse([&]{fault.import_interchange(exported,100000);});check(nano_test_sync_failures()>0,"import sync failure not reached");nano_test_disarm_sync_fault();refuse([&]{fault.sign_proposal(header,version,sign);});}
#endif
 // Another object sharing the database must wait for an exact-repeat provider
 // before export. A token check alone cannot close this read/sign race.
 {ChainStore gate_disk(temp/"shared-signing-gate");BeaconSigningJournal first(gate_disk,1337,genesis,validators,key,8);
 first.sign_proposal(header,version,sign);BeaconSigningJournal second(gate_disk,1337,genesis,validators,key,8,first.commitment());
 std::promise<void> entered,release,export_started;auto released=release.get_future();
 auto signing=std::async(std::launch::async,[&]{return first.sign_proposal(header,version,[&](const auto& root){entered.set_value();released.wait();return sign(root);});});
 entered.get_future().get();auto exporting=std::async(std::launch::async,[&]{export_started.set_value();return second.retire_and_export_interchange();});
 export_started.get_future().get();const auto state=exporting.wait_for(std::chrono::milliseconds(50));release.set_value();signing.get();exporting.get();check(state==std::future_status::timeout,"export crossed another instance's signing callback");
 refuse([&]{first.sign_proposal(header,version,sign);});refuse([&]{second.sign_proposal(header,version,sign);});}
 std::cout<<Json{{"scenario","beacon-signing-journal"},{"rejectedBeforeProvider",rejected},{"concurrentConflictRefused",true},{"crashAfterReservation",true},{"syncFailureTested",sync_tested},{"validatorDutiesActive",false},{"eip3076ExportRetiresDurably",true},{"eip3076Import",true},{"sharedInstanceExportWaitsForSigner",true},{"officialInterchangeCorpusInThisTest",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
