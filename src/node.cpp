#include "nano/node.hpp"
#include "nano/http.hpp"
#include "nano/consensus_altair_runtime.hpp"
#include "nano/consensus_bellatrix_runtime.hpp"
#include "nano/consensus_capella_runtime.hpp"
#include "nano/consensus_capella_health.hpp"
#include <spawn.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
extern char** environ;
namespace nano {
namespace {
std::filesystem::path executable_directory(const std::filesystem::path& argument){
#ifdef __APPLE__
 uint32_t size=0;_NSGetExecutablePath(nullptr,&size);std::vector<char> path(size);
 if(_NSGetExecutablePath(path.data(),&size)==0)return std::filesystem::canonical(path.data()).parent_path();
#elif defined(__linux__)
 std::error_code error;auto path=std::filesystem::canonical("/proc/self/exe",error);if(!error)return path.parent_path();
#endif
 return std::filesystem::canonical(argument).parent_path();
}
class Child {
 pid_t pid_=-1;
public:
 Child(const std::filesystem::path& executable,const std::vector<std::string>& arguments,const std::string& token,const std::vector<std::string>& excluded){
  std::vector<std::string> args{executable.string()};args.insert(args.end(),arguments.begin(),arguments.end());std::vector<char*> argv;for(auto& arg:args)argv.push_back(arg.data());argv.push_back(nullptr);
  std::vector<std::string> environment;for(char** item=environ;*item;++item){std::string_view value(*item);bool omit=value.starts_with("VALIDATOR_KEY=")||value.starts_with("NANO_NETWORK_TOKEN=")||value.starts_with("NANO_SUPERVISOR_PID=")||value.starts_with("NANO_PRIVATE_KEY=")||value.starts_with("NANO_MNEMONIC=")||value.starts_with("NANO_HD_PASSPHRASE=")||value.starts_with("NANO_WALLET_PASSWORD=");for(const auto& name:excluded)if(value.starts_with(name+"="))omit=true;if(!omit)environment.emplace_back(*item);}environment.push_back("NANO_NETWORK_TOKEN="+token);environment.push_back("NANO_SUPERVISOR_PID="+std::to_string(getpid()));std::vector<char*> env;for(auto& value:environment)env.push_back(value.data());env.push_back(nullptr);
  posix_spawnattr_t attributes;posix_spawnattr_init(&attributes);sigset_t mask,defaults;sigemptyset(&mask);sigemptyset(&defaults);sigaddset(&defaults,SIGINT);sigaddset(&defaults,SIGTERM);posix_spawnattr_setsigmask(&attributes,&mask);posix_spawnattr_setsigdefault(&attributes,&defaults);posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETSIGMASK|POSIX_SPAWN_SETSIGDEF);
  int error=posix_spawn(&pid_,executable.c_str(),nullptr,&attributes,argv.data(),env.data());posix_spawnattr_destroy(&attributes);if(error){pid_=-1;throw std::runtime_error("Cannot start nano-p2p: "+std::string(std::strerror(error)));}
 }
 ~Child(){stop();}
 bool exited(){if(pid_<0)return true;int status=0;auto result=waitpid(pid_,&status,WNOHANG);if(result==pid_||(result<0&&errno==ECHILD)){pid_=-1;return true;}return false;}
 void stop(){if(pid_<0)return;kill(pid_,SIGTERM);for(int i=0;i<50;++i){if(exited())return;std::this_thread::sleep_for(std::chrono::milliseconds(100));}kill(pid_,SIGKILL);while(waitpid(pid_,nullptr,0)<0&&errno==EINTR){}pid_=-1;}
 pid_t pid()const{return pid_;}
};
}
int run_node(Blockchain& chain,const std::filesystem::path& database,const std::filesystem::path& executable,const Json& config){
 if(config.value("dev_mode",false))throw std::invalid_argument("Development snapshots require nano serve without P2P");
 auto port=u64(quantity(config.value("rpc_port",Json(8545))));auto p2p_port=u64(quantity(config.value("p2p_port",Json(30303))));if(port>65535||p2p_port>65535)throw std::invalid_argument("Invalid node port");
 auto host=config.value("rpc_host",std::string("127.0.0.1"));if(host!="127.0.0.1"&&host!="0.0.0.0")throw std::invalid_argument("Managed P2P requires RPC bound to 127.0.0.1 or 0.0.0.0");
 auto directory=executable_directory(executable);auto binary=directory/"nano-p2p";if(!std::filesystem::exists(binary))binary=directory.parent_path()/"build-network/nano-p2p";
 binary=config.value("p2p_binary",binary.string());
 auto identity=config.value("p2p_identity",database.string()+".peerkey");auto listen=config.value("listen_addr",std::string("/ip4/")+config.value("p2p_host",std::string("0.0.0.0"))+"/tcp/"+std::to_string(p2p_port));
 BellatrixRuntime bellatrix;CapellaRuntime capella;
 if(chain.genesis().consensus=="beacon"&&!config.contains("bellatrix")&&!config.contains("capella"))throw std::invalid_argument("Beacon node requires Bellatrix or Capella runtime configuration");
 if(config.contains("capella")){
  if(host!="127.0.0.1"||config.contains("bellatrix")||config.contains("altair")||config.contains("bft"))throw std::invalid_argument("Capella service requires local RPC and exclusive consensus configuration");
  capella=start_capella_runtime(chain,config.at("capella"));
  const auto current=capella.reactor->current();
  std::cout<<Json{{"event","capella-runtime-ready"},{"sequence",quantity(Int(std::to_string(current.token.sequence)))},{"digest",hex(current.token.digest)},{"executionBound",true},{"networkProtocolActive",false}}.dump()<<std::endl;
 }
 if(config.contains("bellatrix")){
  if(host!="127.0.0.1"||config.contains("altair")||config.contains("bft"))throw std::invalid_argument("Bellatrix service requires local RPC and exclusive consensus configuration");
  bellatrix=start_bellatrix_runtime(chain,config.at("bellatrix"));
  const auto current=bellatrix.reactor->current();
  std::cout<<Json{{"event","bellatrix-runtime-ready"},{"sequence",quantity(Int(std::to_string(current.token.sequence)))},{"digest",hex(current.token.digest)},{"executionBound",true},{"networkProtocolActive",false}}.dump()<<std::endl;
 }
 AltairRuntime altair;
 if(config.contains("altair")){
  if(host!="127.0.0.1"||config.contains("bft"))throw std::invalid_argument("Altair service requires local RPC and no legacy BFT signer");
  const auto path=config.at("altair").at("database").get<std::string>();
  if(std::filesystem::weakly_canonical(path)==std::filesystem::weakly_canonical(database))throw std::invalid_argument("Altair Store requires a separate database until execution binding is implemented");
  altair=start_altair_runtime(config.at("altair"),chain.genesis().chain_id,chain.genesis().block.header.hash());
  const auto state=altair.reactor->current();
  std::cout<<Json{{"event","altair-runtime-ready"},{"sequence",quantity(Int(std::to_string(state.token.sequence)))},{"digest",hex(state.token.digest)},{"executionBound",false},{"networkProtocolActive",false}}.dump()<<std::endl;
 }
 auto token=hex(generate_private_key());Rpc rpc(chain,token,AdmissionBudget::Clock::now,RpcSimulationLimits{},altair.rpc,bellatrix.rpc,capella.reactor?std::make_shared<CapellaRuntime>(capella):nullptr);rpc.set_beacon_sync_tracker(bellatrix.sync_tracker);rpc.load_wallets(config,host);auto server=std::make_unique<HttpServer>(rpc,host,uint16_t(port),std::chrono::seconds(4),capella.reactor?capella.beacon_read:bellatrix.beacon_read,bellatrix.beacon_events);
 std::vector<std::string> args{std::to_string(server->port()),identity,listen};auto peers=config.value("bootnodes",std::vector<std::string>{});args.insert(args.end(),peers.begin(),peers.end());std::vector<std::string> excluded;for(const auto& entry:config.value("wallets",Json::array()))excluded.push_back(entry.at("password_env"));if(config.contains("bft"))excluded.push_back(config.at("bft").at("key_env").get<std::string>());for(const auto* profile:{"bellatrix","capella"})if(config.contains(profile)&&config.at(profile).contains("validator"))for(const auto& key:config.at(profile).at("validator").at("keys"))excluded.push_back(key.at("keyEnv").get<std::string>());Child child(binary,args,token,excluded);
 auto child_pid=child.pid();
 std::atomic<bool> stopping=false,failed=false;
 std::thread monitor([&]{CapellaValidatorFailureAlert signer_alert;while(!stopping){try{if(auto alert=signer_alert.poll(capella.validator.get()))std::cerr<<alert->dump()<<std::endl;}catch(...){/* Best-effort telemetry must not stop consensus admission. */}if(child.exited()){failed=true;server->stop();return;}std::this_thread::sleep_for(std::chrono::milliseconds(100));}});
 std::cout<<"Nano JSON-RPC listening on "<<host<<":"<<server->port()<<std::endl;
 std::cout<<Json{{"event","p2p-process"},{"pid",child_pid}}.dump()<<std::endl;
 try{server->run();}catch(...){stopping=true;monitor.join();throw;}
 stopping=true;monitor.join();child.stop();
 if(capella.reactor){server.reset();auto restart=stop_capella_runtime(capella);restart["event"]="capella-runtime-stopped";std::cout<<restart.dump()<<std::endl;}
 if(bellatrix.reactor){server.reset();auto restart=stop_bellatrix_runtime(bellatrix);restart["event"]="bellatrix-runtime-stopped";std::cout<<restart.dump()<<std::endl;}
 if(failed){std::cerr<<"Nano P2P process exited; node stopped\n";return 1;}return 0;
}
}
