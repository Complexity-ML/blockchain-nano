#include "nano/storage.hpp"
#include <rocksdb/db.h>
#include <rocksdb/utilities/options_util.h>
#include <fstream>
#include <iostream>
using namespace nano;
void require(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
template<class D=rocksdb::DB> std::unique_ptr<D> open(const rocksdb::Options&o,const std::string&p){std::unique_ptr<D>d;if constexpr(requires{D::Open(o,p,&d);}){require(D::Open(o,p,&d).ok(),"open failed");}else{D*raw=nullptr;require(D::Open(o,p,&raw).ok(),"open failed");d.reset(raw);}return d;}
int main(int argc,char**argv){try{
 require(argc==3,"usage create|verify database");std::string path=argv[2],mode=argv[1];const Bytes genesis(32,7);const Int chain=1337;
 const std::string payload="portable-checkpoint-v1";
 if(mode=="create"){
  require(!std::filesystem::exists(path),"refuse existing database");
  {ChainStore store(path);store.bind_chain(chain,genesis,"beacon");store.commit_fork_checkpoint(chain,genesis,std::nullopt,payload,4096);}
  rocksdb::ConfigOptions config;rocksdb::DBOptions dbo;std::vector<rocksdb::ColumnFamilyDescriptor>cfs;
  auto status=rocksdb::LoadLatestOptions(config,path,&dbo,&cfs);require(status.ok()&&cfs.size()==1,"cannot load actual writer options");
  bool pinned=false;for(auto&f:std::filesystem::directory_iterator(path))if(f.path().filename().string().starts_with("OPTIONS-")){std::ifstream in(f.path());std::string text((std::istreambuf_iterator<char>(in)),{});pinned|=text.find("format_version=6")!=std::string::npos;}
  require(pinned,"writer did not pin format6");
  auto db=open(rocksdb::Options(dbo,cfs[0].options),path);rocksdb::FlushOptions flush;flush.wait=true;require(db->Flush(flush).ok(),"flush failed");
 }else require(mode=="verify","unknown mode");
 ChainStore store(path,ChainStore::OpenMode::ReadOnly);store.bind_chain(chain,genesis,"beacon");auto r=store.fork_checkpoint(chain,genesis,4096);require(r&&r->snapshot==payload,"checkpoint lost");require(r->token.sequence==1,"sequence mismatch");
 bool refused=false;try{store.commit_fork_checkpoint(chain,genesis,r->token,"mutation",4096);}catch(...){refused=true;}require(refused,"read-only write accepted");
 std::cout<<Json{{"mode",mode},{"sequence",r->token.sequence},{"digest",hex(r->token.digest)},{"payload",r->snapshot},{"readOnlyWriteRefused",refused}}.dump()<<'\n';
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
