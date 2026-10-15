#include "support/process.hpp"
using namespace test;

namespace {
// Exercise the installed entry points. No genesis/database/request argument is
// supplied, so the four refusing programs must stop before opening state.
void runtime_install_smoke(const fs::path& prefix,const fs::path& working){
 for(const char* name:{"nano","nano-p2p","nano-storage-maintenance","nano-bootstrap-archive-maintenance","nano-capella-duty-readiness"}){
  const auto binary=prefix/"bin"/name;
  check(fs::is_regular_file(binary)&&!fs::is_symlink(binary),std::string("installed executable missing: ")+name);
 }
 check(fs::is_regular_file(prefix/"share/nano/licenses/notices.sha256"),"installed notice manifest missing");
 fs::create_directory(working);
 const Env clean{{"NANO_SUPERVISOR_PID",std::nullopt}};
 const auto invoke=[&](const std::vector<std::string>& args){Process child(args,clean,working);return child.wait(5);};
 const auto call=[&](const char* name){return invoke({(prefix/"bin"/name).string()});};
 const auto help=invoke({(prefix/"bin/nano").string(),"--help"});
 check(help.code==0&&help.out.find("Usage: nano <")!=std::string::npos,"installed nano help failed");
 const auto p2p=call("nano-p2p");
 check(p2p.code==1&&p2p.err.find("Usage: nano-p2p ")!=std::string::npos,"installed transport did not refuse before startup");
 const auto compact=call("nano-storage-maintenance");
 check(compact.code==1&&compact.err.find("Usage: compact-owned-copy ")!=std::string::npos,"installed storage tool did not refuse before opening a copy");
 const auto bootstrap=call("nano-bootstrap-archive-maintenance");
 check(bootstrap.code==1,"installed bootstrap tool did not refuse empty arguments");
 const auto b=Json::parse(bootstrap.out);
 check(b.at("completed")==false&&b.at("refusedStage")=="arguments"&&b.at("runtimeStarted")==false&&b.at("productionQualified")==false,"installed bootstrap refusal stage differs");
 const auto duty=call("nano-capella-duty-readiness");
 check(duty.code==1,"installed duty tool did not refuse empty arguments");
 const auto d=Json::parse(duty.out);
 check(d.at("status")=="REFUSED_NO_AUTOMATIC_RETRY"&&d.at("refusedStage")=="request"&&d.at("reason")=="validation_or_io_refused"&&d.at("probeCompleted")==false&&d.at("keysLoaded")==false&&d.at("runtimeStarted")==false,"installed duty refusal stage differs");
 check(fs::is_empty(working),"argument-only installed probes wrote state");
}
}

int main(int argc,char** argv){try{
 check(argc==3||(argc==4&&std::string(argv[3])=="--runtime-install-smoke"),"cmake and build required; optional --runtime-install-smoke");
 const bool smoke_only=argc==4;
 Temp root;auto prefix=root/"prefix with spaces";
 auto installed=run({argv[1],"--install",argv[2],"--prefix",prefix.string()});
 check(installed.code==0,installed.err);
 check(fs::is_regular_file(prefix/"bin/nano")&&fs::is_regular_file(prefix/"bin/nano-p2p")&&fs::is_regular_file(prefix/"share/nano/genesis/testnet.json"),"installed files missing");
 if(smoke_only){runtime_install_smoke(prefix,root/"argument-only entrypoints");std::cout<<"Installed five-executable argument-only smoke passed; no database or network started\n";return 0;}
 for(std::string name:{"mainnet","testnet"}){auto signed_file=prefix/("share/nano/genesis/"+name+".json"),legacy=prefix/("share/nano/genesis/legacy/"+name+"-unsigned.json");check(load(signed_file)["config"]["posSignatureVersion"]==1,"signed genesis absent");auto binary=(prefix/"bin/nano").string();auto old=success(run({binary,"init",legacy.string(),(root/(name+"-old")).string()})),fresh=success(run({binary,"init",signed_file.string(),(root/(name+"-new")).string()}));check(old["blockHash"]!=fresh["blockHash"],"genesis identities collide");auto refused=run({binary,"status",signed_file.string(),(root/(name+"-old")).string()});check(refused.code!=0&&refused.err.find("database belongs to another chain")!=std::string::npos,"genesis mismatch accepted");success(run({binary,"status",legacy.string(),(root/(name+"-old")).string()}));}
 auto docs=prefix/"share/nano/docs";for(auto name:{"README.md","GENESIS-VERSIONS.md","FINALITY.md","NETWORK-ROBUSTNESS.md"})check(fs::is_regular_file(docs/name),"missing installed documentation");
 auto moved=root/"relocated installation";fs::rename(prefix,moved);auto aliases=root/"aliases";fs::create_directory(aliases);fs::create_symlink(moved/"bin/nano",aliases/"nano");auto working=root/"unrelated";fs::create_directory(working);auto genesis=root/"genesis.json",config=root/"config.json";save(genesis,{{"config",{{"chainId",1337},{"consensus","pos"}}}});save(config,{{"rpc_port",0},{"p2p_port",0},{"p2p_host","127.0.0.1"}});
 for(std::string name:{"PATH","symlink"}){auto dir=name=="PATH"?moved/"bin":aliases;const char* path=std::getenv("PATH");Env env={{"PATH",dir.string()+":"+(path?path:"")}};auto database=root/("database-"+name);Process process({"nano","run",genesis.string(),database.string(),config.string()},env,working);int port=process.port(),child=process.event("p2p-process")["pid"];process.event("ready");check(rpc(port,"eth_chainId")=="0x539","relocated RPC failed");check(process.stop().code==0,"relocated stop failed");check(kill(child,0)<0&&errno==ESRCH,"transport child not reaped");check(success(run({"nano","status",genesis.string(),database.string()},env,working))["blockNumber"]=="0x0","relocated status failed");}
 std::cout<<"Native relocated installation checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
