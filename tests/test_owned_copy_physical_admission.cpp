#include "support/process.hpp"
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/write_batch.h>
#include <array>
#include <chrono>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace test;
namespace {
const auto began=std::chrono::steady_clock::now();
uint64_t elapsed(){return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count());}
uint64_t rss(){rusage s{};check(::getrusage(RUSAGE_SELF,&s)==0,"RSS observation");
#ifdef __APPLE__
 return uint64_t(s.ru_maxrss)/1024;
#else
 return uint64_t(s.ru_maxrss);
#endif
}
void bounded(){check(elapsed()<120000&&rss()<=131072,"test resource budget");}
void private_dir(const fs::path& p){check(fs::create_directory(p),"new private directory");check(::chmod(p.c_str(),0700)==0,"private directory mode");}
void file(const fs::path& p,std::string_view b){int fd=::open(p.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);check(fd>=0,"new owned file");size_t pos=0;while(pos<b.size()){auto n=::write(fd,b.data()+pos,b.size()-pos);if(n<0&&errno==EINTR)continue;if(n<=0){::close(fd);throw std::runtime_error("fixture write");}pos+=size_t(n);}check(::fsync(fd)==0,"fixture fsync");check(::close(fd)==0,"fixture close");}
std::string digest(std::string_view s){return hex(sha256(Bytes(s.begin(),s.end()))).substr(2);}
std::array<int64_t,9> material(const struct stat& s){
#ifdef __APPLE__
 const auto mt=int64_t(s.st_mtimespec.tv_sec)*1000000000+s.st_mtimespec.tv_nsec;
 const auto ct=int64_t(s.st_ctimespec.tv_sec)*1000000000+s.st_ctimespec.tv_nsec;
#else
 const auto mt=int64_t(s.st_mtim.tv_sec)*1000000000+s.st_mtim.tv_nsec;
 const auto ct=int64_t(s.st_ctim.tv_sec)*1000000000+s.st_ctim.tv_nsec;
#endif
 return {int64_t(s.st_dev),int64_t(s.st_ino),int64_t(s.st_mode),int64_t(s.st_uid),int64_t(s.st_gid),int64_t(s.st_nlink),int64_t(s.st_size),mt,ct};
}
using Inventory=std::map<std::string,Json>;
// Refusal oracle uses lstat for every type, never follows links or opens FIFOs.
// Atime is excluded; all other nine fields and all regular bytes are retained.
Inventory inventory(const fs::path& root){Inventory out;uint64_t bytes=0;auto one=[&](const fs::path& p){bounded();check(out.size()<4096,"inventory file bound");struct stat a{},b{},f{};check(::lstat(p.c_str(),&a)==0,"inventory lstat");Json row{{"material",material(a)}};if(S_ISREG(a.st_mode)){check(a.st_size>=0&&uint64_t(a.st_size)<=64*1024*1024-bytes,"inventory byte bound");bytes+=uint64_t(a.st_size);int fd=::open(p.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);check(fd>=0,"inventory regular FD");check(::fstat(fd,&f)==0&&material(f)==material(a),"inventory FD/path binding");std::string raw;raw.reserve(size_t(a.st_size));std::array<char,8192> block{};for(;;){auto n=::read(fd,block.data(),block.size());if(n<0&&errno==EINTR)continue;check(n>=0,"inventory read");if(!n)break;raw.append(block.data(),size_t(n));check(raw.size()<=uint64_t(a.st_size),"inventory file grew");}check(::fstat(fd,&f)==0&&material(f)==material(a),"inventory FD stability");check(::close(fd)==0,"inventory close");check(raw.size()==uint64_t(a.st_size),"inventory length");row["sha256"]=digest(raw);}else if(S_ISLNK(a.st_mode)){row["target"]=fs::read_symlink(p).string();}check(::lstat(p.c_str(),&b)==0&&material(a)==material(b),"inventory path stability");check(out.emplace(p.lexically_relative(root).string(),std::move(row)).second,"inventory unique path");};one(root);for(const auto& x:fs::recursive_directory_iterator(root))one(x.path());return out;}
rocksdb::Options options(){rocksdb::Options o;o.create_if_missing=false;o.paranoid_checks=true;o.allow_fallocate=false;rocksdb::BlockBasedTableOptions table;table.format_version=6;o.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table));return o;}
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_db(const fs::path& p,bool create,bool ro=false){
 auto o=options();o.create_if_missing=create;std::unique_ptr<Database> result;
 const auto good=[](const rocksdb::Status& status){check(status.ok(),"synthetic RocksDB open: "+status.ToString());};
 if(ro){if constexpr(requires { Database::OpenForReadOnly(o,p.string(),&result); })good(Database::OpenForReadOnly(o,p.string(),&result));else{Database* raw=nullptr;auto status=Database::OpenForReadOnly(o,p.string(),&raw);result.reset(raw);good(status);}}
 else{if constexpr(requires { Database::Open(o,p.string(),&result); })good(Database::Open(o,p.string(),&result));else{Database* raw=nullptr;auto status=Database::Open(o,p.string(),&raw);result.reset(raw);good(status);}}
 return result;
}
using Rows=std::map<std::string,std::string>;
Rows logical(const fs::path& p){auto db=open_db(p,false,true);rocksdb::ReadOptions o;o.verify_checksums=true;o.fill_cache=false;std::unique_ptr<rocksdb::Iterator> i(db->NewIterator(o));Rows rows;size_t bytes=0;for(i->SeekToFirst();i->Valid();i->Next()){bounded();bytes+=i->key().size()+i->value().size();check(rows.size()<256&&bytes<=1024*1024,"synthetic logical bound");rows.emplace(i->key().ToString(),i->value().ToString());}check(i->status().ok(),"synthetic logical scan");return rows;}
struct Fixture{
 fs::path root,source,copy,request;bool migration;
 Fixture(const fs::path& parent,const std::string& name,bool migrate):root(parent/name),migration(migrate){private_dir(root);private_dir(root/"original");private_dir(root/"candidate");source=root/"original/chain";copy=root/"candidate/chain";private_dir(source);const std::string payload="public synthetic opaque checkpoint payload";const std::string sum="0x"+digest(payload);Json token{{"sequence","0x1"},{"digest",sum}};{auto db=open_db(source,true);rocksdb::WriteBatch batch;batch.Put("meta/schema","nano/1");batch.Put("fork-checkpoint/v1/token",token.dump());batch.Put("fork-checkpoint/v1/payload",payload);batch.Put("execution-checkpoint/v1",Json{{"digest",sum}}.dump());batch.Put("execution-state-pruning/v1",Json{{"before","0x2"},{"anchorNumber","0x2"},{"anchorHash","0x"+std::string(64,'1')}}.dump());for(unsigned i=0;i<32;++i)batch.Put("public/"+std::to_string(i),std::string(1024,char('a'+i%26)));rocksdb::WriteOptions w;w.sync=true;check(db->Write(w,&batch).ok(),"synthetic fixture batch");rocksdb::FlushOptions f;f.wait=true;check(db->Flush(f).ok(),"synthetic fixture SST");}check(::chmod(source.c_str(),0700)==0,"synthetic source root");fs::copy(source,copy,fs::copy_options::recursive);check(::chmod(copy.c_str(),0700)==0,"synthetic copy root");const auto marker=root/"candidate"/(migration?".nano-owned-schema-migration-copy":".nano-owned-compaction-copy");file(marker,migration?"owned-offline-schema-migration-v1\n":"owned-offline-qualification-copy-v1\n");request=root/"request.json";file(request,Json{{"expectedCheckpoint",token},{"limits",{{"stateBytes","0x100000"}}}}.dump());check(logical(source)==logical(copy),"fresh copy logical equality");
 }
 fs::path marker()const{return root/"candidate"/(migration?".nano-owned-schema-migration-copy":".nano-owned-compaction-copy");}
 std::vector<std::string> args(const std::string& executable={})const{const auto binary=executable.empty()?std::string(migration?NANO_TEST_MIGRATE:NANO_TEST_COMPACT):executable;std::vector<std::string> a{binary,"--offline-owned-copy",copy.string()};if(migration){a.push_back("--audit-request");a.push_back(request.string());}return a;}
};
uint64_t calls=0;
Result invoke(const Fixture& f,const std::string& binary={}){bounded();Process child(f.args(binary),{{"NANO_TEST_MIGRATION_STOP",std::nullopt}});++calls;auto r=child.wait(10);bounded();return r;}
void emit(const char* name,Json fields){bounded();fields["scenario"]=name;fields["result"]="PASS";fields["newSignaturesCreated"]=0;fields["privateInputsRead"]=false;fields["productionQualified"]=false;fields["elapsedMs"]=elapsed();fields["maxRssKiB"]=rss();std::cout<<fields.dump()<<std::endl;}
void independent(const fs::path& p){uint64_t start=calls;for(bool migration:{false,true}){Fixture f(p,migration?"independent-migration":"independent-compaction",migration);private_dir(f.copy/"ordinary-subdirectory");file(f.copy/"ordinary-subdirectory/public-leaf","ordinary independent bytes");check(::chmod(f.copy.c_str(),0755)==0,"nonprivate copy mode remains supported");const auto source=inventory(f.root/"original");const auto expected=logical(f.copy);const auto r=invoke(f);check(r.code==0,"valid independent CLI copy refused: "+r.err);const auto report=Json::parse(r.out);auto after=logical(f.copy);if(migration){check(report.at("onlySchemaChanged")==true,"migration did not attest exact non-schema equality");auto changed=expected;changed["meta/schema"]="nano/2-state-pruned";check(after==changed,"migration changed anything except schema");const auto again=invoke(f);check(again.code==0,"migration idempotent replay refused");const auto j=Json::parse(again.out);check(j.at("alreadyMigrated")==true&&j.at("writes")==0,"migration replay wrote");check(logical(f.copy)==changed,"migration replay changed logical rows");}else check(report.at("logicalContentUnchanged")==true&&after==expected,"compaction altered logical rows");check(inventory(f.root/"original")==source,"successful independent CLI modified source");}emit("owned-copy-independent-cli-success",{{"actualCliInvocations",calls-start},{"compactionLogicalRowsExact",true},{"migrationOnlySchemaChanged",true},{"migrationReplayWritesZero",true},{"originalBytesAndMaterialUnchanged",true},{"descendantDirectoriesAccepted",true},{"root0755Accepted",true}});}
// Each case contains a complete, otherwise accepted synthetic database. An
// irrelevant extra leaf therefore cannot hide a mutable RocksDB open.
void refused(const fs::path& p,unsigned mode,const char* name,const std::function<void(Fixture&)>& alter){uint64_t start=calls;for(bool migration:{false,true}){Fixture f(p,std::string(name)+(migration?"-migration":"-compaction"),migration);alter(f);const auto before=inventory(f.root);auto r=invoke(f);check(r.code==1,"unsafe physical copy was not refused");check(!r.err.empty(),"physical refusal diagnostic missing");check(inventory(f.root)==before,"refusal opened/mutated source, copy, marker or external alias");}emit(name,{{"caseId",mode},{"actualCliInvocations",calls-start},{"bothCliRefused",true},{"allBytesAndNineMaterialFieldsUnchanged",true},{"atimeCompared",false}});}
void marker_aliases(const fs::path& p){const auto start=calls;for(bool migration:{false,true})for(unsigned variant=0;variant<2;++variant){Fixture f(p,std::string("marker-alias-")+(migration?"migration-":"compact-")+std::to_string(variant),migration);if(variant==0)fs::create_hard_link(f.marker(),f.root/"external-marker");else{const auto bytes=read(f.marker());fs::remove(f.marker());file(f.root/"external-marker",bytes);fs::create_symlink(f.root/"external-marker",f.marker());}const auto before=inventory(f.root);const auto r=invoke(f);check(r.code==1&&!r.err.empty(),"marker alias was not refused");check(inventory(f.root)==before,"marker alias refusal changed source/copy/external marker");}emit("owned-copy-marker-alias-refusal",{{"caseId",8},{"actualCliInvocations",calls-start},{"bothCliRefused",true},{"allBytesAndNineMaterialFieldsUnchanged",true},{"atimeCompared",false},{"markerHardlinkAndSymlinkRefused",true}});}

