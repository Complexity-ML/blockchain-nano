#include "nano/storage.hpp"
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
using namespace nano;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static std::map<std::string,Bytes> files(const std::filesystem::path& root){
 std::map<std::string,Bytes> result;
 for(const auto& entry:std::filesystem::recursive_directory_iterator(root))if(entry.is_regular_file()){
  std::ifstream in(entry.path(),std::ios::binary);Bytes content((std::istreambuf_iterator<char>(in)),{});
  check(!in.bad(),"cannot read evidence file");result.emplace(entry.path().lexically_relative(root).string(),sha256(content));
 }
 return result;
}
int main(){try{
 char pattern[]="/tmp/nano-read-only-XXXXXX";auto created=mkdtemp(pattern);check(created,"temporary directory");
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}} cleanup{created};
 const auto missing=cleanup.path/"missing"/"db";bool rejected=false;
 try{ChainStore store(missing,ChainStore::OpenMode::ReadOnly);}catch(const std::exception&){rejected=true;}
 check(rejected&&!std::filesystem::exists(missing.parent_path()),"read-only open created missing path");
 const auto path=cleanup.path/"chain";const Bytes genesis(32,7);const Int chain=1337;const std::string payload="unflushed-checkpoint";
 // Leave the synchronous WAL without a graceful RocksDB close/flush.
 const auto pid=fork();check(pid>=0,"fork failed");
 if(pid==0){try{auto* store=new ChainStore(path);store->bind_chain(chain,genesis,"beacon");store->commit_fork_checkpoint(chain,genesis,std::nullopt,payload,4096);_exit(0);}catch(...){_exit(1);}}
 int status=0;check(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"WAL fixture failed");
 const auto before=files(path);check(!before.empty(),"empty database fixture");
 ForkCheckpointToken token;
 {
  ChainStore store(path,ChainStore::OpenMode::ReadOnly);store.bind_chain(chain,genesis,"beacon");
  auto record=store.fork_checkpoint(chain,genesis,4096);check(record&&record->snapshot==payload,"read-only WAL recovery lost checkpoint");token=record->token;
  rejected=false;try{store.bind_chain(42,genesis,"beacon");}catch(const std::exception&){rejected=true;}check(rejected,"wrong identity accepted");
  rejected=false;try{store.commit_fork_checkpoint(chain,genesis,token,"mutation",4096);}catch(const std::exception&){rejected=true;}check(rejected,"read-only checkpoint write succeeded");
 }
 check(files(path)==before,"read-only recovery or rejected write mutated database files");
 {ChainStore store(path,ChainStore::OpenMode::ReadOnly);auto record=store.fork_checkpoint(chain,genesis,4096,token);check(record&&record->snapshot==payload,"reopen changed checkpoint");}
 check(files(path)==before,"read-only reopen mutated files");
 // Existing writable behavior must still recover and commit the next sequence.
 {ChainStore store(path);const auto next=store.commit_fork_checkpoint(chain,genesis,token,"next",4096);check(next.sequence==token.sequence+1,"writable mode regression");}
 std::cout<<"Read-only WAL recovery, source file hashes, write refusal, identity and writable reopen passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
