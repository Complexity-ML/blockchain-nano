#include "support/rocksdb.hpp"
#include "support/process.hpp"
#include "nano/rpc.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include <rocksdb/db.h>
using namespace test;
int main(){try{
 Temp root;auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));
 std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Paris";
 check(fork=="Paris"||fork=="Shanghai","explicit network profile");
 auto genesis=Genesis::from_json({{"gasLimit","0x2625a00"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 Blockchain chain(root/"producer",genesis);
 for(unsigned nonce=0;nonce<10;++nonce){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.gas=4000000;tx.gas_price=2000000000;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.data=Bytes(900000,0);auto signed_tx=SignedTransaction::sign(tx,key);check(signed_tx.raw().size()<1024*1024,"transaction below pool byte ceiling");check(chain.submit(signed_tx),"transaction admitted");}
 auto block=chain.block_template(signer,12,0);check(block.transactions.size()==10,"all ten fit gas budget");sign_block(block,key,1337,genesis.block.header.hash());chain.verify_authentication(block);chain.verify_proposal(block,12);

 FinalityVote reserved;
 {ChainStore journal(root/"journal");reserved=chain.vote_for_block(journal,key,block,0,VoteStep::prevote,{},12);}
 check(chain.tip().header.number==0,"reservation alone cannot publish a block");
 const auto prefix="voting/v1/"+hex(bytes(Int(1337),32))+"/"+hex(genesis.block.header.hash())+"/"+address(signer)+"/";
 const auto proposal_key=prefix+"proposal/"+hex(bytes(Int(1),8))+"/"+hex(block.header.hash());
 const std::string tag="nano/proposal/chunks-v1\n";
 auto edit=[&](const std::function<void(rocksdb::DB&)>& callback){std::unique_ptr<rocksdb::DB> db;rocksdb::Options options;check(test::open_rocksdb(options,(root/"journal").string(),db).ok(),"raw journal open");callback(*db);};
 auto read_raw=[](rocksdb::DB& db,const std::string& name){std::string value;check(db.Get(rocksdb::ReadOptions(),name,&value).ok(),"raw journal key exists");return value;};
 std::string manifest,watermark;std::vector<std::string> chunks;
 edit([&](rocksdb::DB& db){manifest=read_raw(db,proposal_key);watermark=read_raw(db,prefix+"watermark");check(manifest.starts_with(tag),"large proposal uses chunk manifest");auto metadata=Json::parse(manifest.substr(tag.size()));auto count=metadata.at("chunks").get<size_t>();check(count==18,"expected bounded chunk count");size_t total=0;for(size_t i=0;i<count;++i){auto value=read_raw(db,proposal_key+"/chunks/"+std::to_string(i));check(value.size()<=1024*1024,"chunk byte limit");total+=value.size();chunks.push_back(std::move(value));}check(total==metadata.at("bytes")&&total>16*1024*1024,"complete oversized payload is stored");});
 rocksdb::WriteOptions durable;durable.sync=true;
 for(unsigned mode=0;mode<8;++mode){
  edit([&](rocksdb::DB& db){auto put=[&](const std::string& name,const std::string& value){check(db.Put(durable,name,value).ok(),"write corruption fixture");};auto chunk0=proposal_key+"/chunks/0";auto last=proposal_key+"/chunks/"+std::to_string(chunks.size()-1);auto metadata=Json::parse(manifest.substr(tag.size()));
   if(mode==0)check(db.Delete(durable,chunk0).ok(),"delete fragment fixture");
   if(mode==1){auto bad=chunks[0];bad[0]^=1;put(chunk0,bad);}
   if(mode==2)put(last,chunks.back().substr(1));
   if(mode==3){metadata["chunks"]=chunks.size()+1;put(proposal_key,tag+metadata.dump());}
   if(mode==4){metadata["keccak"]="0x"+std::string(64,'0');put(proposal_key,tag+metadata.dump());}
   if(mode==5)put(proposal_key,"nano/proposal/chunks-v2\n"+metadata.dump());
   if(mode==6){metadata["bytes"]=std::numeric_limits<uint64_t>::max();metadata["chunks"]=(std::numeric_limits<uint64_t>::max()-1)/(1024*1024)+1;put(proposal_key,tag+metadata.dump());}
   if(mode==7)check(db.Delete(durable,proposal_key).ok(),"delete manifest fixture");
  });
  bool refused=false;{ChainStore journal(root/"journal");try{chain.resume_consensus_vote(journal,key,12);}catch(const std::exception&){refused=true;}}
  check(refused&&chain.tip().header.number==0,"corrupt or incomplete proposal cannot resume signing or commit");
  edit([&](rocksdb::DB& db){check(read_raw(db,prefix+"watermark")==watermark,"rejected recovery changed vote watermark");check(db.Put(durable,proposal_key,manifest).ok(),"restore manifest");for(size_t i=0;i<chunks.size();++i)check(db.Put(durable,proposal_key+"/chunks/"+std::to_string(i),chunks[i]).ok(),"restore chunk");});
 }
 {ChainStore journal(root/"journal");auto recovered=chain.resume_consensus_vote(journal,key,12);check(recovered&&recovered->signature==reserved.signature,"reopened fragments recover exact reserved signature");auto loaded=journal.voting_proposal(signer,1,block.header.hash(),1337,genesis.block.header.hash());check(loaded&&loaded->to_transport_json()==block.to_transport_json(),"reconstructed signed bytes and header match");ConsensusRound round(chain,journal,key,0,ConsensusRound::Clock::time_point{},1s,1s,1s);check(round.advance(ConsensusRound::Clock::time_point{},12).committed,"recovered large proposal reaches finality");}
 check(chain.tip().header.hash()==block.header.hash()&&chain.audit().at("verified")==true,"recovered large canonical history audits");
 edit([&](rocksdb::DB& db){check(read_raw(db,proposal_key)==manifest,"precommit silently rewrote proposal manifest");for(size_t i=0;i<chunks.size();++i)check(read_raw(db,proposal_key+"/chunks/"+std::to_string(i))==chunks[i],"precommit silently rewrote proposal bytes");});
 std::cout<<Json{{"scenario","chunked-voting-proposal"},{"fork",fork},{"chunks",chunks.size()},{"compactBytes",block.to_transport_json().dump().size()},{"corruptionsRejected",8},{"exactSignatureResumed",true},{"canonicalAudit",true},{"proposalBytesPreserved",true}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
