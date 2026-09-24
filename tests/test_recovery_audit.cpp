#include "support/process.hpp"
#include "nano/chain.hpp"
#include "nano/block_auth.hpp"
#include <rocksdb/db.h>
using namespace test;
int main(int argc,char** argv){try{
 check(argc==2,"explicit fork required");std::string fork=argv[1];check(fork=="Paris"||fork=="Shanghai","supported fork required");Temp root;
 auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));auto recipient=public_address(public_key(bytes(Int(2),32)));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"evmFork",fork},{"bootstrapValidator",signer}}},{"alloc",{{signer,{{"balance","1000000000000000000000"}}}}}});
 std::vector<Block> blocks;std::vector<std::vector<FinalityVote>> certificates;Bytes first_tx;
 {Blockchain chain(root/"source",genesis);ChainStore journal(root/"source-journal");chain.recover_voting_progress(journal,signer);
  for(unsigned n=0;n<2;++n){Transaction tx;tx.nonce=n;tx.to=recipient;tx.value=123;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,key);if(n==0)first_tx=signed_tx.hash();check(chain.submit(signed_tx),"submit fixture transfer");auto block=chain.block_template(signer,Int((n+1)*12));sign_block(block,key,1337,genesis.block.header.hash());auto prevote=chain.vote_for_block(journal,key,block,0,VoteStep::prevote,{},Int((n+1)*12));auto precommit=chain.vote_for_block(journal,key,block,0,VoteStep::precommit,{prevote},Int((n+1)*12));
   chain.commit_certified_round(journal,signer,block,0,{precommit},Int((n+1)*12));chain.recover_voting_progress(journal,signer);blocks.push_back(block);certificates.push_back({precommit});}
  check(chain.state().get(recipient).balance==246&&chain.audit().at("verified")==true,"verified in-process extensions failed");
 }
 for(const std::string scenario:{"reopen","unverified-extension"}){
  auto path=root/scenario;
  {Blockchain chain(path,genesis);chain.import_certified_block(blocks[0],0,certificates[0],12);if(scenario=="reopen"){ChainStore journal(root/"old-journal");chain.recover_voting_progress(journal,signer);chain.import_certified_block(blocks[1],0,certificates[1],24);chain.recover_voting_progress(journal,signer);}}
  // Keep a valid certified tip while corrupting an older receipt, which only
  // complete replay can detect. The original DB owner is already destroyed.
  {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(rocksdb::DB::Open(options,path.string(),&db).ok(),"open closed corruption fixture");std::string value;auto storage_key="receipt/"+hex(first_tx);check(db->Get(rocksdb::ReadOptions(),storage_key,&value).ok(),"read historical receipt");auto receipt=Json::parse(value);receipt["gasUsed"]="0x1";rocksdb::WriteOptions durable;durable.sync=true;check(db->Put(durable,storage_key,receipt.dump()).ok(),"persist historical receipt corruption");}
  {Blockchain reopened(path,genesis);if(scenario=="unverified-extension")reopened.import_certified_block(blocks[1],0,certificates[1],24);
   ChainStore journal(root/(scenario+"-journal"));bool refused=false;try{reopened.recover_voting_progress(journal,signer);}catch(const std::runtime_error& e){refused=std::string(e.what()).find("receipt differs from replay")!=std::string::npos;}
   check(refused,"recovery authorized corrupted historical prefix");check(!journal.voting_position(signer,1337,genesis.block.header.hash()),"failed audit advanced fresh voting journal");
  }
 }
 std::cout<<Json{{"fork",fork},{"verifiedExtensions",2},{"corruptReopenRejected",true},{"unverifiedPrefixExtensionRejected",true},{"journalUnchangedOnFailure",true}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