void baseline(const fs::path& p,const std::string& compact,const std::string& migrate){for(bool migration:{false,true}){Fixture f(p,migration?"baseline-migrate":"baseline-compact",migration);file(f.root/"original/public-alias", "public synthetic alias bytes");fs::create_hard_link(f.root/"original/public-alias",f.copy/"public-alias");const auto original=inventory(f.root/"original");const auto r=invoke(f,migration?migrate:compact);check(r.code==0,"old CLI did not accept aliased synthetic copy: "+r.err);check(inventory(f.root/"original")==original,"baseline altered external alias unexpectedly");}emit("owned-copy-old-cli-alias-acceptance",{{"actualCliInvocations",calls},{"oldAcceptedPhysicallyAliasedCopy",true},{"originalBytesAndMaterialUnchanged",true},{"baselineConsumerObjectMayDiffer",true},{"correctedCodeQualified",false}});}
}
int main(int argc,char** argv){try{Temp temp;check(::chmod(temp.path.c_str(),0700)==0,"private test root");if(argc==4&&std::string(argv[1])=="--baseline-aliased-cli"){baseline(temp.path,argv[2],argv[3]);return 0;}check(argc==2&&std::string(argv[1])=="--self-test","usage: --self-test or --baseline-aliased-cli OLD-COMPACT OLD-MIGRATE");independent(temp.path);
 refused(temp.path,1,"owned-copy-external-hardlink-refusal",[](Fixture& f){fs::remove(f.copy/"CURRENT");fs::create_hard_link(f.source/"CURRENT",f.copy/"CURRENT");});
 refused(temp.path,2,"owned-copy-internal-hardlink-refusal",[](Fixture& f){fs::create_hard_link(f.copy/"CURRENT",f.copy/"second-current");});
 refused(temp.path,3,"owned-copy-symlink-refusal",[](Fixture& f){fs::create_symlink(f.source/"CURRENT",f.copy/"unexpected-link");});
 refused(temp.path,4,"owned-copy-special-file-refusal",[](Fixture& f){check(::mkfifo((f.copy/"unexpected-fifo").c_str(),0600)==0,"owned FIFO fixture");});
 refused(temp.path,5,"owned-copy-directory-current-refusal",[](Fixture& f){fs::remove(f.copy/"CURRENT");private_dir(f.copy/"CURRENT");});
 refused(temp.path,6,"owned-copy-descendant-hardlink-refusal",[](Fixture& f){private_dir(f.copy/"nested");file(f.root/"original/public-leaf","public synthetic nested alias");fs::create_hard_link(f.root/"original/public-leaf",f.copy/"nested/public-leaf");});
 refused(temp.path,7,"owned-copy-lock-hardlink-refusal",[](Fixture& f){check(fs::is_regular_file(f.copy/"LOCK"),"real RocksDB LOCK present");fs::create_hard_link(f.copy/"LOCK",f.root/"external-lock");});
 marker_aliases(temp.path);
 emit("owned-copy-physical-admission-summary",{{"scenarioRows",9},{"actualCliInvocations",calls},{"unsafeCliRefusals",18},{"successfulCliInvocations",3},{"oldTestsReplayed",false},{"liveWriterOrSameUidAdversaryQualified",false},{"sourceCopyIndependenceQualified",true},{"runtimeStarted",false},{"networkUsed",false}});return 0;
}catch(const std::exception& e){std::cout<<Json{{"diagnostic","owned-copy-physical-admission-refused"},{"reason",e.what()},{"actualCliInvocations",calls},{"newSignaturesCreated",0},{"privateInputsRead",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
