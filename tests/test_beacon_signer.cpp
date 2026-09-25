#include "support/process.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_bls.hpp"
#include "blst.h"
#include <atomic>
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
 std::cout<<Json{{"scenario","beacon-signing-journal"},{"rejectedBeforeProvider",rejected},{"concurrentConflictRefused",true},{"crashAfterReservation",true},{"syncFailureTested",sync_tested},{"validatorDutiesActive",false},{"eip3076ImportExport",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
