#include "support/process.hpp"
#include "nano/rpc.hpp"
#include "nano/block_auth.hpp"
using namespace test;
int main(int argc,char** argv){try{
 check(argc==1||(argc==2&&std::string(argv[1])=="--paged"),"expected optional --paged");bool paged=argc==2;
 Temp root;std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Shanghai";
 Json artifact;std::ifstream(std::string(NANO_TEST_SOURCE)+"/contracts/NanoStaking.json")>>artifact;
 constexpr unsigned initial=1000;std::vector<Bytes> keys;std::vector<std::string> addresses;Json alloc=Json::object();
 for(unsigned i=1;i<=initial+1;++i){keys.push_back(bytes(Int(i),32));addresses.push_back(public_address(public_key(keys.back())));alloc[addresses.back()]={{"balance",quantity(Int(ProofOfStake::minimum_stake*2))}};}
 auto registry=create_address(addresses[0],0);
 Json config={{"chainId",1337},{"evmFork",fork},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",addresses[0]},{"stakingContract",registry}};
 if(paged)config["stakingQueryRules"]={{"version",2},{"perCallGas",5000000},{"syncGas",50000000},{"pageSize",64}};
 auto seed=Genesis::from_json({{"config",config},{"alloc",alloc}});State registered;
 auto registration=[&](unsigned i){Transaction t;t.chain_id=1337;t.value=ProofOfStake::minimum_stake;t.gas=3000000;t.gas_price=2000000000;if(i==0)t.data=unhex(artifact.at("bytecode"));else{t.to=registry;t.data=unhex(artifact.at("methodIdentifiers").at("registerValidator()"));}return SignedTransaction::sign(t,keys[i]);};
 {Blockchain setup(root/"setup",seed);
  unsigned next=0;while(next<initial){std::vector<Bytes> hashes;auto end=next==0?1:std::min(initial,next+50);
   for(;next<end;++next){auto tx=registration(next);check(setup.submit(tx),"signed registration admitted");hashes.push_back(tx.hash());}
   auto selected=setup.next_validator();auto i=std::find(addresses.begin(),addresses.end(),address(*selected))-addresses.begin();
   // Addresses returned by public_address may use checksum casing.
   if(i==addresses.size())i=std::find_if(addresses.begin(),addresses.end(),[&](const auto& a){return address(a)==address(*selected);})-addresses.begin();
   check(i<keys.size(),"known setup producer");setup.validate_signed(keys[i],Int(setup.tip().header.timestamp+12));
   for(const auto& h:hashes)check(setup.store().receipt(h)&&setup.store().receipt(h)->success,"registration executed successfully");
   check(setup.validators().size()==next,"all registered validators eligible");
  }registered=setup.state();
 }
 config["bftVersion"]=1;auto genesis=Genesis::from_json({{"config",config},{"alloc",registered.to_json()}});Bytes final_hash;
 {Blockchain chain(root/"bft",genesis);check(chain.voting_set().size()==initial,"initial BFT set contains 1000 validators");
  for(unsigned height=1;height<=2;++height){auto members=chain.voting_set();
   if(height==1)check(chain.submit(registration(initial)),"1001st signed registration admitted");
   auto selected=chain.next_validator();auto producer=std::find_if(addresses.begin(),addresses.end(),[&](const auto& a){return address(a)==address(*selected);})-addresses.begin();check(producer<keys.size(),"known BFT producer");
   auto block=chain.block_template(*selected,Int(height*12));sign_block(block,keys[producer],1337,genesis.block.header.hash());
   std::vector<FinalityVote> votes;auto set_hash=voting_set_hash(members);
   for(unsigned i=0;i<(height==1?initial:initial+1);++i){FinalityVote v;v.height=height;v.round=0;v.step=VoteStep::precommit;v.validator=addresses[i];v.validator_set_hash=set_hash;v.block_hash=block.header.hash();sign_vote(v,keys[i],1337,genesis.block.header.hash());votes.push_back(v);}
   auto deficient=votes;deficient.resize((members.size()*2)/3);check(!verify_commit_certificate(deficient,members,height,0,block.header.hash(),1337,genesis.block.header.hash()),"at most two thirds never finalizes");
   auto bad=votes;bad.back().signature[0]^=1;check(!verify_commit_certificate(bad,members,height,0,block.header.hash(),1337,genesis.block.header.hash()),"invalid signature refused even with otherwise ample votes");
   if(height==2){auto decisive=deficient;decisive.push_back(votes.back());check(verify_commit_certificate(decisive,members,height,0,block.header.hash(),1337,genesis.block.header.hash()),"1001st validator supplies decisive quorum weight");}
   Json encoded=Json::array();for(const auto& v:votes)encoded.push_back(encode_vote_message(v,1337,genesis.block.header.hash()));
   auto payload=block.to_json(true);payload["bftCertificate"]={{"version",1},{"round","0x0"},{"votes",encoded}};
   Rpc rpc(chain);auto response=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","validator_submitBlock"},{"params",Json::array({payload})}});
   check(response.contains("result")&&response.at("result").value("success",false),"RPC accepts certificate above old 1000-vote cap: "+response.dump());
   check(chain.voting_set().size()==initial+1,"1001 validators eligible after registration commit");
   if(height==1)check(chain.store().receipt(registration(initial).hash())->success,"1001st registration receipt succeeds");
   auto proof=chain.store().certificate_at(height);check(proof&&proof->votes.size()==votes.size(),"complete certificate persisted");
   final_hash=block.header.hash();
  }
  check(chain.audit().at("verified")==true,"full BFT history replays with 1001-validator set");
 }
 {Blockchain reopened(root/"bft",genesis);check(reopened.tip().header.hash()==final_hash&&reopened.voting_set().size()==1001&&reopened.store().certificate_at(2)->votes.size()==1001&&reopened.audit().at("verified")==true,"1001-validator history and certificate survive reopen");}
 std::cout<<Json{{"scenario","validator-open-admission"},{"fork",fork},{"pageSize",paged?64:0},{"signedRegistrations",1001},{"finalValidators",1001},{"certificateVotes",1001},{"reopenedAudit",true},{"networkProcesses",0},{"rpcCertificateAccepted",true},{"newValidatorDecisive",true}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
