#include "support/process.hpp"
#include "nano/chain.hpp"
#include "nano/block_auth.hpp"
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
extern "C" void nano_test_disarm_sync_fault();
using namespace test;
int main(int argc,char** argv){try{
 check(argc==2||argc==3||argc==5,"fork or fork/root/boundary/phase required");std::string fork=argv[1];check(fork=="Paris"||fork=="Shanghai","explicit fork required");
 const bool retry_mode=argc==3&&std::string(argv[2])=="--retry";check(argc!=3||retry_mode,"unknown mode");
 if(argc==2||retry_mode){Json records=Json::array();for(const std::string disposition:{"retained","lost"})for(const std::string boundary:{"reservation","commit","progress"}){Temp root;
  auto failed=success(run({argv[0],fork,root.path.string(),boundary,retry_mode?"fail-retry":"fail"}));check(failed.at("syncFailures").get<unsigned>()>0,"sync fault was not exercised");
  if(disposition=="lost"){
   // The faulted child has exited. Remove only the observed unsynced append
   // from its selected temporary WAL; preserve every earlier byte/database.
   auto name=failed.at("walName").get<std::string>();check(fs::path(name).filename()==name,"invalid WAL basename");auto wal=root/(boundary=="commit"?"chain":"journal")/name;
   auto before=failed.at("walBytesBefore").get<uint64_t>();check(fs::file_size(wal)==before+failed.at("walAppendedBytes").get<uint64_t>(),"unexpected WAL changes after child exit");
   int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open owned WAL for loss simulation");bool truncated=ftruncate(fd,before)==0&&fsync(fd)==0;close(fd);check(truncated&&fs::file_size(wal)==before,"failed to simulate selected unsynced suffix loss");
  }
  auto recovered=success(run({argv[0],fork,root.path.string(),boundary,"recover"}));check(recovered.at("blockHash")==failed.at("blockHash")&&recovered.at("audit").at("verified")==true,"recovery changed certified block");
  if(boundary!="reservation")check(recovered.at("precommitSignature")==failed.at("precommitSignature"),"durable precommit changed after sync error");
  if(disposition=="lost")check(recovered.at("reopenedHeight")== (boundary=="progress"?"0x1":"0x0")&&!recovered.at("reservationRetained").get<bool>(),"removed WAL suffix survived restart");
  if(boundary=="progress")check(recovered.at("journalHeightBeforeRecovery")== (disposition=="lost"?1:2),"journal suffix disposition not observed");
  records.push_back({{"disposition",disposition},{"boundary",boundary},{"failed",failed},{"recovered",recovered}});
 }std::cout<<Json{{"fork",fork},{"fault","macOS fsync EIO injection"},{"records",records}}.dump()<<std::endl;return 0;}
 fs::path root=argv[2];std::string boundary=argv[3],phase=argv[4];check(boundary=="reservation"||boundary=="commit"||boundary=="progress","invalid boundary");const bool failing=phase=="fail"||phase=="fail-retry";check(failing||phase=="recover","invalid phase");
 auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));auto recipient=public_address(public_key(bytes(Int(2),32)));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"evmFork",fork},{"bootstrapValidator",signer}}},{"alloc",{{signer,{{"balance","1000000000000000000000"}}}}}});
 Blockchain chain(root/"chain",genesis);ChainStore journal(root/"journal");auto reopened_height=chain.tip().header.number;auto original_position=journal.voting_position(signer,1337,genesis.block.header.hash());auto original_journal_height=original_position?original_position->height:0;check(reopened_height==0||reopened_height==1,"unexpected recovered height");
 Transaction tx;tx.to=recipient;tx.value=123;tx.gas=21000;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,key);Block block;
 if(reopened_height==0){check(chain.submit(signed_tx),"submit transfer");block=chain.block_template(signer,12);sign_block(block,key,1337,genesis.block.header.hash());}
 else{check(phase=="recover"&&boundary!="reservation","only fully appended canonical write may survive sync failure");block=chain.tip();}
 std::vector<FinalityVote> certificate;
 if(boundary!="reservation"){
  if(failing){auto prevote=chain.vote_for_block(journal,key,block,0,VoteStep::prevote,{},12);certificate.push_back(chain.vote_for_block(journal,key,block,0,VoteStep::precommit,{prevote},12));}
  else if(boundary=="progress"){auto durable=chain.store().certificate_at(1);check(durable.has_value(),"durable canonical certificate missing after progress failure");certificate=durable->votes;}
  else{auto reserved=journal.latest_consensus_vote(signer,1337,genesis.block.header.hash());check(reserved&&reserved->step==VoteStep::precommit&&reserved->block_hash==block.header.hash(),"prior durable precommit lost");sign_vote(*reserved,key,1337,genesis.block.header.hash());certificate.push_back(*reserved);}
 }
 if(failing){
  check(reopened_height==0,"fault fixture starts at genesis");fs::path wal;for(const auto& entry:fs::directory_iterator(root/(boundary=="commit"?"chain":"journal")))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"WAL missing");auto before=fs::file_size(wal);nano_test_arm_sync_fault(wal.c_str());
  FinalityVote vote;vote.height=1;vote.step=VoteStep::prevote;vote.validator_set_hash=voting_set_hash(chain.voting_set());vote.validator=signer;vote.block_hash=block.header.hash();std::string error;
  try{if(boundary=="reservation")sign_consensus_vote(journal,vote,key,chain.voting_set(),{},1337,genesis.block.header.hash(),&block);else chain.commit_certified_round(journal,signer,block,0,certificate,12);}catch(const std::runtime_error& e){error=e.what();}
  check(nano_test_sync_failures()>0&&error.find("RocksDB:")!=std::string::npos,"expected observed RocksDB sync error: "+error);check(fs::file_size(wal)>before,"sync error must follow WAL append");
  check(chain.tip().header.number==(boundary=="progress"?1:0),"wrong live height at sync failure");if(boundary!="progress")check(chain.state().root()==genesis.state.root(),"failed canonical sync changed live state");else check(chain.state().get(recipient).balance==123,"durable canonical state missing at journal failure");
  Json result={{"walName",wal.filename().string()},{"walBytesBefore",before},{"error",error},{"syncFailures",nano_test_sync_failures()},{"walAppendedBytes",fs::file_size(wal)-before},{"blockHash",hex(block.header.hash())},{"liveHeight",quantity(chain.tip().header.number)}};
  if(boundary=="reservation"){check(vote.signature.empty(),"signature emitted despite sync error");result["signatureEmpty"]=true;}else result["precommitSignature"]=hex(certificate.at(0).signature);
  if(phase=="fail-retry"){
   auto faults=nano_test_sync_failures();auto height=chain.tip().header.number;auto state=chain.state().root();auto cursor=chain.event_cursor();auto pending=chain.pending_count();
   nano_test_disarm_sync_fault();std::string retry_error;
   try{
    if(boundary=="reservation")sign_consensus_vote(journal,vote,key,chain.voting_set(),{},1337,genesis.block.header.hash(),&block);
    else if(boundary=="commit")chain.commit_certified_round(journal,signer,block,0,certificate,12);
    else chain.recover_voting_progress(journal,signer);
   }catch(const std::runtime_error& e){retry_error=e.what();}
   check(!retry_error.empty()&&retry_error.find("RocksDB:")!=std::string::npos,"failed storage unexpectedly accepted in-process retry: "+retry_error);
   check(vote.signature.empty(),"in-process retry emitted uncertain signature");
   auto alternative=block;alternative.header.timestamp=13;sign_block(alternative,key,1337,genesis.block.header.hash());
   auto conflict=boundary=="reservation"?vote:certificate.at(0);conflict.signature.clear();conflict.block_hash=alternative.header.hash();std::string conflict_error;
   try{sign_consensus_vote(journal,conflict,key,chain.voting_set(),{},1337,genesis.block.header.hash(),&alternative);}catch(const std::runtime_error& e){conflict_error=e.what();}
   check(!conflict_error.empty()&&conflict.signature.empty(),"storage failure allowed contradictory vote");
   check(chain.tip().header.number==height&&chain.state().root()==state&&chain.event_cursor()==cursor&&chain.pending_count()==pending,"failed retry changed state events or pool");
   check(nano_test_sync_failures()==faults,"disabled sync injector fired during retry");
   result["retryRejected"]=true;result["retryError"]=retry_error;result["conflictingVoteRejected"]=true;result["conflictError"]=conflict_error;result["injectorDisabled"]=true;result["liveStateEventsPoolUnchanged"]=true;
   result["walAppendedBytes"]=fs::file_size(wal)-before;
  }
  std::cout<<result.dump()<<std::endl;_exit(0);
 }
 bool retained_reservation=false;
 if(boundary=="reservation"){
  auto saved=journal.latest_consensus_vote(signer,1337,genesis.block.header.hash());retained_reservation=saved.has_value();
  if(saved){check(saved->block_hash==block.header.hash(),"recovered reservation names another block");auto alternative=chain.block_template(signer,13);sign_block(alternative,key,1337,genesis.block.header.hash());auto conflicting=*saved;conflicting.block_hash=alternative.header.hash();conflicting.signature.clear();bool rejected=false;
   try{sign_consensus_vote(journal,conflicting,key,chain.voting_set(),{},1337,genesis.block.header.hash(),&alternative);}catch(const std::runtime_error& e){rejected=std::string(e.what())=="conflicting vote refused at height/round/step";}
   check(rejected&&conflicting.signature.empty(),"retained reservation did not prevent conflicting signature");}
  auto prevote=chain.vote_for_block(journal,key,block,0,VoteStep::prevote,{},12);certificate.push_back(chain.vote_for_block(journal,key,block,0,VoteStep::precommit,{prevote},12));
 }
 if(reopened_height==0)chain.commit_certified_round(journal,signer,block,0,certificate,12);else chain.recover_voting_progress(journal,signer);
 check(chain.state().get(recipient).balance==123,"recovery applied transfer incorrectly");auto receipt=chain.store().receipt(signed_tx.hash());check(receipt&&receipt->success,"recovered receipt missing");auto position=journal.voting_position(signer,1337,genesis.block.header.hash());check(position&&position->height==2,"journal recovery incorrect");
 std::cout<<Json{{"journalHeightBeforeRecovery",original_journal_height},{"reopenedHeight",quantity(reopened_height)},{"reservationRetained",retained_reservation},{"blockHash",hex(block.header.hash())},{"precommitSignature",hex(certificate.at(0).signature)},{"audit",chain.audit()}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
