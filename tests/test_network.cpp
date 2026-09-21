#include "support/process.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
using namespace test;
int main(int argc,char** argv){try{
 check(argc>=3,"core and transport required");std::string core=argv[1],network=argv[2];bool signed_mode=argc>3,bft_mode=argc>3&&std::string(argv[3])=="--bft";Temp root;
 auto fixture=load(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/network.json");std::string sender=fixture["sender"];
 Json config={{"chainId",1337},{"consensus","pos"}};if(signed_mode){config["posSignatureVersion"]=1;config["bootstrapValidator"]=address(account);}
 if(bft_mode)config["bftVersion"]=1;
 auto genesis=root/"genesis.json";save(genesis,{{"config",config},{"alloc",{{sender,{{"balance",quantity(Int("10000000000000000000000"))}}}}}});
 Process left({core,"serve",genesis.string(),(root/"a").string(),"0"}),right({core,"serve",genesis.string(),(root/"b").string(),"0"});int a=left.port(),b=right.port();
 auto validator=root/"validator.json";
 if(signed_mode)save(validator,{{"nodes",Json::array({url(a)})},{"chain_id",1337},{"genesis_hash",rpc(a,"eth_getBlockByNumber",{"earliest",false})["hash"]},{"signing_db",(root/"journal").string()},{"genesis_file",genesis.string()},{"verification_db",(root/"verified").string()},{"max_retries",1}});
 std::unique_ptr<Blockchain> bft_chain;std::unique_ptr<ChainStore> bft_journal;
 if(bft_mode){bft_chain=std::make_unique<Blockchain>(root/"bft-producer",Genesis::from_json(load(genesis)));bft_journal=std::make_unique<ChainStore>(root/"bft-journal");}
 auto produce=[&](){Json result;if(bft_mode){
  auto candidate=Block::from_json(rpc(a,"validator_getBlockTemplate",{account}));
  sign_block(candidate,unhex(key()),1337,bft_chain->genesis().block.header.hash());
  auto now=ConsensusRound::Clock::now();ConsensusRound round(*bft_chain,*bft_journal,unhex(key()),0,now,1s,1s,1s);
  round.propose(candidate,candidate.header.timestamp);check(round.advance(now,candidate.header.timestamp).committed,"local BFT fixture failed to certify");
  auto certificate=bft_chain->store().certificate_at(candidate.header.number);Json votes=Json::array();for(const auto& vote:certificate->votes)votes.push_back(encode_vote_message(vote,1337,bft_chain->genesis().block.header.hash()));
  auto payload=candidate.to_json(true);payload["bftCertificate"]={{"version",1},{"round","0x0"},{"votes",votes}};result=rpc(a,"validator_submitBlock",{payload});
 }else if(signed_mode)result=success(run({core,"validator",validator.string(),"--once"},{{"VALIDATOR_KEY",key()}}));else result=rpc(a,"validator_submitBlock",{rpc(a,"validator_getBlockTemplate",{bootstrap})});check(result["success"],"block production failed");};
 produce();produce();produce();
 Process pa({network,std::to_string(a),(root/"a.key").string(),"/ip4/127.0.0.1/tcp/0"});std::string addr=pa.event("listen")["address"];pa.event("ready");
 auto dns=addr;dns.replace(dns.find("/ip4/127.0.0.1/"),std::string("/ip4/127.0.0.1/").size(),"/dns4/localhost/");auto peer=addr.substr(addr.rfind("/p2p/")+5);
 Process pb({network,std::to_string(b),(root/"b.key").string(),"/ip4/127.0.0.1/tcp/0","/ip4/127.0.0.1/tcp/1/p2p/"+peer,dns});pb.event("listen");pb.event("ready");
 until([&]{return rpc(b,"eth_blockNumber")=="0x3";},"History synchronized through DNS bootstrap");auto txhash=rpc(a,"eth_sendRawTransaction",{fixture["raw"]});
 until([&]{return !rpc(b,"eth_getTransactionByHash",{txhash}).is_null();},"Transaction propagated");produce();until([&]{return rpc(b,"eth_blockNumber")=="0x4";},"Block announcement imported");
 auto aa=rpc(a,"eth_getBlockByNumber",{"latest",false}),bb=rpc(b,"eth_getBlockByNumber",{"latest",false});check(aa["hash"]==bb["hash"]&&aa["stateRoot"]==bb["stateRoot"],"peer commitments differ");check(rpc(a,"eth_getTransactionReceipt",{txhash})==rpc(b,"eth_getTransactionReceipt",{txhash}),"peer receipts differ");
 if(signed_mode){Json last;for(int h=1;h<=4;++h){auto l=rpc(a,"eth_getBlockByNumber",{quantity(Int(h)),true});last=rpc(b,"eth_getBlockByNumber",{quantity(Int(h)),true});check(l["producerProof"]==last["producerProof"]&&last["producerProof"]["version"]==1,"signed proof not preserved");}
  auto forged=last;forged.erase("producerProof");check(!rpc(b,"validator_submitBlock",{forged})["success"].get<bool>(),"missing proof accepted");forged=last;auto signature=unhex(forged["producerProof"]["signature"]);signature[0]^=1;forged["producerProof"]["signature"]=hex(signature);check(!rpc(b,"validator_submitBlock",{forged})["success"].get<bool>(),"forged proof accepted");check(rpc(b,"eth_getBlockByNumber",{"latest",false})["hash"]==last["hash"],"forgery changed chain");}
 pa.stop();Process restarted({network,std::to_string(a),(root/"a.key").string(),"/ip4/127.0.0.1/tcp/0"});std::string newaddr=restarted.event("listen")["address"];check(newaddr.substr(newaddr.rfind("/p2p/")+5)==peer,"P2P identity lost on restart");
 for(int id:{1338,1337}){auto c=config;c["chainId"]=id;auto path=root/(std::to_string(id)+".json");save(path,{{"config",c},{"alloc",{{sender,{{"balance",quantity(Int("10000000000000000000000")+(id==1337?1:0))}}}}}});
  Process foreign({core,"serve",path.string(),(root/("wrong-"+std::to_string(id))).string(),"0"});int port=foreign.port();Process remote({network,std::to_string(port),(root/(std::to_string(id)+".key")).string(),"/ip4/127.0.0.1/tcp/0",newaddr});remote.event("listen");remote.event("ready");
  auto end=std::chrono::steady_clock::now()+3s;while(std::chrono::steady_clock::now()<end){remote.pump(50);check(remote.running(),"foreign peer unexpectedly exited");}
  check(rpc(port,"eth_blockNumber")=="0x0","foreign chain synchronized");std::istringstream lines(remote.output());std::string line;while(std::getline(lines,line)){auto event=Json::parse(line,nullptr,false);check(!event.is_object()||event.value("event","")!="peer","foreign peer admitted");}}
 std::cout<<"Native network integration passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
