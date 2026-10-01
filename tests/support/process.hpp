#pragma once
#include "nano/client.hpp"
#include <spawn.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <map>
extern char** environ;
namespace test {
using namespace nano;
using namespace std::chrono_literals;
namespace fs=std::filesystem;
using Env=std::map<std::string,std::optional<std::string>>;
inline void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
inline std::string read(const fs::path& path){std::ifstream f(path);check(bool(f),"read "+path.string());return {std::istreambuf_iterator<char>(f),{}};}
inline Json load(const fs::path& path){return Json::parse(read(path));}
inline void save(const fs::path& path,const Json& data){std::ofstream f(path);f<<data.dump();check(bool(f),"write "+path.string());}
struct Temp {fs::path path;Temp(){std::string pattern=(fs::temp_directory_path()/"nano-native-test-XXXXXX").string();check(mkdtemp(pattern.data()),"mkdtemp");path=pattern;}~Temp(){std::error_code error;fs::remove_all(path,error);}fs::path operator/(const std::string& name)const{return path/name;}};
struct Result {int code;std::string out,err;};
// Shell-free subprocess ownership with separate output streams and bounded waits.
class Process {
 pid_t pid_=-1;int output_=-1,error_=-1;std::optional<int> status_;std::string out_,err_;
 static void close_fd(int& fd){if(fd>=0)close(fd);fd=-1;}
 static void drain(int& fd,std::string& target){if(fd<0)return;char buffer[8192];for(;;){auto n=::read(fd,buffer,sizeof(buffer));if(n>0){target.append(buffer,n);check(target.size()<=8*1024*1024,"child output limit");}else if(n==0){close_fd(fd);break;}else if(errno==EINTR)continue;else if(errno==EAGAIN||errno==EWOULDBLOCK)break;else throw std::runtime_error("read child pipe");}}
public:
 Process(const std::vector<std::string>& args,const Env& overrides={},const fs::path& cwd={}){
  check(!args.empty(),"missing child command");int out[2],err[2];check(pipe(out)==0,"stdout pipe");if(pipe(err)!=0){close(out[0]);close(out[1]);throw std::runtime_error("stderr pipe");}
  for(int fd:{out[0],out[1],err[0],err[1]})fcntl(fd,F_SETFD,FD_CLOEXEC);
  posix_spawn_file_actions_t actions;posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_adddup2(&actions,out[1],STDOUT_FILENO);posix_spawn_file_actions_adddup2(&actions,err[1],STDERR_FILENO);
  posix_spawn_file_actions_addopen(&actions,STDIN_FILENO,"/dev/null",O_RDONLY,0);
  if(!cwd.empty())posix_spawn_file_actions_addchdir_np(&actions,cwd.c_str());
  std::map<std::string,std::string> env;for(char** e=environ;*e;++e){std::string entry=*e;auto equals=entry.find('=');env[entry.substr(0,equals)]=entry.substr(equals+1);}
  for(const auto& [key,value]:overrides){if(value)env[key]=*value;else env.erase(key);}
  std::vector<std::string> environment;for(const auto& [key,value]:env)environment.push_back(key+"="+value);
  std::vector<char*> argv,envp;for(const auto& arg:args)argv.push_back(const_cast<char*>(arg.c_str()));argv.push_back(nullptr);for(auto& e:environment)envp.push_back(e.data());envp.push_back(nullptr);
  std::string executable=args[0];
  if(executable.find('/')==std::string::npos){
   std::istringstream search(env.contains("PATH")?env.at("PATH"):"/usr/bin:/bin");std::string directory;bool found=false;
   while(std::getline(search,directory,':')){fs::path candidate=(directory.empty()?fs::path("."):fs::path(directory))/executable;if(candidate.is_relative())candidate=(cwd.empty()?fs::current_path():cwd)/candidate;if(access(candidate.c_str(),X_OK)==0){executable=candidate.string();found=true;break;}}
   if(!found){posix_spawn_file_actions_destroy(&actions);for(int fd:{out[0],out[1],err[0],err[1]})close(fd);throw std::runtime_error("executable absent from child PATH: "+args[0]);}
  }
  auto code=posix_spawn(&pid_,executable.c_str(),&actions,nullptr,argv.data(),envp.data());posix_spawn_file_actions_destroy(&actions);close(out[1]);close(err[1]);
  if(code){close(out[0]);close(err[0]);pid_=-1;throw std::runtime_error("spawn "+args[0]+": "+std::strerror(code));}
  output_=out[0];error_=err[0];fcntl(output_,F_SETFL,O_NONBLOCK);fcntl(error_,F_SETFL,O_NONBLOCK);
 }
 Process(const Process&)=delete;Process& operator=(const Process&)=delete;
 ~Process(){try{stop();}catch(...){if(pid_>0&&!status_){kill(pid_,SIGKILL);int status;while(waitpid(pid_,&status,0)<0&&errno==EINTR){}}}close_fd(output_);close_fd(error_);}
 void pump(int timeout=20){pollfd fds[2]={{output_,POLLIN,0},{error_,POLLIN,0}};poll(fds,2,timeout);drain(output_,out_);drain(error_,err_);}
 bool running(){if(status_)return false;int value;auto result=waitpid(pid_,&value,WNOHANG);if(result==pid_){status_=WIFEXITED(value)?WEXITSTATUS(value):-WTERMSIG(value);return false;}check(result==0||(result<0&&errno==EINTR),"waitpid failed");return true;}
 void signal(int value){if(running())check(kill(pid_,value)==0,"signal child failed");}
 Result wait(int seconds=20){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);while(running()){check(std::chrono::steady_clock::now()<deadline,"child timeout: "+out_+err_);pump();}pump(0);return {*status_,out_,err_};}
 Result stop(){if(pid_<=0)return {0,{},{}};if(running()){signal(SIGTERM);auto end=std::chrono::steady_clock::now()+5s;while(running()&&std::chrono::steady_clock::now()<end)pump();if(running())signal(SIGKILL);}return wait(5);}
 std::string line(const std::function<bool(const std::string&)>& predicate,int seconds=20){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);for(;;){pump();std::istringstream lines(out_);std::string line;while(std::getline(lines,line))if(predicate(line))return line;check(running()&&std::chrono::steady_clock::now()<end,"missing child output: "+out_+err_);}}
 Json event(const std::string& name){return Json::parse(line([&](const std::string& s){auto j=Json::parse(s,nullptr,false);return j.is_object()&&j.value("event","")==name;}));}
 std::string output(){pump(0);return out_;}
 int port(){auto value=line([](const std::string& s){return s.find("listening on")!=std::string::npos;});return std::stoi(value.substr(value.rfind(':')+1));}
};
inline Result run(const std::vector<std::string>& args,const Env& env={},const fs::path& cwd={}){Process child(args,env,cwd);return child.wait();}
inline Json success(const Result& result){check(result.code==0,result.err+result.out);return Json::parse(result.out);}
inline std::string url(int port){return "http://127.0.0.1:"+std::to_string(port);}
inline Json rpc(int port,const std::string& method,Json params=Json::array()){RpcClient client({url(port)},5s,1);return client.call(method,std::move(params));}
inline Json rpc(int port,const std::string& method,std::initializer_list<Json> params){auto array=Json::array();for(const auto& value:params)array.push_back(value);return rpc(port,method,std::move(array));}
inline void until(const std::function<bool()>& predicate,const std::string& label,int seconds=25){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);while(std::chrono::steady_clock::now()<end){if(predicate()){std::cout<<label<<std::endl;return;}std::this_thread::sleep_for(100ms);}throw std::runtime_error("timeout: "+label);}
inline std::string key(unsigned n=1){return hex(bytes(Int(n),32));}
inline const std::string account="0x7E5F4552091A69125d5DfCb7b8C2659029395Bdf";
inline const std::string bootstrap="0x311384E7e4803AdBDB1D8ab46b973eeE0f9cF55B";
}
