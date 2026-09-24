#include "support/process.hpp"
#include "nano/chain.hpp"
#include "nano/block_auth.hpp"
#include <sys/resource.h>
#include <sys/stat.h>
#include <cerrno>
using namespace test;
int main(int argc,char** argv){try{
 check(argc==2||argc==6,"fork, or fork/directory/boundary/phase/mode required");
 std::string fork=argv[1];check(fork=="Paris"||fork=="Shanghai","explicit supported fork required");
 if(argc==2){
  Json records=Json::array();
  for(const std::string boundary:{"reservation","commit"}){
   Temp measured;auto measurement=success(run({argv[0],fork,measured.path.string(),boundary,"measure","zero"}));auto size=measurement.at("appendBytes").get<uint64_t>();check(size>64,"control WAL append unexpectedly small");
   for(const auto budget:std::vector<uint64_t>{0,1,6,7,32,size/2,size-1}){auto mode=budget?std::to_string(budget):std::string("zero");Temp root;
   auto failed=success(run({argv[0],fork,root.path.string(),boundary,"fail",mode}));
   check(failed.at("writeRejected")==true&&failed.at("height")=="0x0","OS write failure was not observed before state advance");
   auto recovered=success(run({argv[0],fork,root.path.string(),boundary,"recover",mode}));
   check(recovered.at("height")=="0x1"&&recovered.at("audit").at("verified")==true,"recovery did not produce audited canonical block");
   if(boundary=="commit")check(failed.at("reservedSignature")==recovered.at("reservedSignature"),"recovery changed already durable precommit signature");
   records.push_back({{"mode",mode},{"controlAppendBytes",size},{"boundary",boundary},{"failed",failed},{"recovered",recovered}});
  }
  }
  std::cout<<Json{{"fork",fork},{"fault","RLIMIT_FSIZE/EFBIG"},{"records",records}}.dump()<<std::endl;return 0;
 }
 fs::path root=argv[2];std::string boundary=argv[3],phase=argv[4],mode=argv[5];
 const bool enospc=mode=="enospc";uint64_t budget=mode=="zero"||enospc?0:std::stoull(mode);
 check(boundary=="reservation"||boundary=="commit","invalid boundary");check(phase=="fail"||phase=="recover"||phase=="measure"||phase=="verify","invalid phase");
 auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));auto recipient=public_address(public_key(bytes(Int(2),32)));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"evmFork",fork},{"bootstrapValidator",signer}}},{"alloc",{{signer,{{"balance","1000000000000000000000"}}}}}});
 Blockchain chain(root/"chain",genesis);ChainStore journal(root/"journal");
 Transaction tx;tx.to=recipient;tx.value=123;tx.gas=21000;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,key);
 if(phase=="verify"){
  auto receipt=chain.store().receipt(signed_tx.hash());auto position=journal.voting_position(signer,1337,genesis.block.header.hash());
  check(chain.tip().header.number==1&&chain.state().get(recipient).balance==123&&chain.state().get(signer).nonce==1,"reopened recovery applied transfer incorrectly");
  check(receipt&&receipt->success&&chain.store().certificate_at(1)&&position&&position->height==2,"reopened recovery lost receipt certificate or voting position");
  auto audit=chain.audit();check(audit.at("verified")==true,"reopened recovery audit failed");
  std::cout<<Json{{"height","0x1"},{"recipientBalance","0x7b"},{"senderNonce","0x1"},{"audit",audit}}.dump()<<std::endl;return 0;
 }
 check(chain.tip().header.number==0&&chain.state().root()==genesis.state.root(),"failed write advanced canonical state after reopen");
 check(!chain.store().receipt(signed_tx.hash())&&!chain.store().block_by_number(1)&&!chain.store().certificate_at(1),"failed write left partial canonical indexes");
 check(chain.submit(signed_tx),"transfer admitted");auto block=chain.block_template(signer,12);sign_block(block,key,1337,genesis.block.header.hash());
 std::vector<FinalityVote> certificate;
 if(phase=="recover"&&boundary=="commit"){
  auto resumed=chain.resume_consensus_vote(journal,key,12);check(resumed&&resumed->step==VoteStep::precommit&&resumed->block_hash==block.header.hash(),"durable precommit lost after failed canonical write");certificate.push_back(*resumed);
 }else if(boundary=="commit"||phase=="recover"){
  if(phase=="recover")check(!journal.latest_consensus_vote(signer,1337,genesis.block.header.hash()),"failed reservation became durable");
  auto prevote=chain.vote_for_block(journal,key,block,0,VoteStep::prevote,{},12);
  certificate.push_back(chain.vote_for_block(journal,key,block,0,VoteStep::precommit,{prevote},12));
 }
 if(phase=="fail"||phase=="measure"){
  fs::path wal;for(const auto& entry:fs::directory_iterator(root/(boundary=="reservation"?"journal":"chain")))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();
  check(!wal.empty(),"active WAL file missing");auto initial_size=fs::file_size(wal);
  struct rlimit original{};check(getrlimit(RLIMIT_FSIZE,&original)==0,"read file size limit");auto limited=original;limited.rlim_cur=initial_size+budget;
  if(phase=="fail"&&!enospc)check(signal(SIGXFSZ,SIG_IGN)!=SIG_ERR&&setrlimit(RLIMIT_FSIZE,&limited)==0,"install child-only file write limit");
  uint64_t filler_bytes=0;
  if(phase=="fail"&&enospc){
   // Refuse the host filesystem and any volume larger than the bounded fixture.
   struct stat target{},host{};check(stat(root.c_str(),&target)==0&&stat("/",&host)==0&&target.st_dev!=host.st_dev,"ENOSPC requires an isolated mounted volume");
   check(fs::space(root).capacity<=128*1024*1024,"ENOSPC volume exceeds 128 MiB guard");
   int fd=open((root/"enospc-filler").c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);check(fd>=0,"create isolated filler");
   std::vector<char> zeros(65536,0);
   for(size_t chunk:{size_t(65536),size_t(4096),size_t(1)}){
    for(;;){auto n=write(fd,zeros.data(),chunk);if(n<0){if(errno==EINTR)continue;check(errno==ENOSPC,"filler failed without ENOSPC");break;}check(n>0,"zero-length filler write");filler_bytes+=n;check(filler_bytes<=128*1024*1024,"filler exceeded safety cap");}
   }
   close(fd);
  }
  FinalityVote vote;vote.height=1;vote.round=0;vote.step=VoteStep::prevote;vote.validator_set_hash=voting_set_hash(chain.voting_set());vote.validator=signer;vote.block_hash=block.header.hash();
  std::string error;
  try{
   if(boundary=="reservation")sign_consensus_vote(journal,vote,key,chain.voting_set(),{},1337,genesis.block.header.hash(),&block);
   else chain.commit_certified_round(journal,signer,block,0,certificate,12);
  }catch(const std::runtime_error& e){error=e.what();}
  if(phase=="measure"){check(error.empty(),"control append failed: "+error);std::cout<<Json{{"appendBytes",fs::file_size(wal)-initial_size}}.dump()<<std::endl;return 0;}
  // Keep the failing limit installed until process exit: no destructor flush may
  // accidentally turn the failed operation into a successful later write.
  check(error.find("RocksDB:")!=std::string::npos&&error.find(enospc?"No space left on device":"File too large")!=std::string::npos,"expected RocksDB OS write error, received: "+error);
  check(error.find(boundary=="reservation"?"/journal/":"/chain/")!=std::string::npos,"error came from wrong database");
  auto final_size=fs::file_size(wal);check(enospc?final_size>=initial_size:final_size==initial_size+budget,"WAL did not contain the requested partial append");
  check(vote.signature.empty(),"signature escaped before durable reservation");
  check(chain.tip().header.number==0&&chain.state().root()==genesis.state.root(),"failed commit changed live chain state");
  Json result={{"fault",enospc?"ENOSPC":"EFBIG"},{"fillerBytes",filler_bytes},{"writeRejected",true},{"walBytesBefore",initial_size},{"walBytesAfter",final_size},{"partialBytes",final_size-initial_size},{"error",error},{"height",quantity(chain.tip().header.number)}};
  if(boundary=="reservation")result["signatureEmpty"]=vote.signature.empty();
  else{auto position=journal.voting_position(signer,1337,genesis.block.header.hash());check(position&&position->height==1,"journal advanced despite failed canonical write");result["reservedSignature"]=hex(certificate.at(0).signature);}
  std::cout<<result.dump()<<std::endl;
  _exit(0);
 }
 chain.commit_certified_round(journal,signer,block,0,certificate,12);
 check(chain.state().get(recipient).balance==123,"recovered transfer applied once");auto receipt=chain.store().receipt(signed_tx.hash());check(receipt&&receipt->success,"recovered receipt missing");
 auto position=journal.voting_position(signer,1337,genesis.block.header.hash());check(position&&position->height==2,"journal did not advance after successful retry");
 std::cout<<Json{{"height",quantity(chain.tip().header.number)},{"blockHash",hex(block.header.hash())},{"audit",chain.audit()},{"reservedSignature",hex(certificate.at(0).signature)}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
