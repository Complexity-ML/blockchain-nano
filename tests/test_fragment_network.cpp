#include "support/process.hpp"
#include "nano/rpc.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "../network/json_frame.hpp"
using namespace test;
int main(int argc,char** argv){try{
 check(argc==3||argc==4,"core and relay executables required");const bool faults=argc==4&&std::string(argv[3])=="--faults";check(argc==3||faults,"unknown network fixture option");
 Temp root;auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));
 std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Paris";
 check(fork=="Paris"||fork=="Shanghai","explicit network profile");
 auto genesis=Genesis::from_json({{"gasLimit","0x2625a00"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 Json expected;std::vector<std::string> tx_hashes;
 {Blockchain chain(root/"producer",genesis);ChainStore journal(root/"journal");
 for(unsigned nonce=0;nonce<10;++nonce){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.gas=4000000;tx.gas_price=2000000000;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.data=Bytes(900000,0);auto signed_tx=SignedTransaction::sign(tx,key);check(signed_tx.raw().size()<1024*1024,"transaction below pool byte ceiling");check(chain.submit(signed_tx),"transaction admitted");}
 auto block=chain.block_template(signer,12,0);check(block.transactions.size()==10,"all ten fit gas budget");sign_block(block,key,1337,genesis.block.header.hash());chain.verify_authentication(block);chain.verify_proposal(block,12);

 ConsensusRound round(chain,journal,key,0,ConsensusRound::Clock::time_point{},1s,1s,1s);round.propose(block,12);check(round.advance(ConsensusRound::Clock::time_point{},12).committed,"source uses real local journal-backed finality");check(chain.audit().at("verified")==true,"source history audits before network startup");
 expected={{"hash",hex(block.header.hash())},{"stateRoot",hex(block.header.state_root)},{"compactBytes",block.to_transport_json().dump().size()}};check(expected.at("compactBytes").get<size_t>()>16*1024*1024,"fixture requires fragmented transport");for(const auto& tx:block.transactions)tx_hashes.push_back(hex(tx.hash()));
 }
 auto genesis_path=root/"genesis.json";save(genesis_path,{{"gasLimit","0x2625a00"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 auto config=[&](const std::string& name,Json peers){auto file=root/(name+".json");save(file,{{"rpc_port",0},{"rpc_host","127.0.0.1"},{"p2p_port",0},{"p2p_host","127.0.0.1"},{"p2p_binary",argv[2]},{"bootnodes",peers}});return file.string();};
 Process source({argv[1],"run",genesis_path.string(),(root/"producer").string(),config("source",Json::array())},Env{{"NANO_TEST_NETWORK_TRACE","1"},{"NANO_TEST_DROP_BLOCK_FRAGMENT_ONCE",faults?std::optional<std::string>("1"):std::nullopt},{"NANO_TEST_DROP_BLOCK_IMPORT_ACK_ONCE",std::nullopt}});auto source_port=source.port();auto address=source.event("listen").at("address").get<std::string>();auto source_child=source.event("p2p-process").at("pid").get<int>();source.event("ready");
 bool aggregate_refused=false;try{rpc(source_port,"nano_networkBlocks",Json::array({0}));}catch(const std::exception& e){aggregate_refused=std::string(e.what()).find("single-page size limit")!=std::string::npos;}check(aggregate_refused,"legacy aggregate route must not carry this fixture");
 Process follower({argv[1],"run",genesis_path.string(),(root/"follower").string(),config("follower",Json::array({address}))},Env{{"NANO_TEST_NETWORK_TRACE","1"},{"NANO_TEST_DROP_BLOCK_FRAGMENT_ONCE",std::nullopt},{"NANO_TEST_DROP_BLOCK_IMPORT_ACK_ONCE",faults?std::optional<std::string>("1"):std::nullopt}});auto follower_port=follower.port();auto follower_child=follower.event("p2p-process").at("pid").get<int>();follower.event("ready");
 until([&]{check(source.running()&&follower.running(),"managed nodes remain alive during transfer");return rpc(follower_port,"eth_blockNumber")=="0x1";},"Follower catches up oversized certified block via real libp2p",90);
 auto transfer=follower.event("test-block-fragments-imported");check(transfer.at("height")==1&&transfer.at("blockHash")==expected.at("hash")&&transfer.at("fragments")==40,"relay trace proves forty fragments and verified local import");
 Json fault_evidence=nullptr;
 if(faults){auto dropped=source.event("test-block-fragment-dropped");auto failed=follower.event("test-block-fragments-failed");auto ack=follower.event("test-block-import-ack-dropped");
  check(dropped.at("blockHash")==expected.at("hash")&&dropped.at("offset")=="0x40000","second fragment was actually dropped");check(failed.at("fragments")==1,"first attempt failed after one accepted fragment");check(ack.at("blockHash")==expected.at("hash")&&transfer.at("finishReplyMissing")==true,"actual lost HTTP commit reply was recovered by canonical hash lookup");fault_evidence={{"droppedFragment",dropped},{"failedAttempt",failed},{"droppedCommitReply",ack}};
 }
 auto final=rpc(follower_port,"eth_getBlockByNumber",Json::array({"0x1",false}));check(final.at("hash")==expected.at("hash")&&final.at("stateRoot")==expected.at("stateRoot"),"follower canonical commitments match");
 for(const auto& hash:tx_hashes){auto receipt=rpc(follower_port,"eth_getTransactionReceipt",Json::array({hash}));check(receipt.at("status")=="0x1"&&receipt==rpc(source_port,"eth_getTransactionReceipt",Json::array({hash})),"fragmented catch-up receipt mismatch");}
 check(rpc(follower_port,"eth_getBalance",Json::array({"0x00000000000000000000000000000000000000b0","latest"}))=="0xa","transfers applied exactly once");
 check(follower.stop().code==0&&source.stop().code==0,"managed nodes stop cleanly");check(kill(source_child,0)<0&&errno==ESRCH&&kill(follower_child,0)<0&&errno==ESRCH,"relays reaped");
 if(faults){auto count=[](const std::string& output,const std::string& event){size_t n=0;std::istringstream lines(output);std::string line;while(std::getline(lines,line)){auto row=Json::parse(line,nullptr,false);if(row.is_object()&&row.value("event","")==event)++n;}return n;};check(count(source.output(),"test-block-fragment-dropped")==1&&count(follower.output(),"test-block-import-ack-dropped")==1&&count(follower.output(),"test-block-fragments-failed")==1&&count(follower.output(),"test-block-fragments-imported")==1,"fault injection and successful commit must each occur exactly once");}
 for(const auto& name:{"producer","follower"}){Blockchain reopened(root/name,genesis);check(hex(reopened.tip().header.hash())==expected.at("hash").get<std::string>()&&reopened.audit().at("verified")==true,"reopened fragmented history audits");}
 std::cout<<Json{{"faults",faults},{"faultEvidence",fault_evidence},{"scenario",faults?"libp2p-fragment-faults":"libp2p-fragment-sync"},{"fork",fork},{"compactBytes",expected.at("compactBytes")},{"fragments",40},{"managedNodes",2},{"reopenedAudits",2},{"receiptCount",tx_hashes.size()},{"aggregateRouteRefused",true},{"blockHash",expected.at("hash")},{"stateRoot",expected.at("stateRoot")},{"distributedProposalQualified",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
