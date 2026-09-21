#include "nano/chain.hpp"
#include "nano/block_auth.hpp"
#include "nano/crypto.hpp"
#include <iostream>
#include <unistd.h>
using namespace nano;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::invalid_argument("phase and fixture directory required");
 const std::string phase=argv[1];const std::filesystem::path root=argv[2];
 auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));
 auto recipient=public_address(public_key(bytes(Int(2),32)));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",signer}}},{"alloc",{{signer,{{"balance","1000000000000000000000"}}}}}});
 Blockchain chain(root/"chain",genesis);ChainStore journal(root/"journal");
 auto position=[&](){auto p=journal.voting_position(signer,1337,genesis.block.header.hash());return p?p->height:0;};
 Json result={{"phase",phase}};
 if(phase=="reserve"||phase=="commit"){
  require(chain.tip().header.number==0,"fixture must start before first commit");
  Transaction tx;tx.to=recipient;tx.value=123;tx.gas=21000;tx.gas_price=2000000000;
  require(chain.submit(SignedTransaction::sign(tx,key)),"submit fixture transfer");
  auto block=chain.block_template(signer,12);sign_block(block,key,1337,genesis.block.header.hash());
  if(phase=="commit"){
   auto locked=journal.voting_lock(signer,1337,genesis.block.header.hash());
   require(locked&&locked->block_hash,"locked proposal identity missing after SIGKILL");
   auto restored=journal.voting_proposal(signer,locked->height,*locked->block_hash,1337,genesis.block.header.hash());
   require(restored&&restored->to_json(true)==block.to_json(true),"voted block content missing after SIGKILL");
   chain.verify_authentication(*restored);chain.verify_proposal(*restored,12);block=*restored;
   auto reserved=journal.latest_consensus_vote(signer,1337,genesis.block.header.hash());
   require(reserved&&reserved->signature.empty()&&reserved->step==VoteStep::precommit&&reserved->block_hash==block.header.hash(),"reserved vote fields missing after SIGKILL");
   reserved=chain.resume_consensus_vote(journal,key,12);
   require(reserved.has_value(),"chain failed to recover durable vote after SIGKILL");
   require(verify_vote(*reserved,1337,genesis.block.header.hash()),"recovered reservation cannot reproduce valid signature");
   result["recoveredSignature"]=hex(reserved->signature);
  }
  auto prevote=chain.vote_for_block(journal,key,block,0,VoteStep::prevote,{},12);
  RoundVoteCollector votes(chain.voting_set(),1337,genesis.block.header.hash(),1,0);
  votes.receive(encode_vote_message(prevote,1337,genesis.block.header.hash()));
  auto precommit=chain.vote_for_block(journal,key,block,0,VoteStep::precommit,votes.certificate(VoteStep::prevote,block.header.hash()),12);
  votes.receive(encode_vote_message(precommit,1337,genesis.block.header.hash()));
  if(phase=="commit")chain.import_certified_block(block,0,votes.certificate(VoteStep::precommit,block.header.hash()),12);
  require(position()==1,"journal must not advance before recovery");
  result["blockHash"]=hex(block.header.hash());result["signature"]=hex(precommit.signature);
 }else if(phase=="recover"||phase=="verify"){
  require(chain.tip().header.number==1,"certified first block missing after abrupt stop");
  require(chain.state().get(recipient).balance==123,"committed transfer state missing");
  require(chain.store().certificate_at(1).has_value(),"canonical certificate missing");
  chain.recover_voting_progress(journal,signer);require(position()==2,"journal did not catch up");
  if(phase=="verify"){
   Transaction tx;tx.nonce=1;tx.to=recipient;tx.value=456;tx.gas=21000;tx.gas_price=2000000000;
   auto signed_tx=SignedTransaction::sign(tx,key);require(chain.submit(signed_tx),"submit second transfer");
   auto block=chain.block_template(signer,24);sign_block(block,key,1337,genesis.block.header.hash());
   auto prevote=chain.vote_for_block(journal,key,block,0,VoteStep::prevote,{},24);
   RoundVoteCollector votes(chain.voting_set(),1337,genesis.block.header.hash(),2,0);
   votes.receive(encode_vote_message(prevote,1337,genesis.block.header.hash()));
   auto precommit=chain.vote_for_block(journal,key,block,0,VoteStep::precommit,votes.certificate(VoteStep::prevote,block.header.hash()),24);
   votes.receive(encode_vote_message(precommit,1337,genesis.block.header.hash()));
   chain.commit_certified_round(journal,signer,block,0,votes.certificate(VoteStep::precommit,block.header.hash()),24);
   require(position()==3&&chain.state().get(recipient).balance==579,"second commit or voting progression incorrect");
   auto receipt=chain.store().receipt(signed_tx.hash());require(receipt&&receipt->success,"second transaction receipt missing");
   auto audit=chain.audit();require(audit.at("certifiedBlocks")=="0x2","certified audit count mismatch");result["audit"]=audit;
  }
 }else throw std::invalid_argument("unknown fixture phase");
 result["height"]=quantity(chain.tip().header.number);result["votingHeight"]=position();
 std::cout<<result.dump()<<std::endl;
 // Deliberately keep both RocksDB instances open. The supervisor kills only
 // this fixture process after reading its durable-phase acknowledgement.
 if(phase!="verify")for(;;)pause();
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
