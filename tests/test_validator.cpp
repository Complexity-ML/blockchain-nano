#include "nano/client.hpp"
#include "nano/http.hpp"
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <cstdlib>
using namespace nano;
namespace asio=boost::asio;using tcp=asio::ip::tcp;
static int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f,const char* message){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,message);}
struct Running{HttpServer& server;std::thread thread;Running(HttpServer& s):server(s),thread([&s]{s.run();}){}~Running(){server.stop();thread.join();}};
int main(){try{
 {
  struct Environment {
   std::string name;std::optional<std::string> previous;
   explicit Environment(std::string n):name(std::move(n)){if(const char* value=std::getenv(name.c_str()))previous=value;}
   ~Environment(){if(previous)setenv(name.c_str(),previous->c_str(),1);else unsetenv(name.c_str());}
  } nodes("VALIDATOR_NODES"),interval("VALIDATOR_BLOCK_TIME");
  setenv("VALIDATOR_NODES"," http://127.0.0.1:8545, https://localhost:8546 ",1);setenv("VALIDATOR_BLOCK_TIME","7",1);
  auto config=validator_environment(Json::object());
  check(config["nodes"]==Json::array({"http://127.0.0.1:8545","https://localhost:8546"})&&config["block_time"]==7,"validator accepts environment nodes and interval");
  Json explicit_config={{"nodes",Json::array({"http://localhost:9000"})},{"block_time",12}};
  setenv("VALIDATOR_NODES","invalid",1);setenv("VALIDATOR_BLOCK_TIME","invalid",1);
  check(validator_environment(explicit_config)==explicit_config,"JSON validator configuration overrides environment");
  for(const char* bad:{"http://localhost:8545,",",http://localhost:8545","file:///tmp/node"}){
   setenv("VALIDATOR_NODES",bad,1);rejects([&]{validator_environment(Json{{"block_time",12}});},"invalid environment node rejected");
  }
  unsetenv("VALIDATOR_NODES");
  for(const char* bad:{"0","3601","-1","abc"}){
   setenv("VALIDATOR_BLOCK_TIME",bad,1);rejects([&]{validator_environment(Json::object());},"invalid environment interval rejected");
  }
  unsetenv("VALIDATOR_BLOCK_TIME");check(validator_environment(Json::object()).empty(),"absent environment preserves validator defaults");
 }
 auto path=std::filesystem::temp_directory_path()/("nano-validator-"+std::to_string(getpid()));
 auto validator=public_address(public_key(bytes(Int(1),32)));auto key=bytes(Int(2),32);auto sender=public_address(public_key(key));
 auto genesis=Genesis::from_json({{"config",{{"consensus","pos"}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}}}}});
 ProofOfStake pos;pos.register_validator(validator,ProofOfStake::minimum_stake,0);
 {ChainStore store(path);store.bind_chain(genesis.chain_id,genesis.block.header.hash(),"pos");store.commit(genesis.block,genesis.state,{},pos.to_json());}
 Int final_height=2;
 {Blockchain chain(path,genesis);Rpc rpc(chain,"validator-test-token");HttpServer server(rpc,"127.0.0.1",0);Running running(server);
 auto url="http://127.0.0.1:"+std::to_string(server.port());RpcClient client({"http://127.0.0.1:1",url},std::chrono::seconds(2),1);
 check(client.call("eth_chainId")=="0x539"&&client.current_node()==url,"client failover to healthy node");
 RpcClient pinned({"http://127.0.0.1:1",url},std::chrono::seconds(2),1);
 pinned.pin_chain(genesis.chain_id,genesis.block.header.hash());
 check(pinned.call("eth_chainId")=="0x539","pinned client verifies failover endpoint");
 pinned.pin_chain(genesis.chain_id+1,genesis.block.header.hash());
 rejects([&]{pinned.call("eth_chainId");},"pinned client refuses wrong chain ID");
 pinned.pin_chain(genesis.chain_id,Bytes(32));
 rejects([&]{pinned.call("eth_chainId");},"pinned client refuses wrong genesis");
 bool rpc_error=false;try{client.call("unknown");}catch(const RpcError& e){rpc_error=e.code==-32601;}check(rpc_error&&client.current_node()==url,"RPC errors are not retried as transport failures");
 auto selection=client.call("validator_isSelected",{validator});check(selection["selected"]==true&&selection["blockNumber"]=="0x0","validator selection endpoint");
 check(client.call("validator_isSelected",{sender})["selected"]==false,"nonselected validator");
 auto status=client.call("validator_getSyncStatus");check(status["ready"]==true&&status["networkConnected"]==false&&status["peerCount"]==0,"sync status exposes local readiness");
 Transaction tx;tx.to=validator;tx.gas_price=2000000000;tx.value=123;auto signed_tx=SignedTransaction::sign(tx,key);client.call("eth_sendRawTransaction",{hex(signed_tx.raw())});
 check(client.call("validator_getPendingTransactions").size()==1&&client.call("validator_getPendingTransactions",{"0x1"}).empty(),"pending transactions gas cap");
 auto before=chain.state().root();auto cursor=chain.event_cursor();auto block=client.call("validator_getBlockTemplate",{validator});
 check(block["transactions"].size()==1&&block["coinbase"]==address(validator)&&block["blockNumber"]=="0x1","block template includes selected transactions");
 check(chain.state().root()==before&&chain.tip().header.number==0&&chain.pending_count()==1&&chain.event_cursor()==cursor,"template leaves state pool and events unchanged");
 rejects([&]{client.call("validator_getBlockTemplate",{sender});},"template rejects wrong producer");
 auto corrupt=block;corrupt["stateRoot"]=hex(Bytes(32));corrupt.erase("hash");check(client.call("validator_submitBlock",{corrupt})["success"]==false,"submission rejects false commitments");
 check(chain.tip().header.number==0&&chain.state().root()==before,"rejected submission leaves state intact");
 auto result=client.call("validator_submitBlock",{block});check(result["success"]==true&&result["blockNumber"]=="0x1"&&result["transactionCount"]==1,"HTTP validator produces block");
 ValidatorStats stats(validator);auto initial_stats=stats.to_json(url,true);
 check(initial_stats["blocks_produced"]==0&&initial_stats["last_block_time"].is_null(),"validator statistics start empty");
 stats.record(result);auto first_stats=stats.to_json(client.current_node(),true);
 check(first_stats["blocks_produced"]==1&&first_stats["total_rewards_wei"]==quantity(result["blockReward"]).get_str()&&first_stats["total_gas_fees_wei"]==quantity(result["gasFees"]).get_str(),"statistics record actual block reward and fees exactly");
 check(first_stats["current_node"]==url&&first_stats["running"]==true&&first_stats["last_block_time"].is_number_integer(),"statistics expose failover node and activity");
 stats.record(Json{{"success",false}});stats.record(Json{{"skipped",true},{"success",false}});
 rejects([&]{stats.record(Json{{"success",true},{"blockReward","0x2"},{"gasFees","invalid"}});},"statistics reject malformed rewards");
 check(stats.to_json(url,false)["total_earned_wei"]==first_stats["total_earned_wei"]&&stats.to_json(url,false)["blocks_produced"]==1,"failed and malformed attempts do not change statistics");
 stats.record(Json{{"success",true},{"blockReward","0x1"},{"gasFees","0x1"}});
 auto total=stats.to_json(url,false);check(total["blocks_produced"]==2&&total["running"]==false&&Int(total["total_earned_wei"].get<std::string>())==quantity(result["totalReward"])+2,"statistics preserve one-wei increments above floating point precision");
 Int fees=Int(21000)*2000000000;check(quantity(result["gasFees"])==fees&&quantity(result["totalReward"])==ProofOfStake::block_reward+fees,"actual execution fees and rewards returned");
 check(chain.pending_count()==0&&chain.store().receipt(signed_tx.hash()).has_value(),"submission persists receipt and drains pool");
 check(client.call("validator_submitBlock",{block})==result&&chain.store().count()==2,"duplicate block submission idempotent");
 auto stale=block;stale["timestamp"]=quantity(Int(quantity(stale["timestamp"])+1));stale.erase("hash");check(client.call("validator_submitBlock",{stale})["success"]==false,"stale different template rejected");
 check(validate_once(client,sender)["skipped"]==true,"validator client skips unselected address");
 std::ostringstream output;auto previous_output=std::cout.rdbuf(output.rdbuf());int exit_code;
 try{exit_code=run_validator(Json{{"validator_key",hex(bytes(Int(1),32))},{"nodes",Json::array({url})}},true);}
 catch(...){std::cout.rdbuf(previous_output);throw;}
 std::cout.rdbuf(previous_output);auto once_result=Json::parse(output.str());
 check(exit_code==0&&once_result["success"]==true&&chain.tip().header.number==2,"native validator client completes production cycle");
 check(once_result["stats"]["blocks_produced"]==1&&once_result["stats"]["running"]==false&&once_result["stats"]["current_node"]==url,"one-shot validator publishes final session statistics");
 asio::io_context context;tcp::acceptor stalled(context,tcp::endpoint(asio::ip::make_address("127.0.0.1"),0));auto start=std::chrono::steady_clock::now();RpcClient impatient({"http://127.0.0.1:"+std::to_string(stalled.local_endpoint().port())},std::chrono::milliseconds(100),1);
 rejects([&]{impatient.call("eth_chainId");},"unresponsive peer reaches timeout");check(std::chrono::steady_clock::now()-start<std::chrono::seconds(2),"request deadline is bounded");
 rejects([]{RpcClient invalid({"http://user:secret@localhost"});},"credentials in URL rejected");
 rejects([&]{client.call("nno_startValidating");},"automatic validator requires owned coinbase wallet");
 auto wallet_path=path.string()+".wallet";Wallet(bytes(Int(1),32)).save_keyfile(wallet_path,"validator-test-password");
 setenv("NANO_VALIDATOR_TEST_PASSWORD","validator-test-password",1);
 Json config={{"coinbase",validator},{"block_time",1},{"wallets",Json::array({Json{{"path",wallet_path},{"password_env","NANO_VALIDATOR_TEST_PASSWORD"}}})}};
 auto invalid_config=config;invalid_config["block_time"]=0;
 rejects([&]{rpc.load_wallets(invalid_config,"127.0.0.1");},"zero block interval rejected");
 invalid_config=config;invalid_config["coinbase"]=sender;rpc.load_wallets(invalid_config,"127.0.0.1");
 rejects([&]{client.call("nno_startValidating");},"coinbase must match an unlocked wallet");
 rpc.load_wallets(config,"127.0.0.1");unsetenv("NANO_VALIDATOR_TEST_PASSWORD");std::filesystem::remove(wallet_path);
 auto report=Json{{"peerId","local"},{"addresses",Json::array()},{"peers",Json::array({Json{{"peerId","ahead"},{"bestBlock",100}}})}};
 client.call("nano_networkReport",{"validator-test-token",report});
 check(client.call("nno_startValidating")==true&&client.call("nno_startValidating")==true,"automatic validation start is idempotent");
 std::this_thread::sleep_for(std::chrono::milliseconds(1200));
 check(chain.tip().header.number==2&&client.call("nno_getNetworkInfo")["node"]["validating"]==true,"automatic validator waits for synchronization");
 report["peers"]=Json::array();client.call("nano_networkReport",{"validator-test-token",report});
 tx.nonce=1;tx.value=456;auto automatic_tx=SignedTransaction::sign(tx,key);client.call("eth_sendRawTransaction",{hex(automatic_tx.raw())});
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
 while(chain.tip().header.number==2&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(50));
 check(chain.tip().header.number>2&&chain.store().receipt(automatic_tx.hash()).has_value(),"server timer validates pending transaction without manual template submission");
 check(client.call("nno_stopValidating")==true&&client.call("nno_stopValidating")==true,"automatic validation stop is idempotent");
 final_height=chain.tip().header.number;std::this_thread::sleep_for(std::chrono::milliseconds(1200));
 check(chain.tip().header.number==final_height&&client.call("nno_getNetworkInfo")["node"]["validating"]==false,"stop prevents further automatic blocks");
 check(client.call("nno_getNetworkInfo")["node"]["validationError"].is_null(),"successful automatic production has no validation error");

 }
 {Blockchain reopened(path,genesis);check(reopened.tip().header.number==final_height&&reopened.validators()[0]["blocks_validated"]==quantity(final_height),"remote production restored from disk");}
 std::filesystem::remove_all(path);std::cout<<checks<<" validator/client checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"After "<<checks<<" checks: "<<e.what()<<"\n";return 1;}}
