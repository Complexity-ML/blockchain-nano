#include "nano/beacon_journal_cli.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_bls.hpp"
#include <charconv>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <set>
namespace nano {
namespace {
namespace fs=std::filesystem;
void require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
struct File {
 int fd;
 explicit File(int value):fd(value){if(fd<0)throw std::runtime_error("Cannot open beacon journal artifact");}
 ~File(){::close(fd);}
 File(const File&)=delete;File& operator=(const File&)=delete;
};
void sync(int fd){int result;do{result=::fsync(fd);}while(result<0&&errno==EINTR);if(result<0)throw std::runtime_error("Cannot synchronize beacon journal artifact");}
std::string read_file(const fs::path& path,size_t bound){
 File input(::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));struct stat info{};
 require(::fstat(input.fd,&info)==0&&S_ISREG(info.st_mode),"Journal inputs must be regular files, not links/devices");
 require(info.st_size>=0&&uintmax_t(info.st_size)<=bound,"Journal input exceeds configured byte limit");
 std::string text;char buffer[4096];
 for(;;){auto n=::read(input.fd,buffer,sizeof(buffer));if(n<0&&errno==EINTR)continue;if(n<0)throw std::runtime_error("Cannot read beacon journal artifact");if(!n)break;require(size_t(n)<=bound-text.size(),"Journal input grew beyond configured byte limit");text.append(buffer,size_t(n));}
 return text;
}
Json parse(const std::string& text){
 std::vector<std::set<std::string>> keys;
 return Json::parse(text,[&](int depth,Json::parse_event_t event,Json& value){
  require(depth<=16,"Journal JSON nesting limit");
  if(event==Json::parse_event_t::object_start)keys.emplace_back();
  else if(event==Json::parse_event_t::object_end)keys.pop_back();
  else if(event==Json::parse_event_t::key)require(keys.back().insert(value.get<std::string>()).second,"Duplicate journal JSON key");return true;
 });
}
Bytes fixed(const Json& value,size_t size){require(value.is_string(),"Expected hexadecimal string");const auto text=value.get<std::string>();require(text.size()==2+size*2,"Invalid journal identity length");auto bytes=unhex(text);require(bytes.size()==size&&hex(bytes)==text,"Journal identities must be canonical hexadecimal");return bytes;}
uint64_t integer(const Json& value){const auto n=u64(quantity(value));require(value==quantity(Int(std::to_string(n))),"Expected canonical hexadecimal quantity");return n;}
Json token_json(const ForkCheckpointToken& token){return {{"sequence",quantity(Int(std::to_string(token.sequence)))},{"digest",hex(token.digest)}};}
// Each operation uses a new directory. A complete marker is written last;
// artifacts from an interrupted operation are retained for explicit recovery.
struct Output {
 fs::path path;File directory;
 static int create(const fs::path& path){
  const auto absolute=fs::absolute(path).lexically_normal();const auto name=absolute.filename().string();require(!name.empty()&&name!="."&&name!="..","Invalid journal output directory");
  const auto parent_path=fs::canonical(absolute.parent_path());
  File parent(::open(parent_path.c_str(),O_RDONLY|O_CLOEXEC|O_DIRECTORY|O_NOFOLLOW));
  if(::mkdirat(parent.fd,name.c_str(),0700)<0)throw std::runtime_error("Journal output directory must be new; refusing overwrite");
  sync(parent.fd);return ::openat(parent.fd,name.c_str(),O_RDONLY|O_CLOEXEC|O_DIRECTORY|O_NOFOLLOW);
 }
 explicit Output(const fs::path& target):path(fs::absolute(target).lexically_normal()),directory(create(path)){}
 void write(const char* name,const std::string& value){
  File output(::openat(directory.fd,name,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600));size_t offset=0;
  while(offset<value.size()){auto n=::write(output.fd,value.data()+offset,value.size()-offset);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("Cannot write beacon journal artifact");offset+=size_t(n);}
  sync(output.fd);sync(directory.fd);
 }
};
}
int beacon_journal_command(int argc,char** argv){
 require(argc>=3,"beacon-journal requires init/status/import/export; use --help");const std::string operation=argv[2];
 const bool init=operation=="init",status=operation=="status",import=operation=="import",exporting=operation=="export";
 require((init&&argc==6)||(status&&argc==6)||(import&&argc==9)||(exporting&&argc==7),"Invalid beacon-journal arguments; use --help");
 const auto identity=parse(read_file(argv[3],4096));require(identity.is_object()&&identity.size()==5,"Expected exactly five beacon identity fields");
 const auto chain=quantity(identity.at("chainId"));require(chain>0&&identity.at("chainId")==quantity(chain),"Invalid chain ID");
 const auto execution=fixed(identity.at("executionGenesis"),32),genesis=fixed(identity.at("genesisValidatorsRoot"),32),key=fixed(identity.at("publicKey"),48);
 const auto slots=integer(identity.at("slotsPerEpoch"));require(slots>0&&aggregate_consensus_bls_keys(std::vector<Bytes>{key}).has_value(),"Invalid slot configuration or BLS public key");
 ConsensusRoot validators{};std::copy(genesis.begin(),genesis.end(),validators.begin());
 std::optional<ForkCheckpointToken> expected;
 if(!init){const auto token=parse(read_file(argv[5],4096));require(token.is_object()&&token.size()==2,"Invalid expected commitment");expected=ForkCheckpointToken{integer(token.at("sequence")),fixed(token.at("digest"),32)};require(expected->sequence>0,"Invalid expected sequence");}
 std::string history;size_t import_bound=0;
 if(import){const std::string limit=argv[7];const auto parsed=std::from_chars(limit.data(),limit.data()+limit.size(),import_bound);require(!limit.empty()&&parsed.ec==std::errc{}&&parsed.ptr==limit.data()+limit.size()&&import_bound>0,"Import byte limit must be a positive decimal integer");history=read_file(argv[6],import_bound);}
 const fs::path database=fs::absolute(argv[4]).lexically_normal();
 // Never let a read/import/export invocation silently initialize a missing DB.
 if(init)require(!fs::exists(fs::symlink_status(database)),"Journal initialization requires a new database directory");
 else require(!fs::is_symlink(fs::symlink_status(database))&&fs::is_regular_file(database/"CURRENT"),"An existing dedicated journal database is required");
 std::unique_ptr<Output> output;
 if(!status){
  const fs::path target=fs::absolute(argv[init?5:import?8:6]).lexically_normal();
  const auto db_canonical=fs::weakly_canonical(database),out_canonical=fs::weakly_canonical(target);
  auto relative=out_canonical.lexically_relative(db_canonical);
  require(!relative.empty()&&*relative.begin()=="..","Journal receipt directory must be outside the database");
  output=std::make_unique<Output>(target);
  output->write("intent.json",Json{{"operation",operation},{"identity",identity},{"expectedCommitment",expected?token_json(*expected):Json(nullptr)}}.dump(2)+"\n");
 }
 if(init){if(::mkdir(database.c_str(),0700)<0)throw std::runtime_error("Journal initialization requires a new database directory");File parent(::open(database.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));sync(parent.fd);}
 ChainStore disk(database);BeaconSigningJournal journal(disk,chain,execution,validators,key,slots,expected);
 size_t matches=0;std::string interchange;
 if(import){matches=journal.import_interchange(history,import_bound);require(matches>0,"Interchange contains no history entry for this validator");}
 if(exporting)interchange=journal.retire_and_export_interchange();
 const auto commitment=token_json(journal.commitment());Json result{{"operation",operation},{"publicKey",hex(key)},{"commitment",commitment},{"retired",journal.is_retired()}};
 if(import)result["matchedEntries"]=matches;
 if(output){
  // Preserve the new commitment before writing the interchange, so a failed
  // interchange write can be recovered by re-exporting with this commitment.
  output->write("commitment.json",commitment.dump(2)+"\n");
  if(exporting){interchange+="\n";output->write("interchange.json",interchange);result["interchangeSha256"]=hex(sha256(Bytes(interchange.begin(),interchange.end())));}
  result["outputDirectory"]=output->path.string();output->write("complete.json",result.dump(2)+"\n");
 }
 std::cout<<result.dump()<<std::endl;return 0;
}
}
