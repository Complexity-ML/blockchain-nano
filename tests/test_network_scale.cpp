#include "support/process.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
using namespace test;
int main(int argc,char** argv){try{
 check(argc==4,"core, transport and node count required");
 std::string core=argv[1],network=argv[2],arg=argv[3];check(!arg.empty()&&arg.find_first_not_of("0123456789")==std::string::npos,"decimal count required");
 auto count=std::stoul(arg);check(count>=2&&count<=10000,"test harness budget 2..10000 nodes");
 Temp root;auto started=std::chrono::steady_clock::now();
 auto fixture=load(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/network.json");std::string sender=fixture["sender"];
 Json config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",address(account)},{"bftVersion",1},{"evmFork","Shanghai"}};
 auto genesis=root/"genesis.json";save(genesis,{{"config",config},{"alloc",{{sender,{{"balance",quantity(Int("10000000000000000000000"))}}}}}});
 std::vector<std::unique_ptr<Process>> cores,relays;std::vector<int> ports;std::vector<std::string> seeds;
 for(unsigned i=0;i<count;++i){cores.push_back(std::make_unique<Process>(std::vector<std::string>{core,"serve",genesis.string(),(root/("node-"+std::to_string(i))).string(),"0","127.0.0.1"}));ports.push_back(cores.back()->port());}
 int a=ports[0];bool signed_mode=true;auto validator=root/"unused-validator.json";
 auto bft_chain=std::make_unique<Blockchain>(root/"producer",Genesis::from_json(load(genesis)));
 auto bft_journal=std::make_unique<ChainStore>(root/"journal");
 auto produce=[&](){Json result;if(true){
  auto candidate=Block::from_json(rpc(a,"validator_getBlockTemplate",{account}));
  sign_block(candidate,unhex(key()),1337,bft_chain->genesis().block.header.hash());
  auto now=ConsensusRound::Clock::now();ConsensusRound round(*bft_chain,*bft_journal,unhex(key()),0,now,1s,1s,1s);
  round.propose(candidate,candidate.header.timestamp);check(round.advance(now,candidate.header.timestamp).committed,"local BFT fixture failed to certify");
  auto certificate=bft_chain->store().certificate_at(candidate.header.number);Json votes=Json::array();for(const auto& vote:certificate->votes)votes.push_back(encode_vote_message(vote,1337,bft_chain->genesis().block.header.hash()));
  auto payload=candidate.to_json(true);payload["bftCertificate"]={{"version",1},{"round","0x0"},{"votes",votes}};result=rpc(a,"validator_submitBlock",{payload});
 }else if(signed_mode)result=success(run({core,"validator",validator.string(),"--once"},{{"VALIDATOR_KEY",key()}}));else result=rpc(a,"validator_submitBlock",{rpc(a,"validator_getBlockTemplate",{bootstrap})});check(result["success"],"block production failed");};
 produce();produce();produce();
 // Bounded-degree bootstrap tree; real libp2p identities and TCP transports.
 for(unsigned i=0;i<count;++i){std::vector<std::string> args{network,std::to_string(ports[i]),(root/("peer-"+std::to_string(i))).string(),"/ip4/127.0.0.1/tcp/0"};if(i)args.push_back(seeds[(i-1)/2]);relays.push_back(std::make_unique<Process>(args));seeds.push_back(relays.back()->event("listen")["address"]);relays.back()->event("ready");}
 auto all_at=[&](const char* height){for(unsigned i=0;i<count;++i){check(cores[i]->running()&&relays[i]->running(),"node exited");if(rpc(ports[i],"eth_blockNumber")!=height)return false;}return true;};
 until([&]{return all_at("0x3");},"all nodes synchronized certified history",120);
 auto hash=rpc(a,"eth_sendRawTransaction",{fixture["raw"]});
 until([&]{for(auto port:ports)if(rpc(port,"eth_getTransactionByHash",{hash}).is_null())return false;return true;},"transaction reached all nodes",120);
 produce();until([&]{return all_at("0x4");},"certified block reached all nodes",120);
 auto expected=rpc(a,"eth_getBlockByNumber",{"latest",false});auto receipt=rpc(a,"eth_getTransactionReceipt",{hash});check(!receipt.is_null(),"receipt exists");
 for(auto port:ports){auto block=rpc(port,"eth_getBlockByNumber",{"latest",false});check(block["hash"]==expected["hash"]&&block["stateRoot"]==expected["stateRoot"],"commitment mismatch");check(rpc(port,"eth_getTransactionReceipt",{hash})==receipt,"receipt mismatch");}
 for(auto& child:relays)child->stop();for(auto& child:cores)child->stop();
 std::cout<<Json{{"scenario","real-network-scale"},{"nodes",count},{"coreProcesses",count},{"relayProcesses",count},{"validatorCount",1},{"fork","Shanghai"},{"blocks",4},{"transactions",1},{"matchingCommitments",count},{"matchingReceipts",count},{"elapsedSeconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()},{"multiHost",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
