#include "support/process.hpp"
#include "nano/http.hpp"
#include "support/persistent_rpc.hpp"
#include <atomic>
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "../network/json_frame.hpp"
using namespace test;
struct RunningServer {HttpServer& server;std::thread thread;explicit RunningServer(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningServer(){server.stop();thread.join();}};
int main(){try{
 Temp root;auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));
 std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Paris";
 check(fork=="Paris"||fork=="Shanghai","explicit network profile");
 auto genesis=Genesis::from_json({{"gasLimit","0x2625a00"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 Blockchain chain(root/"producer",genesis);ChainStore journal(root/"journal");
 for(unsigned nonce=0;nonce<10;++nonce){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.gas=4000000;tx.gas_price=2000000000;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.data=Bytes(900000,0);auto signed_tx=SignedTransaction::sign(tx,key);check(signed_tx.raw().size()<1024*1024,"transaction below pool byte ceiling");check(chain.submit(signed_tx),"transaction admitted");}
 auto block=chain.block_template(signer,12,0);check(block.transactions.size()==10,"all ten fit gas budget");sign_block(block,key,1337,genesis.block.header.hash());chain.verify_authentication(block);chain.verify_proposal(block,12);

 ConsensusRound round(chain,journal,key,0,ConsensusRound::Clock::time_point{},1s,1s,1s);round.propose(block,12);check(round.advance(ConsensusRound::Clock::time_point{},12).committed,"real local journal-backed finality");
 Rpc rpc(chain);HttpServer server(rpc,"127.0.0.1",0);RunningServer running(server);PersistentRpc client(server.port());
 auto call=[&](const std::string& method,Json params)->Json{try{return {{"jsonrpc","2.0"},{"id",1},{"result",client.call(method,params)}};}catch(const PersistentRpcError& error){return {{"jsonrpc","2.0"},{"id",1},{"error",{{"code",error.code},{"message",error.what()}}}};}};

 const auto block_hash=hex(block.header.hash());auto manifest=call("nano_networkBlockManifest",Json::array({block_hash})).at("result");auto hashes=manifest.at("block").at("transactions");unsigned fragments=0,rejected=0;size_t maximum_request=0;
 {
  Blockchain follower(root/"follower",genesis);std::atomic<int64_t> elapsed{0};const auto clock_start=AdmissionBudget::Clock::now();const std::string token="synthetic-import-token";
  Rpc receiver(follower,token,[&]{return clock_start+std::chrono::milliseconds(elapsed.load());});HttpServer inbound(receiver,"127.0.0.1",0);RunningServer inbound_running(inbound);PersistentRpc destination(inbound.port());
  auto reject=[&](const std::string& method,const Json& params,std::optional<int> code=std::nullopt){bool refused=false;try{destination.call(method,params);}catch(const PersistentRpcError& e){refused=!code||e.code==*code;}check(refused,"invalid HTTP import request accepted");++rejected;check(follower.tip().header.number==0,"failed transfer changed canonical state");};
  reject("nano_beginBlockImport",Json::array({"wrong",manifest}),-32001);
  reject("nano_appendBlockImport",Json::array({"wrong",hex(Bytes(16,0)),0,Json::object()}),-32001);
  reject("nano_finishBlockImport",Json::array({"wrong",hex(Bytes(16,0))}),-32001);
  reject("nano_abortBlockImport",Json::array({"wrong",hex(Bytes(16,0))}),-32001);
  check(call("nano_beginBlockImport",Json::array({token,manifest})).at("error").at("code")==-32001,"empty-token RPC cannot enable import");
  auto begin=[&](const Json& m){return destination.call("nano_beginBlockImport",Json::array({token,m})).at("id").get<std::string>();};
  auto expired=begin(manifest);elapsed=61000;reject("nano_finishBlockImport",Json::array({token,expired}));
  std::vector<std::string> slots;for(unsigned i=0;i<4;++i)slots.push_back(begin(manifest));reject("nano_beginBlockImport",Json::array({token,manifest}),-32005);for(const auto& id:slots)check(destination.call("nano_abortBlockImport",Json::array({token,id}))==true,"abort frees staging slot");
  auto id=begin(manifest);auto bad_manifest=manifest;bad_manifest["block"]["bftCertificate"]["votes"]=Json::array();auto bad_id=begin(bad_manifest);
  reject("nano_finishBlockImport",Json::array({token,id}));
  for(size_t index=0;index<hashes.size();++index){uint64_t offset=0;for(;;){
   auto fragment=call("nano_networkTransactionFragment",Json::array({block_hash,hashes[index],quantity(Int(std::to_string(offset)))})).at("result");
   if(index==0&&offset==0){auto wrong=fragment;wrong["blockHash"]=hex(Bytes(32,0));reject("nano_appendBlockImport",Json::array({token,id,index,wrong}));}
   Json params=Json::array({token,id,index,fragment});auto size=Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_appendBlockImport"},{"params",params}}.dump().size();maximum_request=std::max(maximum_request,size);check(size<=512*1024+2048,"bounded HTTP ingress request");
   auto progress=destination.call("nano_appendBlockImport",params);destination.call("nano_appendBlockImport",Json::array({token,bad_id,index,fragment}));++fragments;
   check(follower.tip().header.number==0,"partial or merely complete assembly committed without finish");
   if(index==0&&offset==0)check(destination.call("nano_appendBlockImport",params)==progress,"duplicate HTTP fragment is idempotent");
   if(fragment.at("next").is_null()){check(progress.at("transaction")==index+1,"completed transaction cursor");break;}offset=u64(quantity(fragment.at("next")));
  }}
  auto invalid=destination.call("nano_finishBlockImport",Json::array({token,bad_id}));check(invalid.at("success")==false&&follower.tip().header.number==0,"complete payload without BFT certificate cannot commit");++rejected;
  reject("nano_finishBlockImport",Json::array({token,bad_id}));
  auto imported=destination.call("nano_finishBlockImport",Json::array({token,id}));check(imported.at("success")==true&&follower.tip().header.hash()==block.header.hash()&&follower.state().root()==chain.state().root()&&follower.audit().at("verified")==true,"fragmented HTTP import reaches exact audited canonical state");
  bool repeated=false;try{destination.call("nano_finishBlockImport",Json::array({token,id}));}catch(const PersistentRpcError&){repeated=true;}check(repeated,"finished import is consumed");
 }
 {Blockchain reopened(root/"follower",genesis);check(reopened.tip().header.hash()==block.header.hash()&&reopened.audit().at("verified")==true,"fragmented import survives database reopen");}
 std::cout<<Json{{"scenario","block-fragment-ingress"},{"fork",fork},{"fragmentsPerAssembly",fragments},{"assembliesForwarded",2},{"maximumRequestBytes",maximum_request},{"invalidRequestsRejected",rejected},{"privateTokenRequired",true},{"expiryChecked",true},{"sessionLimitChecked",true},{"duplicateIdempotence",true},{"badCertificateRejected",true},{"httpImportAudited",true},{"reopenedAudit",true},{"relayIntegrated",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
