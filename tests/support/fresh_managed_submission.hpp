#pragma once
// Test-only one-shot supervisor child. No signing, DB, key lookup or core edits.
#include "nano/core.hpp"
#include <openssl/evp.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <fcntl.h>
#include <poll.h>
#include <set>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>

namespace nano::test::managed_submission {
namespace fs=std::filesystem;
inline void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
inline void shape(const Json& j,std::initializer_list<const char*> keys){need(j.is_object()&&j.size()==keys.size(),"shape");for(const auto* key:keys)need(j.contains(key),"field");}
inline std::string digest(std::string_view raw){return hex(sha256(Bytes(raw.begin(),raw.end()))).substr(2);}
inline void sha(const Json& j){const auto s=j.get<std::string>();need(s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}),"digest");}
inline Bytes encoded(const Json& j,size_t width){const auto s=j.get<std::string>();need(s.size()==2+2*width,"width");auto b=unhex(s);need(b.size()==width&&hex(b)==s,"encoding");return b;}
inline uint64_t number(const Json& j){need(j.is_string(),"quantity type");const auto n=u64(quantity(j));need(quantity(Int(std::to_string(n)))==j.get<std::string>(),"quantity");return n;}
inline Json parse(std::string_view s){std::vector<std::set<std::string>> keys;return Json::parse(s.begin(),s.end(),[&](int depth,Json::parse_event_t e,Json& v){need(depth<=32,"JSON depth");if(e==Json::parse_event_t::object_start)keys.emplace_back();else if(e==Json::parse_event_t::object_end)keys.pop_back();else if(e==Json::parse_event_t::key)need(keys.back().insert(v.get<std::string>()).second,"duplicate key");return true;});}
inline bool same(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const auto am=a.st_mtimespec,bm=b.st_mtimespec,ac=a.st_ctimespec,bc=b.st_ctimespec;
#else
 const auto am=a.st_mtim,bm=b.st_mtim,ac=a.st_ctim,bc=b.st_ctim;
#endif
 return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&am.tv_sec==bm.tv_sec&&am.tv_nsec==bm.tv_nsec&&ac.tv_sec==bc.tv_sec&&ac.tv_nsec==bc.tv_nsec;
}
inline void path(const fs::path& p){need(p.is_absolute()&&p==p.lexically_normal()&&fs::canonical(p)==p,"canonical path");}
struct Fd {int n=-1;explicit Fd(int f):n(f){need(f>=0,"open");}Fd(const Fd&)=delete;~Fd(){if(n>=0)::close(n);}};
struct Held {fs::path path;struct stat material;std::string hash;uint64_t bytes;bool executable;};
class Files {
 std::vector<Held> held_;
public:
 std::function<void()> check;
 explicit Files(std::function<void()> c):check(std::move(c)){}
 std::string read(const fs::path& p,size_t maximum,bool executable=false,bool retain=true){
  check();path(p);Fd fd(::open(p.c_str(),O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC));struct stat a{},b{},z{};
  need(::fstat(fd.n,&a)==0&&S_ISREG(a.st_mode)&&a.st_uid==::geteuid()&&a.st_nlink==1&&a.st_size>0&&uint64_t(a.st_size)<=maximum&&(a.st_mode&022)==0&&(!executable||(a.st_mode&0111)),"input material");
  EVP_MD_CTX* raw=EVP_MD_CTX_new();need(raw!=nullptr,"digest context");std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context(raw,EVP_MD_CTX_free);need(EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)==1,"digest init");
  std::string out;if(retain)out.reserve(size_t(a.st_size));std::array<char,16384> buffer{};uint64_t total=0;
  for(;;){check();const auto n=::read(fd.n,buffer.data(),buffer.size());if(n<0&&errno==EINTR)continue;need(n>=0,"input read");if(!n)break;need(uint64_t(n)<=uint64_t(a.st_size)-total,"input growth");total+=uint64_t(n);need(EVP_DigestUpdate(context.get(),buffer.data(),size_t(n))==1,"digest update");if(retain)out.append(buffer.data(),size_t(n));}
  std::array<uint8_t,32> hash{};unsigned length=0;need(total==uint64_t(a.st_size)&&EVP_DigestFinal_ex(context.get(),hash.data(),&length)==1&&length==32&&::fstat(fd.n,&b)==0&&::lstat(p.c_str(),&z)==0&&same(a,b)&&same(a,z),"input changed");const auto text=hex(Bytes(hash.begin(),hash.end())).substr(2);
  const auto previous=std::find_if(held_.begin(),held_.end(),[&](const auto& row){return row.path==p;});if(previous==held_.end())held_.push_back({p,a,text,total,executable});else need(same(previous->material,a)&&previous->hash==text&&previous->bytes==total&&previous->executable==executable,"admitted input changed");return out;
 }
 std::string artifact(const Json& d,size_t maximum,bool executable=false){shape(d,{"path","sha256","bytes"});sha(d.at("sha256"));need(d.at("bytes").is_number_unsigned()&&d.at("bytes").get<uint64_t>()>0&&d.at("bytes").get<uint64_t>()<=maximum,"artifact size");const fs::path p(d.at("path").get<std::string>());const auto out=read(p,maximum,executable,!executable);const auto& row=*std::find_if(held_.begin(),held_.end(),[&](const auto& v){return v.path==p;});need(row.hash==d.at("sha256").get<std::string>()&&row.bytes==d.at("bytes").get<uint64_t>(),"artifact bytes");return out;}
 void fence(){for(size_t i=0,n=held_.size();i<n;++i){const auto row=held_[i];(void)read(row.path,size_t(row.bytes),row.executable,false);}}
};
struct Action {Json document,envelope;std::string campaign,owner,kind,payload;};
inline Action policy(const Json& plan,const Json& manifest,const Json& action){
 shape(plan,{"format","campaignId","owner","manifest","action","realP2p","resultFile","maximumWallSeconds"});need(plan.at("format")=="nano-fresh-lifecycle-submission-plan-v1","plan format");sha(plan.at("campaignId"));const auto owner=plan.at("owner").get<std::string>();need(owner=="mac-native"||owner=="mac-docker"||owner=="vps-docker","owner");need(plan.at("maximumWallSeconds").is_number_unsigned()&&plan.at("maximumWallSeconds").get<uint64_t>()>=10&&plan.at("maximumWallSeconds").get<uint64_t>()<=60,"wall budget");
 need(manifest.at("format")=="nano-isolated-capella-fixture-domain-v1"&&manifest.at("synthetic")==true&&manifest.at("productionQualified")==false&&manifest.at("fork")=="Capella/Shanghai"&&manifest.at("campaignId")==plan.at("campaignId")&&quantity(manifest.at("chainId"))==1337,"synthetic manifest");
 shape(action,{"format","campaignId","requestSha256","owner","kind","publicKey","resolvedIndex","preparedHead","payloadDigest","envelope"});need(action.at("format")=="nano-fresh-lifecycle-action-v1"&&action.at("campaignId")==plan.at("campaignId")&&action.at("owner")==owner,"action authority");sha(action.at("requestSha256"));encoded(action.at("publicKey"),48);number(action.at("resolvedIndex"));size_t matched=0;for(const auto& key:manifest.at("publicKeys"))if(key.at("publicKey")==action.at("publicKey"))++matched;need(matched==1,"action key membership");
 shape(action.at("preparedHead"),{"root","stateRoot","slot"});encoded(action.at("preparedHead").at("root"),32);encoded(action.at("preparedHead").at("stateRoot"),32);number(action.at("preparedHead").at("slot"));
 const auto kind=action.at("kind").get<std::string>();need(kind=="voluntary_exit"||kind=="proposer_slashing","action kind");const auto& envelope=action.at("envelope");shape(envelope,{"chainId","genesisHash","consensusIdentity","forkDigest","kind","ssz"});need(envelope.at("chainId")==manifest.at("chainId")&&envelope.at("genesisHash")==manifest.at("executionGenesisHash")&&envelope.at("kind")==kind,"action network");encoded(envelope.at("genesisHash"),32);encoded(envelope.at("consensusIdentity"),32);encoded(envelope.at("forkDigest"),4);auto wire=encoded(envelope.at("ssz"),kind=="voluntary_exit"?112:416);wire.insert(wire.begin(),uint8_t(kind=="voluntary_exit"?3:6));const auto payload=hex(sha256(wire));need(action.at("payloadDigest")==payload,"action payload");return {action,envelope,plan.at("campaignId").get<std::string>(),owner,kind,payload};
}
using Call=std::function<Json(const std::string&,Json)>;
inline Json accepted(const Action& action,const std::string& token,const Call& call,const std::function<void()>& check){
 need(bool(call)&&bool(check),"explicit RPC/check");encoded(Json(token),32);check();const auto status=call("nano_capellaStatus",Json::array({token}));need(status.at("executionBound")==true&&status.at("clockHealthy")==true&&status.at("validatorHealthy")==true&&status.at("networkAdmissionEnabled")==true,"managed Capella status");for(const auto* key:{"chainId","genesisHash","consensusIdentity","forkDigest"})need(status.at(key)==action.envelope.at(key),"runtime network mismatch");const auto session=status.at("admissionSession").get<std::string>();encoded(Json(session),32);
 check();const auto jobs=call("nano_capellaJobs",Json::array({token}));shape(jobs,{"session","highWater","jobs"});need(jobs.at("session")==session&&jobs.at("jobs").is_array()&&jobs.at("jobs").empty(),"undrained admission ledger");const auto high=number(jobs.at("highWater"));need(high<UINT64_MAX,"sequence overflow");const auto sequence=quantity(Int(std::to_string(high+1)));
 auto job=[&](const Json& j){const bool complete=j.at("state")=="completed";if(complete)shape(j,{"sequence","payloadDigest","cancelRequested","state","decision"});else shape(j,{"sequence","payloadDigest","cancelRequested","state"});need(j.at("sequence")==sequence&&j.at("payloadDigest")==action.payload&&j.at("cancelRequested")==false&&(complete||j.at("state")=="pending"),"job identity/state");if(complete)need(j.at("decision")=="accepted","job not accepted");return complete;};
 // Never retry a lost Submit. A refused/unknown outcome stops this child and
 // therefore its managed supervisor; the reserved result path blocks replay.
 check();auto receipt=call("nano_capellaSubmit",Json::array({token,session,sequence,action.envelope.dump()}));bool completed=job(receipt);while(!completed){check();receipt=call("nano_capellaJob",Json::array({token,session,sequence}));completed=job(receipt);if(!completed){check();::usleep(10000);}}
 check();const auto retired=call("nano_capellaForget",Json::array({token,session,sequence}));need(retired==receipt,"retired job differs");return {{"format","nano-fresh-lifecycle-accepted-job-v1"},{"campaignId",action.campaign},{"owner",action.owner},{"kind",action.kind},{"actionSha256",digest(action.document.dump())},{"session",session},{"sequence",sequence},{"job",receipt}};
}
class LocalRpc {
 uint16_t port_;std::function<void()> check_;std::function<int()> remaining_;uint64_t sequence_=0;
 void ready(int fd,short events){for(;;){check_();pollfd p{fd,events,0};const auto n=::poll(&p,1,remaining_());if(n==0||(n<0&&errno==EINTR))continue;need(n>0&&!(p.revents&(POLLERR|POLLNVAL))&&(p.revents&events),"RPC transport deadline");return;}}
public:
 LocalRpc(uint16_t port,std::function<void()> check,std::function<int()> remaining):port_(port),check_(std::move(check)),remaining_(std::move(remaining)){}
 Json operator()(const std::string& method,Json params){
  check_();Fd s(::socket(AF_INET,SOCK_STREAM,0));need(::fcntl(s.n,F_SETFD,FD_CLOEXEC)==0&&::fcntl(s.n,F_SETFL,O_NONBLOCK)==0,"socket flags");
#ifdef __APPLE__
  int one=1;need(::setsockopt(s.n,SOL_SOCKET,SO_NOSIGPIPE,&one,sizeof(one))==0,"socket signal policy");
#endif
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(port_);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);const auto connected=::connect(s.n,reinterpret_cast<sockaddr*>(&address),sizeof(address));need(connected==0||errno==EINPROGRESS,"RPC connect");if(connected){ready(s.n,POLLOUT);int error=0;socklen_t length=sizeof(error);need(::getsockopt(s.n,SOL_SOCKET,SO_ERROR,&error,&length)==0&&error==0,"RPC connection");}
  const auto id=++sequence_;const auto body=Json{{"jsonrpc","2.0"},{"id",id},{"method",method},{"params",std::move(params)}}.dump();need(body.size()<=65536,"RPC request bound");const auto request="POST / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n"+body;
  for(size_t at=0;at<request.size();){ready(s.n,POLLOUT);
#ifdef MSG_NOSIGNAL
   constexpr int flags=MSG_NOSIGNAL;
#else
   constexpr int flags=0;
#endif
   const auto n=::send(s.n,request.data()+at,request.size()-at,flags);if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))continue;need(n>0,"RPC write");at+=size_t(n);}
  std::string response;std::array<char,4096> buffer{};std::optional<size_t> required;size_t start=0;
  for(;;){ready(s.n,POLLIN);const auto n=::recv(s.n,buffer.data(),buffer.size(),0);if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))continue;need(n>0,"RPC truncated");need(size_t(n)<=1048576-response.size(),"RPC response bound");response.append(buffer.data(),size_t(n));if(!required){const auto end=response.find("\r\n\r\n");if(end!=std::string::npos){need(end<=8192&&response.starts_with("HTTP/1.1 200 "),"RPC HTTP status/header");const auto first=response.find("\r\n");std::set<std::string> fields;std::optional<size_t> length;for(size_t at=first+2;at<end;){const auto next=response.find("\r\n",at),colon=response.find(':',at);need(next!=std::string::npos&&colon<next,"RPC header");auto name=response.substr(at,colon-at);std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});need(fields.insert(name).second&&name!="transfer-encoding","RPC duplicate/encoding header");if(name=="content-length"){const auto text=response.substr(colon+1,next-colon-1);size_t a=0;while(a<text.size()&&text[a]==' ')++a;size_t value=0;const auto parsed=std::from_chars(text.data()+a,text.data()+text.size(),value);need(parsed.ec==std::errc()&&parsed.ptr==text.data()+text.size()&&value<=1048576-end-4,"RPC length");length=value;}at=next+2;}need(bool(length),"RPC content length");start=end+4;required=start+*length;}else need(response.size()<=8192,"RPC header bound");}if(required&&response.size()>=*required){need(response.size()==*required,"RPC extra bytes");break;}}
  const auto result=parse(std::string_view(response).substr(start));shape(result,{"jsonrpc","id","result"});need(result.at("jsonrpc")=="2.0"&&result.at("id")==id,"RPC response envelope");check_();return result.at("result");
 }
};
inline void private_parent(const fs::path& p){path(p);struct stat s{};need(::lstat(p.c_str(),&s)==0&&S_ISDIR(s.st_mode)&&s.st_uid==::geteuid()&&(s.st_mode&07777)==0700,"private control parent");}
inline int child(int argc,char** argv){
 need(argc>=4&&argc<=128,"managed child argv");size_t argument_bytes=0;for(int i=0;i<argc;++i){need(argv[i]!=nullptr,"child argument");const auto n=::strnlen(argv[i],65537);need(n<=65536-argument_bytes,"child argument bound");argument_bytes+=n;}
 const char* plan_env=::getenv("NANO_TEST_FRESH_SUBMISSION_PLAN"),*hash_env=::getenv("NANO_TEST_FRESH_SUBMISSION_PLAN_SHA256"),*token_env=::getenv("NANO_NETWORK_TOKEN"),*parent_env=::getenv("NANO_SUPERVISOR_PID");need(plan_env&&hash_env&&token_env&&parent_env&&::strnlen(plan_env,4097)<=4096&&::strnlen(hash_env,65)==64&&::strnlen(token_env,67)==66&&::strnlen(parent_env,21)<=20,"supervisor context");const fs::path plan_path(plan_env);const std::string expected_hash(hash_env),token(token_env);sha(Json(expected_hash));encoded(Json(token),32);
 auto decimal=[](const char* value,uint64_t maximum){const std::string_view text(value);uint64_t n=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),n);need(!text.empty()&&text.size()<=20&&parsed.ec==std::errc()&&parsed.ptr==text.data()+text.size()&&std::to_string(n)==text&&n>0&&n<=maximum,"decimal context");return n;};const auto parent=decimal(parent_env,INT32_MAX),port=decimal(argv[1],65535);need(parent>1&&uint64_t(::getppid())==parent,"supervisor PID");const auto began=std::chrono::steady_clock::now();uint64_t seconds=60;
 const auto check=[&]{need(uint64_t(::getppid())==parent&&std::chrono::steady_clock::now()-began<std::chrono::seconds(seconds),"child parent/deadline");};Files inputs(check);private_parent(plan_path.parent_path());const auto plan_raw=inputs.read(plan_path,65536);need(digest(plan_raw)==expected_hash,"external plan digest");struct stat pm{};need(::lstat(plan_path.c_str(),&pm)==0&&(pm.st_mode&07777)==0600,"private plan mode");const auto plan=parse(plan_raw);shape(plan,{"format","campaignId","owner","manifest","action","realP2p","resultFile","maximumWallSeconds"});need(plan.at("maximumWallSeconds").is_number_unsigned(),"deadline type");seconds=plan.at("maximumWallSeconds").get<uint64_t>();need(seconds>=10&&seconds<=60,"deadline bound");
 const auto manifest=parse(inputs.artifact(plan.at("manifest"),16*1024*1024));const auto action_raw=inputs.artifact(plan.at("action"),65536);struct stat action_mode{};need(::lstat(plan.at("action").at("path").get<std::string>().c_str(),&action_mode)==0&&(action_mode.st_mode&07777)==0600,"private action mode");const auto action=policy(plan,manifest,parse(action_raw));need(action_raw==action.document.dump(),"canonical action bytes");inputs.artifact(plan.at("realP2p"),64*1024*1024,true);const fs::path executable(plan.at("realP2p").at("path").get<std::string>()),output(plan.at("resultFile").get<std::string>());need(executable.filename()=="nano-p2p","real P2P leaf");need(output.is_absolute()&&output==output.lexically_normal()&&output.filename()!="."&&output.filename()!=".."&&!output.filename().empty(),"result path");private_parent(output.parent_path());need(output!=plan_path&&output!=executable&&output!=fs::path(plan.at("action").at("path").get<std::string>())&&output!=fs::path(plan.at("manifest").at("path").get<std::string>()),"result roles");
 // The reservation survives every refusal. It forbids automatic plan reuse
 // after a possibly accepted Submit whose transport outcome became unknown.
 Fd result(::open(output.c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));struct stat output_identity{};need(::fstat(result.n,&output_identity)==0&&output_identity.st_uid==::geteuid()&&output_identity.st_nlink==1&&(output_identity.st_mode&07777)==0600,"result reservation");std::string result_raw;
 auto output_fence=[&]{struct stat a{},b{};need(::fstat(result.n,&a)==0&&::lstat(output.c_str(),&b)==0&&same(a,b)&&S_ISREG(a.st_mode)&&a.st_dev==output_identity.st_dev&&a.st_ino==output_identity.st_ino&&a.st_uid==output_identity.st_uid&&a.st_gid==output_identity.st_gid&&a.st_nlink==1&&(a.st_mode&07777)==0600&&a.st_size>=0&&uint64_t(a.st_size)==result_raw.size(),"result material");std::string readback(result_raw.size(),'\0');size_t at=0;while(at<readback.size()){check();const auto n=::pread(result.n,readback.data()+at,readback.size()-at,off_t(at));if(n<0&&errno==EINTR)continue;need(n>0,"result truncated");at+=size_t(n);}struct stat after{};need(readback==result_raw&&::fstat(result.n,&after)==0&&::lstat(output.c_str(),&b)==0&&same(a,after)&&same(a,b),"result bytes changed");};
 try{
  inputs.fence();LocalRpc rpc(uint16_t(port),check,[&]{check();const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();return int(std::max<int64_t>(1,std::min<int64_t>(1000,int64_t(seconds*1000)-elapsed)));});const auto receipt=accepted(action,token,[&](const auto& method,Json params){return rpc(method,std::move(params));},check);need(receipt.at("actionSha256")==plan.at("action").at("sha256"),"receipt action pin");result_raw=receipt.dump();size_t at=0;while(at<result_raw.size()){check();const auto n=::write(result.n,result_raw.data()+at,result_raw.size()-at);if(n<0&&errno==EINTR)continue;need(n>0,"result write");at+=size_t(n);}need(::fsync(result.n)==0,"result fsync");Fd parent_fd(::open(output.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));need(::fsync(parent_fd.n)==0,"result parent fsync");inputs.fence();output_fence();check();
  need(::unsetenv("NANO_TEST_FRESH_SUBMISSION_PLAN")==0&&::unsetenv("NANO_TEST_FRESH_SUBMISSION_PLAN_SHA256")==0,"remove test controls");std::string daemon=executable.string();std::vector<char*> args{daemon.data()};for(int i=1;i<argc;++i)args.push_back(argv[i]);args.push_back(nullptr);
  std::cout<<Json{{"event","fresh-lifecycle-child-submission-accepted"},{"accepted",true},{"receiptSha256",digest(result_raw)},{"receiptBytes",result_raw.size()},{"authExported",false},{"newSignaturesCreated",0},{"execPending",true},{"productionQualified",false}}.dump()<<std::endl;::execv(daemon.c_str(),args.data());throw std::runtime_error("P2P exec failed");
 }catch(...){inputs.fence();output_fence();throw;}
}
} // namespace nano::test::managed_submission
