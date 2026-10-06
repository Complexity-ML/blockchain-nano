#include "nano/client.hpp"
#include "nano/block_auth.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <thread>
namespace nano {
namespace asio=boost::asio;namespace ssl=asio::ssl;namespace beast=boost::beast;namespace http=beast::http;using tcp=asio::ip::tcp;
namespace {
struct Endpoint{bool tls;std::string host,port,path,authority;};
Endpoint endpoint(const std::string& url){
 Endpoint e;if(url.starts_with("https://"))e.tls=true;else if(url.starts_with("http://"))e.tls=false;else throw std::invalid_argument("RPC URL must use http or https");
 auto start=e.tls?8:7;auto slash=url.find('/',start);e.authority=url.substr(start,slash==std::string::npos?slash:slash-start);e.path=slash==std::string::npos?"/":url.substr(slash);e.port=e.tls?"443":"80";
 if(e.authority.empty()||e.authority.find_first_of("@?#\\\r\n\t ")!=std::string::npos||e.path.find_first_of("#\r\n ")!=std::string::npos)throw std::invalid_argument("Invalid RPC URL");
 if(e.authority[0]=='['){auto end=e.authority.find(']');if(end==std::string::npos)throw std::invalid_argument("Invalid IPv6 RPC URL");e.host=e.authority.substr(1,end-1);if(end+1<e.authority.size()){if(e.authority[end+1]!=':')throw std::invalid_argument("Invalid port");e.port=e.authority.substr(end+2);}}
 else{auto colon=e.authority.find(':');e.host=e.authority.substr(0,colon);if(colon!=std::string::npos)e.port=e.authority.substr(colon+1);}
 if(e.host.empty()||e.port.empty()||e.port.find_first_not_of("0123456789")!=std::string::npos||quantity(Json(e.port))>65535||quantity(Json(e.port))==0)throw std::invalid_argument("Invalid RPC host or port");return e;
}
template<class Stream>Json exchange(asio::io_context& context,Stream& stream,const Endpoint& endpoint,const Json& body,std::chrono::milliseconds timeout){
 tcp::resolver resolver(context);asio::steady_timer timer(context);beast::flat_buffer buffer;http::response_parser<http::string_body> parser;parser.body_limit(16*1024*1024);
 http::request<http::string_body> request(http::verb::post,endpoint.path,11);request.set(http::field::host,endpoint.authority);request.set(http::field::content_type,"application/json");request.body()=body.dump();request.prepare_payload();
 beast::error_code failure;bool timed_out=false;
 auto finish=[&](beast::error_code ec){failure=ec;timer.cancel();beast::error_code ignored;beast::get_lowest_layer(stream).socket().close(ignored);};
 auto write=[&](){if(timed_out){finish(asio::error::timed_out);return;}http::async_write(stream,request,[&](beast::error_code ec,size_t){if(ec||timed_out){finish(ec?ec:asio::error::timed_out);return;}http::async_read(stream,buffer,parser,[&](beast::error_code ec,size_t){finish(ec);});});};
 timer.expires_after(timeout);timer.async_wait([&](beast::error_code ec){if(!ec){timed_out=true;resolver.cancel();beast::error_code ignored;beast::get_lowest_layer(stream).socket().cancel(ignored);beast::get_lowest_layer(stream).socket().close(ignored);}});
 resolver.async_resolve(endpoint.host,endpoint.port,[&](beast::error_code ec,tcp::resolver::results_type results){if(ec||timed_out){finish(ec?ec:asio::error::timed_out);return;}beast::get_lowest_layer(stream).async_connect(results,[&](beast::error_code ec,const tcp::endpoint&){if(ec||timed_out){finish(ec?ec:asio::error::timed_out);return;}if constexpr(requires{stream.async_handshake(ssl::stream_base::client,[](beast::error_code){});})stream.async_handshake(ssl::stream_base::client,[&](beast::error_code ec){if(ec)finish(ec);else write();});else write();});});
 context.run();if(timed_out)throw std::runtime_error("RPC request timed out");if(failure)throw std::runtime_error("RPC transport: "+failure.message());auto response=parser.release();if(response.result()!=http::status::ok)throw std::runtime_error("RPC HTTP status "+std::to_string(response.result_int()));
 auto result=Json::parse(response.body(),nullptr,false);if(result.is_discarded())throw std::runtime_error("RPC response is not JSON");return result;
}
Json post(const Endpoint& endpoint,const Json& request,std::chrono::milliseconds timeout){
 asio::io_context context;
 if(endpoint.tls){ssl::context tls(ssl::context::tls_client);tls.set_default_verify_paths();beast::ssl_stream<beast::tcp_stream> stream(context,tls);stream.set_verify_mode(ssl::verify_peer);stream.set_verify_callback(ssl::host_name_verification(endpoint.host));if(!SSL_set_tlsext_host_name(stream.native_handle(),endpoint.host.c_str()))throw std::runtime_error("TLS server name setup failed");return exchange(context,stream,endpoint,request,timeout);}
 beast::tcp_stream stream(context);return exchange(context,stream,endpoint,request,timeout);
}
volatile std::sig_atomic_t validator_stopped=0;
void stop_validator(int){validator_stopped=1;}
}
RpcClient::RpcClient(std::vector<std::string> nodes,std::chrono::milliseconds timeout,unsigned retries):nodes_(std::move(nodes)),timeout_(timeout),retries_(retries){if(nodes_.empty()||timeout_.count()<=0||retries_<1||retries_>10)throw std::invalid_argument("Invalid RPC client configuration");for(const auto& node:nodes_)endpoint(node);}
void RpcClient::pin_chain(const Int& chain_id,const Bytes& genesis_hash){
 if(chain_id<=0||chain_id>=(Int(1)<<256)||genesis_hash.size()!=32)throw std::invalid_argument("invalid validator chain identity pins");
 std::lock_guard lock(mutex_);pinned_chain_=chain_id;pinned_genesis_=genesis_hash;
}
Json RpcClient::call(const std::string& method,Json params){
 std::lock_guard lock(mutex_);std::string error;
 for(size_t tried=0;tried<nodes_.size();++tried){for(unsigned retry=0;retry<retries_;++retry){
  try{
   // Verify this endpoint on every attempt, including retries and failover.
   // Use direct requests to avoid recursive locking and cross-endpoint checks.
   if(pinned_chain_){
    auto probe=[&](const std::string& name,const Json& args){
     auto id=++request_id_;auto reply=post(endpoint(nodes_[current_]),{{"jsonrpc","2.0"},{"id",id},{"method",name},{"params",args}},timeout_);
     if(!reply.is_object()||reply.value("jsonrpc",Json())!="2.0"||reply.value("id",Json())!=id||reply.contains("error")||!reply.contains("result"))throw std::runtime_error("validator identity probe failed");
     return reply.at("result");
    };
    if(quantity(probe("eth_chainId",Json::array()))!=*pinned_chain_||unhex(probe("eth_getBlockByNumber",Json::array({"0x0",false})).at("hash").get<std::string>())!=pinned_genesis_)throw std::runtime_error("validator chain identity mismatch");
   }
   auto id=++request_id_;auto response=post(endpoint(nodes_[current_]),{{"jsonrpc","2.0"},{"id",id},{"method",method},{"params",params}},timeout_);
   if(!response.is_object()||response.value("jsonrpc",Json())!="2.0"||response.value("id",Json())!=id)throw std::runtime_error("Invalid RPC response envelope");
   if(response.contains("error")){const auto& e=response["error"];throw RpcError(e.at("code").get<int>(),method+": "+e.at("message").get<std::string>(),e.value("data",Json()));}
   if(!response.contains("result"))throw std::runtime_error("Missing RPC result");return response["result"];
  }catch(const RpcError&){throw;}catch(const std::exception& e){error=e.what();if(retry+1<retries_)std::this_thread::sleep_for(std::chrono::milliseconds(500*(retry+1)));}
 }current_=(current_+1)%nodes_.size();}
 throw std::runtime_error("All RPC nodes unavailable: "+error);
}
static Json validate_cycle(RpcClient& client,const std::string& validator,const Wallet* signer,const Json& signing_config,std::unique_ptr<Blockchain>& verification){
 auto addr=address(validator);auto status=client.call("validator_getSyncStatus");if(status.value("syncing",true)||!status.value("ready",false))return {{"success",false},{"skipped",true},{"reason","Node is not ready"}};
 auto selected=client.call("validator_isSelected",{addr});if(!selected.value("selected",false))return {{"success",false},{"skipped",true},{"reason","Validator not selected"}};
 auto block=client.call("validator_getBlockTemplate",{addr});
 auto version=block.value("posSignatureVersion",Json(0));
 if(!version.is_number_integer()||(version!=0&&version!=1))throw std::runtime_error("unsupported PoS signature version");
 if(version==1){
  if(!signer||!signing_config.contains("chain_id")||!signing_config.contains("genesis_hash"))throw std::invalid_argument("signed validator requires chain_id and genesis_hash pins");
  auto chain_id=quantity(signing_config.at("chain_id"));auto genesis=unhex(signing_config.at("genesis_hash").get<std::string>());
  if(genesis.size()!=32||quantity(client.call("eth_chainId"))!=chain_id||unhex(block.at("genesisHash").get<std::string>())!=genesis)throw std::runtime_error("validator chain identity mismatch");
  auto candidate=Block::from_json(block);
  if(!signing_config.contains("signing_db"))throw std::invalid_argument("signed validator requires persistent signing_db");
  auto journal_path=signing_config.at("signing_db").get<std::string>();if(journal_path.empty())throw std::invalid_argument("signing_db cannot be empty");
  if(address(signer->address())!=address(candidate.header.coinbase))throw std::runtime_error("block signer differs from producer");
  ChainStore journal(journal_path);
  if(auto saved=journal.signing_proposal(chain_id,genesis,signer->address(),candidate.header.number))candidate=*saved;
  if(!verification){
  if(!signing_config.contains("genesis_file")||!signing_config.contains("verification_db"))throw std::invalid_argument("signed validator requires genesis_file and verification_db for local execution");
  std::ifstream input(signing_config.at("genesis_file").get<std::string>());
  if(!input)throw std::invalid_argument("cannot open validator genesis_file");
  Json genesis_json;input>>genesis_json;auto local_genesis=Genesis::from_json(genesis_json);
  if(local_genesis.chain_id!=chain_id||local_genesis.block.header.hash()!=genesis||local_genesis.pos_signature_version!=1)throw std::invalid_argument("validator local genesis identity mismatch");
  auto verification_path=signing_config.at("verification_db").get<std::string>();
  if(verification_path.empty())throw std::invalid_argument("verification_db cannot be empty");
  auto opened=std::make_unique<Blockchain>(verification_path,local_genesis);
  // Reopening a database alone only authenticates its tip. Audit its history
  // before relying on the saved state for a new signature.
  opened->audit();
  verification=std::move(opened);
  }
  auto& verified=*verification;
  const Int now(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
  if(candidate.header.number<=verified.tip().header.number)throw std::runtime_error("validator proposal does not extend local verified history");
  unsigned imported=0;
  while(verified.tip().header.number+1<candidate.header.number){
   if(imported++==128)return {{"success",false},{"skipped",true},{"reason","Local verification is synchronizing"},{"verifiedHeight",quantity(verified.tip().header.number)}};
   auto height=Int(verified.tip().header.number+1);
   auto parent=Block::from_json(client.call("eth_getBlockByNumber",{quantity(height),true}));
   verified.import_block(parent,now);
  }
  verified.verify_proposal(candidate,now);
  journal.reserve_signature(chain_id,genesis,signer->address(),candidate.header.number,block_signing_hash(candidate,chain_id,genesis),candidate);
  signer->sign_block(candidate,chain_id,genesis);block=candidate.to_json(true);
 }else if(signing_config.contains("genesis_hash"))throw std::runtime_error("signed validator refuses unsigned block template");
 return client.call("validator_submitBlock",{block});
}
Json validate_once(RpcClient& client,const std::string& validator,const Wallet* signer,const Json& signing_config){
 std::unique_ptr<Blockchain> verification;
 return validate_cycle(client,validator,signer,signing_config,verification);
}
void ValidatorStats::record(const Json& result){
 if(!result.value("success",false)||result.value("skipped",false))return;
 // Decode all remote quantities before publishing a successful observation.
 auto reward=quantity(result.value("blockReward",Json("0x0")));
 auto fee=quantity(result.value("gasFees",Json("0x0")));
 rewards_+=reward;fees_+=fee;++blocks_;
 last_block_=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
Json ValidatorStats::to_json(const std::string& node,bool running)const{
 auto uptime=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-started_).count();
 return {{"address",address_},{"running",running},{"blocks_produced",blocks_},
         {"total_rewards_wei",rewards_.get_str()},{"total_gas_fees_wei",fees_.get_str()},
         {"total_earned_wei",Int(rewards_+fees_).get_str()},{"uptime_seconds",uptime},
         {"last_block_time",last_block_?Json(*last_block_):Json(nullptr)},{"current_node",node}};
}
Json validator_environment(const Json& config){
 if(!config.is_object())throw std::invalid_argument("Validator configuration must be an object");
 Json resolved=config;
 if(!resolved.contains("nodes"))if(const char* raw=std::getenv("VALIDATOR_NODES");raw&&*raw){
  std::string list=raw;Json nodes=Json::array();size_t begin=0;
  do{
   auto end=list.find(',',begin);auto item=list.substr(begin,end==std::string::npos?end:end-begin);
   auto first=item.find_first_not_of(" \t\r\n"),last=item.find_last_not_of(" \t\r\n");
   if(first==std::string::npos)throw std::invalid_argument("VALIDATOR_NODES contains an empty endpoint");
   item=item.substr(first,last-first+1);endpoint(item);nodes.push_back(item);
   if(end==std::string::npos)break;begin=end+1;
  }while(begin<=list.size());
  resolved["nodes"]=std::move(nodes);
 }
 if(!resolved.contains("block_time"))if(const char* raw=std::getenv("VALIDATOR_BLOCK_TIME");raw&&*raw){
  auto interval=u64(quantity(Json(raw)));if(interval<1||interval>3600)throw std::invalid_argument("VALIDATOR_BLOCK_TIME must be 1 to 3600 seconds");
  resolved["block_time"]=interval;
 }
 return resolved;
}
int run_validator(const Json& input,bool once){
 auto config=validator_environment(input);
 auto raw_key=config.value("validator_key",std::string());if(raw_key.empty()){const char* env=std::getenv("VALIDATOR_KEY");if(env)raw_key=env;}auto key=unhex(raw_key);auto addr=public_address(public_key(key));
 auto nodes=config.value("nodes",std::vector<std::string>{"http://localhost:8545"});auto seconds=config.value("rpc_timeout",10);auto interval=config.value("block_time",12);if(seconds<1||seconds>300||interval<1||interval>3600)throw std::invalid_argument("Invalid validator interval/timeout");
 RpcClient client(nodes,std::chrono::seconds(seconds),config.value("max_retries",3U));
 if(config.contains("chain_id")||config.contains("genesis_hash")||config.value("register",false)){
  if(!config.contains("chain_id")||!config.contains("genesis_hash"))throw std::invalid_argument("signed validator requires chain_id and genesis_hash pins");
  client.pin_chain(quantity(config.at("chain_id")),unhex(config.at("genesis_hash").get<std::string>()));
 }
 if(config.value("register",false)){
  auto target=address(config.at("staking_contract").get<std::string>());
  auto amount=nano_amount(config.value("stake_nano",std::string("10000")));
  auto timeout=config.value("registration_timeout",120);if(timeout<1||timeout>3600)throw std::invalid_argument("registration_timeout must be 1 to 3600 seconds");
  if(quantity(client.call("nno_getStake",{addr}))==0){
   // A previous registration may still be pending after a receipt timeout.
   auto latest_nonce=quantity(client.call("eth_getTransactionCount",{addr,"latest"}));
   auto pending_nonce=quantity(client.call("eth_getTransactionCount",{addr,"pending"}));
   if(pending_nonce!=latest_nonce)throw std::runtime_error("validator has pending transactions; confirm their receipts before registering");
   auto submitted=submit_staking(client,Wallet(key),target,"stake-register",amount);
   auto hash=submitted.at("hash").get<std::string>();submitted["event"]="registration_submitted";std::cout<<submitted.dump()<<std::endl;
   auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(timeout);bool confirmed=false;
   while(std::chrono::steady_clock::now()<deadline){
    auto receipt=client.call("eth_getTransactionReceipt",{hash});
    if(!receipt.is_null()){
     if(quantity(receipt.at("status"))!=1)throw std::runtime_error("validator registration failed: "+hash);
     if(quantity(client.call("nno_getStake",{addr}))==0)throw std::runtime_error("registration receipt succeeded without active stake");
     confirmed=true;break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
   }
   if(!confirmed)throw std::runtime_error("validator registration pending; receipt timeout for "+hash);
  }
 }
 Wallet block_signer(key);
 ValidatorStats stats(addr);
 std::unique_ptr<Blockchain> verification;
 auto cycle=[&]{auto result=validate_cycle(client,addr,&block_signer,config,verification);stats.record(result);result["stats"]=stats.to_json(client.current_node(),!once);return result;};
 if(once){auto result=cycle();std::cout<<result.dump(2)<<"\n";return result.value("success",false)||result.value("skipped",false)?0:1;}
 validator_stopped=0;auto old_int=std::signal(SIGINT,stop_validator);auto old_term=std::signal(SIGTERM,stop_validator);
 while(!validator_stopped){try{std::cout<<cycle().dump()<<std::endl;}catch(const std::exception& e){std::cerr<<"Validator: "<<e.what()<<std::endl;}for(int i=0;i<interval*10&&!validator_stopped;++i)std::this_thread::sleep_for(std::chrono::milliseconds(100));}
 std::signal(SIGINT,old_int);std::signal(SIGTERM,old_term);return 0;
}
}
