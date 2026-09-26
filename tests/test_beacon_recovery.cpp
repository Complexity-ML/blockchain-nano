#include "support/process.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "blst.h"
#ifdef NANO_TEST_SYNC_FAULT
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
extern "C" void nano_test_disarm_sync_fault();
#endif
using namespace test;
int main(int argc,char** argv){try{
 std::array<uint8_t,32> seed{};seed[0]=21;blst_scalar secret;blst_keygen(&secret,seed.data(),seed.size(),nullptr,0);blst_p1 point;blst_sk_to_pk_in_g1(&point,&secret);Bytes key(48);blst_p1_compress(key.data(),&point);
 const Bytes genesis(32,8);ConsensusRoot validators{};validators[0]=4;ConsensusBytes4 version{1,0,0,0};
 unsigned calls=0;
 auto sign=[&](const ConsensusRoot& root){++calls;static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes out(96);blst_p2_compress(out.data(),&s);return out;};
 ConsensusBeaconHeader header{10,0,{},{},{}};header.body_root[0]=1;
 if(argc==4&&std::string(argv[1])=="--crash"){
  ChainStore primary(argv[2]),witness(argv[3]);BeaconSigningJournal journal(primary,1337,genesis,validators,key,8,std::nullopt,&witness);
  journal.sign_proposal(header,version,[&](const auto&)->Bytes{raise(SIGKILL);throw std::runtime_error("kill returned");});return 1;
 }
 check(argc==1,"unknown recovery test mode");Temp temp;
 auto refuse=[&](auto action){auto before=calls;bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected&&calls==before,"unsafe recovery reached signer");};
 {
  ChainStore primary(temp/"primary"),witness(temp/"witness");BeaconSigningJournal journal(primary,1337,genesis,validators,key,8,std::nullopt,&witness);
 }
 fs::copy(temp/"primary",temp/"previous",fs::copy_options::recursive);
 Process child({argv[0],"--crash",(temp/"primary").string(),(temp/"witness").string()});check(child.wait().code==-SIGKILL,"child did not crash before signature");
 // Both writes survived; recover with the witness, without an orderly receipt.
 {
  ChainStore primary(temp/"primary"),witness(temp/"witness");BeaconSigningJournal journal(primary,1337,genesis,validators,key,8,std::nullopt,&witness);
  auto bad=header;bad.body_root[0]^=1;refuse([&]{journal.sign_proposal(bad,version,sign);});journal.sign_proposal(header,version,sign);
 }
 // Model loss of the primary's last write with a retained, separately stored
 // witness. Recovery must finish the intended reservation, never choose old state.
 fs::remove_all(temp/"primary");fs::copy(temp/"previous",temp/"primary",fs::copy_options::recursive);
 {
  ChainStore primary(temp/"primary"),witness(temp/"witness");BeaconSigningJournal journal(primary,1337,genesis,validators,key,8,std::nullopt,&witness);
  check(journal.latest_proposal_slot()==10,"reservation not rolled forward");
  auto bad=header;bad.body_root[0]^=1;refuse([&]{journal.sign_proposal(bad,version,sign);});
  BeaconSigningJournal stale(primary,1337,genesis,validators,key,8,std::nullopt,&witness);
  header.slot=11;journal.sign_proposal(header,version,sign);refuse([&]{stale.sign_proposal(header,version,sign);});
  ConsensusAttestationData vote;vote.slot=40;vote.source.epoch=3;vote.target.epoch=5;journal.sign_attestation(vote,version,sign);
 }
 // A primary older than the witness's immediate predecessor is not guessed.
 fs::remove_all(temp/"primary");fs::copy(temp/"previous",temp/"primary",fs::copy_options::recursive);
 {
  ChainStore primary(temp/"primary"),witness(temp/"witness");refuse([&]{BeaconSigningJournal journal(primary,1337,genesis,validators,key,8,std::nullopt,&witness);});
 }
 // Empty or wrong-key witnesses cannot authorize an existing database.
 {
  ChainStore primary(temp/"previous"),empty(temp/"empty");refuse([&]{BeaconSigningJournal journal(primary,1337,genesis,validators,key,8,std::nullopt,&empty);});
 }
 {
  ChainStore primary(temp/"wrong-key"),witness(temp/"witness");auto wrong=validators;wrong[0]^=1;refuse([&]{BeaconSigningJournal journal(primary,1337,genesis,wrong,key,8,std::nullopt,&witness);});
  check(!primary.fork_checkpoint(1337,genesis,4096),"wrong identity mutated primary");
 }
#ifdef NANO_TEST_SYNC_FAULT
 for(bool fail_witness:{false,true}){
  const auto name=fail_witness?"fault-witness":"fault-primary";
  {
   ChainStore primary(temp/(std::string(name)+"-p")),witness(temp/(std::string(name)+"-w"));
   BeaconSigningJournal journal(primary,1337,genesis,validators,key,8,std::nullopt,&witness);
   const auto initial=journal.commitment();fs::path wal;
   for(const auto& f:fs::directory_iterator(temp/(std::string(name)+(fail_witness?"-w":"-p"))))if(f.path().extension()==".log"&&(wal.empty()||f.path().filename()>wal.filename()))wal=f.path();
   check(!wal.empty(),"fault WAL missing");nano_test_arm_sync_fault(wal.c_str());
   header.slot=10;refuse([&]{journal.sign_proposal(header,version,sign);});
   check(nano_test_sync_failures()>0,"sync failure not exercised");nano_test_disarm_sync_fault();
   refuse([&]{journal.sign_proposal(header,version,sign);});
   if(fail_witness)check(primary.fork_checkpoint(1337,genesis,4096)->token==initial,"primary advanced despite failed witness");
  }
  if(!fail_witness){
   ChainStore primary(temp/(std::string(name)+"-p")),witness(temp/(std::string(name)+"-w"));
   BeaconSigningJournal resumed(primary,1337,genesis,validators,key,8,std::nullopt,&witness);
   check(resumed.latest_proposal_slot()==10,"uncertain primary write lost witness reservation");
   auto bad=header;bad.body_root[0]^=1;refuse([&]{resumed.sign_proposal(bad,version,sign);});
  }
 }
#endif
 std::cout<<"Witness crash recovery, exact roll-forward, rollback refusal, identity and stale writer checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
