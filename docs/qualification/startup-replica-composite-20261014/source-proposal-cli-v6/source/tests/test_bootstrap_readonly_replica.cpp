#include "bootstrap-readonly-replica.hpp"
#include "rocksdb-owned-copy.hpp"
#include "nano/types.hpp"
#include <rocksdb/db.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <sys/resource.h>
#include <string_view>
#include <type_traits>
using nano::Json;
namespace fs=std::filesystem;
using nano::maintenance::SealedReadonlyReplica;
namespace {
const auto began=std::chrono::steady_clock::now();
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint64_t elapsed(){return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count());}
uint64_t rss(){rusage r{};require(::getrusage(RUSAGE_SELF,&r)==0,"resource observation");
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void checkpoint(){require(elapsed()<60000&&rss()<=131072,"replica test resource budget");}
struct Temp{
 fs::path path;
 Temp(){auto name=(fs::temp_directory_path()/"nano-readonly-replica-XXXXXX").string();require(::mkdtemp(name.data())!=nullptr,"owned test directory");path=fs::canonical(name);}
 ~Temp(){std::error_code ignored;fs::remove_all(path,ignored);}
};
void directory(const fs::path& path){require(::mkdir(path.c_str(),0700)==0,"exclusive owned directory");}
void emit(const char* scenario,Json fields){fields["scenario"]=scenario;fields["result"]="PASS";fields["newSignaturesCreated"]=0;fields["privateInputsRead"]=false;fields["productionQualified"]=false;std::cout<<fields.dump()<<std::endl;}
using Rows=std::map<std::string,std::string>;
Rows read_rows(const SealedReadonlyReplica& replica){
 const auto path=replica.read_only_path();rocksdb::Options options;options.create_if_missing=false;options.paranoid_checks=true;auto db=nano::maintenance::open_read_only_copy(options,path.string());rocksdb::ReadOptions read;read.verify_checksums=true;read.fill_cache=false;std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(read));Rows rows;
 for(it->SeekToFirst();it->Valid();it->Next()){checkpoint();require(rows.size()<16,"small public logical fixture");rows.emplace(it->key().ToString(),it->value().ToString());}
 require(it->status().ok(),"complete RO traversal");it.reset();db.reset();replica.recheck();return rows;
}
static_assert(!std::is_invocable_v<decltype(&read_rows),const fs::path&>);
static_assert(!std::is_constructible_v<SealedReadonlyReplica,const fs::path&>);
static_assert(!std::is_reference_v<decltype(std::declval<const SealedReadonlyReplica&>().read_only_path())>);
bool overlap(const fs::path& a,const fs::path& b){auto i=a.begin(),j=b.begin();for(;i!=a.end()&&j!=b.end()&&*i==*j;++i,++j){}return i==a.end();}
size_t scratch_count(const fs::path& out){size_t count=0;for(const auto& entry:fs::directory_iterator(out))if(entry.path().filename().string().starts_with("readonly-replica-"))++count;return count;}
void save(const fs::path& path,std::string_view value){const int fd=::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);require(fd>=0,"exclusive owned test file");size_t at=0;while(at<value.size()){const auto n=::write(fd,value.data()+at,value.size()-at);if(n<0&&errno==EINTR)continue;require(n>0,"test file write");at+=size_t(n);}require(::fsync(fd)==0&&::close(fd)==0,"durable test file");}
void change(const fs::path& path){const int fd=::open(path.c_str(),O_WRONLY|O_NOFOLLOW|O_CLOEXEC);require(fd>=0,"owned mutation target");require(::pwrite(fd,"Z",1,0)==1&&::fsync(fd)==0&&::close(fd)==0,"real owned content mutation");}
void conformance(const fs::path& root){
 const auto source=root/"owned-rw-copy",out=root/"audit-output";directory(source);directory(out);save(out/"unrelated-sentinel","unchanged");rocksdb::Options options;options.create_if_missing=true;options.paranoid_checks=true;options.max_background_jobs=1;auto writer=nano::maintenance::open_copy(options,source.string());rocksdb::WriteOptions write;write.sync=true;require(writer->Put(write,"public-key","public-value").ok(),"genuine owned DB fixture");
 // The writer stays open, without concurrent writes. RO opens only the new
 // independent replica. This does not certify quiescence of a live writer.
 auto replica=SealedReadonlyReplica::capture(source,out,1024,16*1024*1024,checkpoint);const auto path=replica.read_only_path();require(!overlap(source,path)&&!overlap(path,source)&&path.parent_path()==out,"RO path is distinct private replica");struct stat a{},b{};require(::lstat((source/"CURRENT").c_str(),&a)==0&&::lstat((path/"CURRENT").c_str(),&b)==0,"actual CURRENT identities");require((a.st_dev!=b.st_dev||a.st_ino!=b.st_ino)&&b.st_nlink==1&&(b.st_mode&0777)==0600,"no shared inode/private replica file");require(read_rows(replica)==Rows{{"public-key","public-value"}},"same genuine logical content");replica.close();require(!fs::exists(path)&&scratch_count(out)==0&&fs::is_regular_file(out/"unrelated-sentinel"),"explicit owned cleanup preserves unrelated output");
 auto caller_output=out;bool changed_output=false;uint64_t owned_callback_calls=0;std::function<void()> change_caller;change_caller=[&]{++owned_callback_calls;checkpoint();if(!changed_output){caller_output=source;changed_output=true;change_caller={};}};
 auto rebound=SealedReadonlyReplica::capture(source,caller_output,1024,16*1024*1024,change_caller);const auto rebound_path=rebound.read_only_path();require(changed_output&&!change_caller&&owned_callback_calls>1&&caller_output==source&&rebound_path.parent_path()==out&&!overlap(source,rebound_path),"callback-mutated caller output cannot redirect the owned replica path");require(read_rows(rebound)==Rows{{"public-key","public-value"}},"mutated caller path retains original replica authority");rebound.close();require(!fs::exists(rebound_path)&&scratch_count(out)==0,"caller mutation cleanup bound to original private output");writer.reset();
 emit("maintenance-readonly-replica-api-conformance",{{"rawPathApiRejectedAtCompileTime",true},{"actualWriterRemainedOpen",true},{"readOnlyOpenedIndependentReplicaOnly",true},{"sameLogicalContent",true},{"callerOutputMutationRemainsBoundToOwnedParent",true},{"clearedCallerCallbackDoesNotDisableOwnedChecks",true},{"sharedInodeUsed",false},{"explicitCleanupObserved",true},{"concurrentWritingQualified",false}});
}
void mutations(const fs::path& root){
 const auto source=root/"mutation-source",out=root/"mutation-output";directory(source);directory(out);save(source/"payload",std::string(131073,'A'));save(out/"unrelated-sentinel","unchanged");bool changed=false,returned=false,rejected=false;
 const std::function<void()> mutate=[&]{checkpoint();if(changed)return;for(const auto& entry:fs::directory_iterator(out))if(entry.path().filename().string().starts_with("readonly-replica-")&&fs::exists(entry.path()/"payload")){change(source/"payload");changed=true;break;}};
 try{auto replica=SealedReadonlyReplica::capture(source,out,32,1024*1024,mutate);returned=true;replica.close();}catch(const std::invalid_argument&){rejected=true;}
 require(changed&&rejected&&!returned&&scratch_count(out)==0,"source mutation refused before a sealed RO capability exists");
 // A copied path is returned by value; changing its file cannot mutate the
 // sealed certificate. Rechecking it must refuse rather than ignore changes.
 auto replica=SealedReadonlyReplica::capture(source,out,32,1024*1024,checkpoint);const auto path=replica.read_only_path();change(path/"payload");rejected=false;try{replica.recheck();}catch(const std::invalid_argument&){rejected=true;}require(rejected,"replica content mutation refused");replica.close();require(!fs::exists(path)&&scratch_count(out)==0&&fs::is_regular_file(out/"unrelated-sentinel"),"mutation cleanup only owned scratch");
 bool armed=false,rebound_parent=false;const auto renamed=root/"renamed-owned-output";const std::function<void()> rebind=[&]{checkpoint();if(armed&&!rebound_parent){require(::rename(out.c_str(),renamed.c_str())==0,"actual owned output parent rename");directory(out);save(out/"substituted-parent-sentinel","preserved");rebound_parent=true;}};
 auto substitutable=SealedReadonlyReplica::capture(source,out,32,1024*1024,rebind);const auto original_path=substitutable.read_only_path();armed=true;bool path_returned=false;rejected=false;try{(void)substitutable.read_only_path();path_returned=true;}catch(const std::invalid_argument&){rejected=true;}
 require(rebound_parent&&rejected&&!path_returned&&fs::is_regular_file(out/"substituted-parent-sentinel")&&fs::exists(renamed/original_path.filename()),"callback parent substitution must refuse before a pathname is returned");bool cleanup_refused=false;try{substitutable.close();}catch(const std::invalid_argument&){cleanup_refused=true;}require(cleanup_refused&&fs::is_regular_file(out/"substituted-parent-sentinel")&&fs::exists(renamed/original_path.filename()),"incomplete cleanup refuses and never deletes substituted parent");
 emit("maintenance-readonly-replica-content-mutation-refusals",{{"sourceMutationBeforeSealRefused",true},{"mutatedReplicaReadbackRefused",true},{"parentRenameSubstitutionDuringRecheckRefused",true},{"substitutedParentNeverDeleted",true},{"cleanupIncompleteRefused",true},{"databaseReadOnlyOpensInThisGroup",0},{"protocolWritesInThisGroup",0},{"successfulCleanupObservedBeforeSubstitution",true}});
}
}
int main(int argc,char** argv){try{require(argc==2&&std::string(argv[1])=="--self-test","usage --self-test");Temp temp;conformance(temp.path);mutations(temp.path);checkpoint();emit("bootstrap-readonly-replica-self-test",{{"groups",2},{"elapsedMs",elapsed()},{"maxRssKiB",rss()},{"runtimeStarted",false},{"networkUsed",false},{"physicalPowerLossQualified",false}});return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
