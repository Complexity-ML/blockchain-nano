#include "nano/rpc.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "support/process.hpp"
using namespace test;
int main(){try{
 Temp root;const auto key=bytes(Int(1),32);const auto signer=public_address(public_key(key));
 const std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Paris";
 check(fork=="Paris"||fork=="Shanghai","explicit supported network profile required");
 const size_t data_bytes=std::getenv("NANO_TEST_BLOCK_DATA_BYTES")?std::stoul(std::getenv("NANO_TEST_BLOCK_DATA_BYTES")):450000;
 check(data_bytes==450000||data_bytes==900000,"bounded fixture sizes required");
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 Blockchain producer(root/"producer",genesis);ChainStore journal(root/"journal");
 for(unsigned nonce=0;nonce<5;++nonce){
  Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.to="0x00000000000000000000000000000000000000b0";tx.gas=4000000;tx.gas_price=2000000000;tx.data=Bytes(data_bytes,0);
  auto signed_tx=SignedTransaction::sign(tx,key);check(signed_tx.raw().size()<1024*1024,"fixture transaction respects pool byte limit");check(producer.submit(signed_tx),"large valid transaction admitted");
 }
 auto block=producer.block_template(signer,12,0);check(block.transactions.size()==5,"all five transactions fit the block gas limit");sign_block(block,key,1337,genesis.block.header.hash());
 auto now=ConsensusRound::Clock::time_point{};ConsensusRound round(producer,journal,key,0,now,1s,1s,1s);round.propose(block,12);
 std::cout<<Json{{"stage","large-block-candidate"},{"fork",fork},{"dataBytesPerTransaction",data_bytes},{"transactions",5},{"blockJsonBytes",block.to_json(true).dump().size()},{"blockGasUsed",quantity(block.header.gas_used)},{"blockGasLimit",quantity(block.header.gas_limit)},{"proposalValidated",true}}.dump()<<std::endl;
 check(round.advance(now,12).committed,"large block receives a valid BFT certificate");
 check(producer.audit().at("verified")==true,"large canonical block replays correctly");
 Rpc rpc(producer);auto reply=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","nano_networkBlocks"},{"params",Json::array({0})}});
 check(reply.contains("result"),"block page RPC failed");auto page=reply.at("result");
 auto block_bytes=block.to_json(true).dump().size();check(block_bytes>8*1024*1024,"fixture must exceed the old page budget");
 std::cout<<Json{{"stage","large-block-page"},{"fork",fork},{"transactions",5},{"blockJsonBytes",block_bytes},{"blockGasUsed",quantity(block.header.gas_used)},{"blockGasLimit",quantity(block.header.gas_limit)},{"certifiedAndAudited",true},{"returnedBlocks",page.at("blocks").size()},{"pageBytes",reply.dump().size()}}.dump()<<std::endl;
 check(page.at("blocks").size()==1,"valid large first block must not produce an empty synchronization page");
 uint64_t cursor=0;unsigned event_pages=0;bool event_block=false;
 while(!event_block&&event_pages<10){
  auto events=rpc.handle({{"jsonrpc","2.0"},{"id",3},{"method","nano_networkEvents"},{"params",Json::array({cursor})}}).at("result");
  auto next=events.at("cursor").get<uint64_t>();check(next>cursor,"large block event stalls the synchronization cursor");cursor=next;++event_pages;
  for(const auto& event:events.at("events"))if(event.at("type")=="block"){
   check(Block::from_json(event.at("value")).header.hash()==block.header.hash(),"announced large block differs");event_block=true;
  }
 }
 check(event_block,"large certified block is absent from bounded event pages");
 Blockchain follower(root/"follower",genesis);Rpc follower_rpc(follower);
 auto imported=follower_rpc.handle({{"jsonrpc","2.0"},{"id",2},{"method","validator_submitBlock"},{"params",Json::array({page.at("blocks").at(0)})}});
 check(imported.at("result").at("success")==true,"follower imports large certified block");
 check(follower.tip().header.hash()==producer.tip().header.hash()&&follower.state().root()==producer.state().root(),"follower canonical state matches producer");
 check(follower.audit().at("verified")==true,"imported history audit succeeds");
 std::cout<<Json{{"scenario","large-block-sync"},{"fork",fork},{"blockHash",hex(block.header.hash())},{"stateRoot",hex(block.header.state_root)},{"followerAudited",true},{"eventPages",event_pages},{"eventBlockFound",event_block}}.dump()<<std::endl;
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
