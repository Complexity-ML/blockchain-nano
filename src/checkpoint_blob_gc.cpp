#include "nano/checkpoint_blob_gc.hpp"
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <openssl/evp.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <set>
#include <tuple>
namespace nano {namespace {
constexpr std::string_view blob_prefix="checkpoint-blob/v1/";
void require(bool ok,const char* why){if(!ok)throw std::invalid_argument(why);}
void status(const rocksdb::Status& s){if(!s.ok())throw std::runtime_error("checkpoint GC: "+s.ToString());}
void shape(const Json& j,std::initializer_list<const char*> keys){require(j.is_object()&&j.size()==keys.size(),"checkpoint GC unknown/missing fields");for(auto k:keys)require(j.contains(k),"checkpoint GC missing field");}
uint64_t number(const Json& j){require(j.is_string(),"checkpoint GC canonical quantity required");auto n=u64(quantity(j));require(n&&quantity(Int(std::to_string(n)))==j.get<std::string>(),"checkpoint GC positive canonical quantity required");return n;}
size_t size(const Json& j){auto n=number(j);require(n<=SIZE_MAX,"checkpoint GC size overflow");return static_cast<size_t>(n);}
std::string sha_text(const Json& j){require(j.is_string(),"checkpoint GC SHA required");auto text=j.get<std::string>();require(text.size()==64&&text.find_first_not_of("0123456789abcdef")==std::string::npos,"checkpoint GC canonical SHA required");return text;}
Bytes root(const Json& j){require(j.is_string(),"checkpoint GC root required");const auto text=j.get<std::string>();auto bytes=unhex(text);require(bytes.size()==32&&hex(bytes)==text,"checkpoint GC canonical root required");return bytes;}
ForkCheckpointToken token(const Json& j){shape(j,{"sequence","digest"});return {number(j.at("sequence")),root(j.at("digest"))};}
Json token_json(const ForkCheckpointToken& t){return {{"sequence",quantity(Int(std::to_string(t.sequence)))},{"digest",hex(t.digest)}};}
std::string hash_text(std::string_view bytes){return hex(sha256(Bytes(bytes.begin(),bytes.end()))).substr(2);}
void add(uint64_t& value,uint64_t n){if(n>UINT64_MAX-value)throw std::overflow_error("checkpoint GC accounting overflow");value+=n;}
class Hash {
 std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> p_{EVP_MD_CTX_new(),EVP_MD_CTX_free};
public:
 Hash(){if(!p_||EVP_DigestInit_ex(p_.get(),EVP_sha256(),nullptr)!=1)throw std::runtime_error("checkpoint GC digest init");}
 void raw(const void* bytes,size_t n){if(EVP_DigestUpdate(p_.get(),bytes,n)!=1)throw std::runtime_error("checkpoint GC digest update");}
 void frame(std::string_view bytes){uint8_t length[8];uint64_t n=bytes.size();for(unsigned i=0;i<8;++i)length[7-i]=uint8_t(n>>(i*8));raw(length,8);raw(bytes.data(),bytes.size());}
 std::string finish(){uint8_t bytes[32];unsigned n=0;if(EVP_DigestFinal_ex(p_.get(),bytes,&n)!=1||n!=32)throw std::runtime_error("checkpoint GC digest finish");return hex(Bytes(bytes,bytes+32)).substr(2);}
};
struct FsIdentity {dev_t device;ino_t inode;bool operator==(const FsIdentity&)const=default;};
FsIdentity directory(const std::filesystem::path& path){
 for(auto at=path;!at.empty();at=at.parent_path()){require(!std::filesystem::is_symlink(at),"checkpoint GC symlink path refused");if(at==at.parent_path())break;}
 struct stat st{};require(::lstat(path.c_str(),&st)==0&&S_ISDIR(st.st_mode)&&st.st_uid==::geteuid()&&(st.st_mode&077)==0,"checkpoint GC owned private directory required");return {st.st_dev,st.st_ino};
}
std::filesystem::path copy_path(const Json& j){require(j.is_string(),"checkpoint GC copy path required");std::filesystem::path p(j.get<std::string>());require(p.is_absolute()&&p==p.lexically_normal(),"checkpoint GC canonical absolute copy path required");directory(p);require(std::filesystem::canonical(p)==p,"checkpoint GC alias path refused");return p;}
bool inside(const std::filesystem::path& a,const std::filesystem::path& b){auto i=a.begin(),j=b.begin();for(;i!=a.end()&&j!=b.end()&&*i==*j;++i,++j){}return i==a.end();}
void marker(const std::filesystem::path& path){
 directory(path.parent_path());const auto file=path.parent_path()/".nano-owned-checkpoint-gc-copy";struct stat st{};
 require(::lstat(file.c_str(),&st)==0&&S_ISREG(st.st_mode)&&st.st_uid==::geteuid()&&st.st_nlink==1&&st.st_size>0&&st.st_size<=64,"checkpoint GC explicit owned-copy marker required");
 std::ifstream f(file,std::ios::binary);char buffer[65];f.read(buffer,sizeof(buffer));std::string text(buffer,size_t(f.gcount()));
 require(f.eof()&&!f.bad()&&text.size()<=64&&(text=="owned-offline-checkpoint-gc-v1"||text=="owned-offline-checkpoint-gc-v1\n"),"checkpoint GC marker differs");
}
using Check=std::function<void()>;
Check deadline(uint64_t ms,Check outer){
 require(ms&&ms<=uint64_t(INT64_MAX),"checkpoint GC duration bound invalid");
 const auto start=std::chrono::steady_clock::now();
 return [start,ms,outer=std::move(outer)]{const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();if(elapsed<0||static_cast<uint64_t>(elapsed)>=ms)throw std::runtime_error("checkpoint GC deadline");if(outer)outer();};
}
struct Scan {std::map<Bytes,uint64_t> blobs;std::string non_blobs;};
Scan scan(rocksdb::DB& db,const CheckpointBlobGcBudget& budget,size_t maximum_blob,const Check& check){
 Scan out;Hash non;uint64_t records=0,bytes=0;rocksdb::ReadOptions ro;ro.verify_checksums=true;ro.fill_cache=false;
 std::unique_ptr<rocksdb::Iterator> it(db.NewIterator(ro));
 for(it->SeekToFirst();it->Valid();it->Next()){
  if(check)check();add(records,1);add(bytes,it->key().size());add(bytes,it->value().size());require(records<=budget.scan_records&&bytes<=budget.scan_bytes,"checkpoint GC scan budget exhausted before complete plan");
  const auto key=it->key().ToString();const auto value=std::string_view(it->value().data(),it->value().size());
  require(!key.starts_with("checkpoint-blob/")||key.starts_with(blob_prefix),"checkpoint GC unknown blob namespace");
  if(key.starts_with(blob_prefix)){
   const auto text=key.substr(blob_prefix.size());const auto digest=root(Json(text));
   require(!value.empty()&&value.size()<=maximum_blob&&hash_text(value)==text.substr(2),"checkpoint GC corrupt/oversized blob");require(out.blobs.emplace(digest,value.size()).second,"checkpoint GC duplicate blob key");
  }else{non.frame(key);non.frame(value);}
 }
 status(it->status());out.non_blobs=non.finish();return out;
}
std::vector<CheckpointArchiveEntry> archives(const Json& j,size_t maximum){
 require(j.is_array()&&j.size()<=1,"checkpoint GC recovery archive bounds");std::vector<CheckpointArchiveEntry> out;
 for(const auto& row:j){require(row.is_array()&&row.size()==3,"checkpoint GC recovery archive shape");require(row[1].is_string(),"checkpoint GC archive slot required");const auto slot=u64(quantity(row[1]));require(quantity(Int(std::to_string(slot)))==row[1].get<std::string>(),"checkpoint GC archive slot encoding");const auto bytes=unhex(row[2].get<std::string>());require(!bytes.empty()&&bytes.size()<=maximum&&hex(bytes)==row[2].get<std::string>(),"checkpoint GC archive bytes");out.push_back({root(row[0]),slot,bytes});}return out;
}
void validate_budget(const CheckpointBlobGcBudget& b){require(b.inventory_files&&b.inventory_bytes&&b.scan_records&&b.scan_bytes&&b.delete_records&&b.delete_bytes&&b.batch_bytes&&b.duration_ms,"checkpoint GC positive finite budgets required");require(b.duration_ms<=uint64_t(INT64_MAX),"checkpoint GC duration overflow");}
struct Busy {bool& b;explicit Busy(bool& busy):b(busy){require(!b,"checkpoint GC reentrant operation");b=true;}~Busy(){b=false;}};
}
CheckpointBlobGcBudget checkpoint_blob_gc_budget(const Json& j){require(j.is_array()&&j.size()==8,"checkpoint GC budget must contain eight quantities");CheckpointBlobGcBudget b{number(j[0]),number(j[1]),number(j[2]),number(j[3]),number(j[4]),number(j[5]),number(j[6]),number(j[7])};validate_budget(b);return b;}
CapellaStoreSnapshotLimits checkpoint_blob_gc_snapshot_limits(const Json& j){require(j.is_array()&&j.size()==8,"checkpoint GC snapshot limits must contain eight quantities");return {size(j[0]),size(j[1]),size(j[2]),size(j[3]),size(j[4]),size(j[5]),number(j[6]),number(j[7])};}
std::string checkpoint_blob_gc_copy_inventory(const std::filesystem::path& path,uint64_t maximum_files,uint64_t maximum_bytes,const std::function<void()>& checkpoint){
 require(maximum_files&&maximum_bytes,"checkpoint GC inventory budget required");directory(path);std::vector<std::filesystem::path> files;uint64_t count=0,total=0;
 for(const auto& item:std::filesystem::recursive_directory_iterator(path)){
  if(checkpoint)checkpoint();struct stat st{};require(::lstat(item.path().c_str(),&st)==0&&st.st_uid==::geteuid()&&(S_ISREG(st.st_mode)||S_ISDIR(st.st_mode)),"checkpoint GC source links/special files/owner refused");
  if(S_ISREG(st.st_mode)){require(st.st_nlink==1&&st.st_size>=0,"checkpoint GC shared hardlink refused");add(count,1);add(total,uint64_t(st.st_size));require(count<=maximum_files&&total<=maximum_bytes,"checkpoint GC inventory budget");files.push_back(item.path());}
 }
 std::sort(files.begin(),files.end());Hash whole;
 for(const auto& file:files){
  if(checkpoint)checkpoint();const int fd=::open(file.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if(fd<0)throw std::runtime_error("checkpoint GC inventory file open");
  struct Fd {int fd;~Fd(){::close(fd);}} owned{fd};struct stat before{},after{};require(::fstat(fd,&before)==0&&S_ISREG(before.st_mode)&&before.st_nlink==1&&before.st_uid==::geteuid(),"checkpoint GC inventory file identity");
  Hash content;uint8_t chunk[65536];uint64_t bytes=0;for(;;){if(checkpoint)checkpoint();const auto n=::read(fd,chunk,sizeof(chunk));if(n<0&&errno==EINTR)continue;if(n<0)throw std::runtime_error("checkpoint GC inventory read");if(!n)break;add(bytes,uint64_t(n));require(bytes<=uint64_t(before.st_size)&&bytes<=maximum_bytes,"checkpoint GC inventory file grew");content.raw(chunk,size_t(n));}
  if(checkpoint)checkpoint();require(::fstat(fd,&after)==0&&bytes==uint64_t(before.st_size)&&before.st_dev==after.st_dev&&before.st_ino==after.st_ino&&before.st_size==after.st_size&&before.st_mtime==after.st_mtime&&before.st_ctime==after.st_ctime,"checkpoint GC inventory changed while reading");
  whole.frame(std::filesystem::relative(file,path).generic_string());whole.frame(std::to_string(bytes));whole.frame(content.finish());
 }
 return whole.finish();
}
Json AuthenticatedCheckpointBlobClosure::description()const{Json refs=Json::array();for(const auto& [id,n]:references_)refs.push_back(Json::array({hex(id),quantity(Int(std::to_string(n)))}));return {{"identity",identity_},{"executionHead",execution_head_},{"references",refs}};}
Json CheckpointBlobGcPlan::description()const{
 auto rows=[](const auto& items){Json result=Json::array();for(const auto& row:items)result.push_back(Json::array({row.key,hex(row.digest),quantity(Int(std::to_string(row.bytes)))}));return result;};Json refs=Json::array();for(const auto& [id,n]:protected_)refs.push_back(Json::array({hex(id),quantity(Int(std::to_string(n)))}));
 return {{"format","nano-owned-checkpoint-blob-gc-plan-v1"},{"planId",id_},{"identity",identity_},{"primary",token_json(primary_token_)},{"witness",token_json(witness_token_)},{"next",token_json(next_token_)},{"protected",refs},{"deletePrimary",rows(primary_)},{"deleteWitness",rows(witness_)},{"nonBlobPrimarySha256",non_blob_primary_},{"nonBlobWitnessSha256",non_blob_witness_},{"operatorOwnedCopiesOnly",true},{"productionQualified",false}};
}
struct CheckpointBlobGcCopies::Impl {
 Json request;Bytes request_sha,stop_sha;std::filesystem::path primary_path,witness_path,primary_source,witness_source;
 std::string primary_source_inventory,witness_source_inventory;FsIdentity primary_source_id,witness_source_id;
 FsIdentity primary_id,witness_id;std::unique_ptr<ChainStore> primary,witness;
 std::recursive_mutex mutex;bool busy=false,unavailable=false,finished=false;
};
CheckpointBlobGcCopies::CheckpointBlobGcCopies(std::unique_ptr<Impl> p):impl_(std::move(p)){}
CheckpointBlobGcCopies::~CheckpointBlobGcCopies()=default;
std::unique_ptr<CheckpointBlobGcCopies> CheckpointBlobGcCopies::open(std::string_view bytes,const Bytes& expected,
 std::string_view stopped,const Bytes& stopped_expected,const CheckpointBlobGcBudget& budget,const std::function<void()>& outer){
 validate_budget(budget);const auto check=deadline(budget.duration_ms,outer);check();
 require(!bytes.empty()&&bytes.size()<=65536&&expected.size()==32&&sha256(Bytes(bytes.begin(),bytes.end()))==expected,"checkpoint GC request pin mismatch");
 require(!stopped.empty()&&stopped.size()<=65536&&stopped_expected.size()==32&&sha256(Bytes(stopped.begin(),stopped.end()))==stopped_expected,"checkpoint GC stop receipt pin mismatch");
 auto p=std::make_unique<Impl>();p->request=Json::parse(bytes);p->request_sha=expected;p->stop_sha=stopped_expected;const auto& j=p->request;
 shape(j,{"format","chainId","genesisHash","profileSha256","primary","witness","next","stopReceiptSha256","limits","budget"});
 require(j.at("format")=="nano-owned-stopped-checkpoint-gc-v1"&&j.dump()==bytes,"checkpoint GC request format/canonicality");
 require(j.at("chainId").is_string()&&quantity(j.at("chainId"))>0&&quantity(j.at("chainId"))<(Int(1)<<256)&&quantity(quantity(j.at("chainId")))==j.at("chainId").get<std::string>(),"checkpoint GC chain ID");root(j.at("genesisHash"));sha_text(j.at("profileSha256"));
 require(sha_text(j.at("stopReceiptSha256"))==hex(stopped_expected).substr(2),"checkpoint GC stop receipt binding");(void)checkpoint_blob_gc_snapshot_limits(j.at("limits"));
 const auto requested_budget=checkpoint_blob_gc_budget(j.at("budget"));
 require(requested_budget.inventory_files==budget.inventory_files&&requested_budget.inventory_bytes==budget.inventory_bytes&&requested_budget.scan_records==budget.scan_records&&requested_budget.scan_bytes==budget.scan_bytes&&requested_budget.delete_records==budget.delete_records&&requested_budget.delete_bytes==budget.delete_bytes&&requested_budget.batch_bytes==budget.batch_bytes&&requested_budget.duration_ms==budget.duration_ms,"checkpoint GC budget request mismatch");
 for(const auto* name:{"primary","witness"}){shape(j.at(name),{"path","inventorySha256","token","sourcePath","sourceInventorySha256"});sha_text(j.at(name).at("inventorySha256"));sha_text(j.at(name).at("sourceInventorySha256"));(void)token(j.at(name).at("token"));}(void)token(j.at("next"));
 p->primary_path=copy_path(j.at("primary").at("path"));p->witness_path=copy_path(j.at("witness").at("path"));p->primary_id=directory(p->primary_path);p->witness_id=directory(p->witness_path);
 require(p->primary_id!=p->witness_id&&!inside(p->primary_path,p->witness_path)&&!inside(p->witness_path,p->primary_path),"checkpoint GC primary/witness copies alias or overlap");marker(p->primary_path);marker(p->witness_path);
 p->primary_source=copy_path(j.at("primary").at("sourcePath"));p->witness_source=copy_path(j.at("witness").at("sourcePath"));p->primary_source_id=directory(p->primary_source);p->witness_source_id=directory(p->witness_source);
 const std::array<std::filesystem::path,4> paths{p->primary_source,p->witness_source,p->primary_path,p->witness_path};
 for(size_t a=0;a<paths.size();++a)for(size_t b=a+1;b<paths.size();++b)require(directory(paths[a])!=directory(paths[b])&&!inside(paths[a],paths[b])&&!inside(paths[b],paths[a]),"checkpoint GC originals/copies share identity or ancestry");
 p->primary_source_inventory=checkpoint_blob_gc_copy_inventory(p->primary_source,budget.inventory_files,budget.inventory_bytes,check);p->witness_source_inventory=checkpoint_blob_gc_copy_inventory(p->witness_source,budget.inventory_files,budget.inventory_bytes,check);
 require(p->primary_source_inventory==sha_text(j.at("primary").at("sourceInventorySha256"))&&p->witness_source_inventory==sha_text(j.at("witness").at("sourceInventorySha256")),"checkpoint GC original source inventory mismatch");

 const auto receipt=Json::parse(stopped);shape(receipt,{"format","scope","allWritersStopped","allReadersStopped","stoppedAtMs","copies"});
 require(receipt.at("format")=="nano-stopped-checkpoint-copy-receipt-v1"&&receipt.at("scope")=="isolated-owned-copies"&&receipt.dump()==stopped&&receipt.at("allWritersStopped").is_boolean()&&receipt.at("allWritersStopped")==true&&receipt.at("allReadersStopped").is_boolean()&&receipt.at("allReadersStopped")==true,"checkpoint GC externally pinned stopped-copy receipt required");(void)number(receipt.at("stoppedAtMs"));
 require(receipt.at("copies").is_array()&&receipt.at("copies").size()==2,"checkpoint GC copy receipt shape");unsigned at=0;
 for(const auto* name:{"primary","witness"}){const auto& copy=receipt.at("copies")[at++];shape(copy,{"role","path","inventorySha256","sourcePath","sourceInventorySha256"});require(copy.at("role")==name&&copy.at("path")==j.at(name).at("path")&&copy.at("inventorySha256")==j.at(name).at("inventorySha256")&&copy.at("sourcePath")==j.at(name).at("sourcePath")&&copy.at("sourceInventorySha256")==j.at(name).at("sourceInventorySha256"),"checkpoint GC copy receipt binding mismatch");check();const auto path=name==std::string_view("primary")?p->primary_path:p->witness_path;require(std::filesystem::is_regular_file(path/"CURRENT"),"checkpoint GC existing store required");require(checkpoint_blob_gc_copy_inventory(path,budget.inventory_files,budget.inventory_bytes,check)==sha_text(copy.at("inventorySha256")),"checkpoint GC closed copy inventory mismatch");require(copy.at("inventorySha256")==copy.at("sourceInventorySha256"),"checkpoint GC copy differs from independently pinned stopped original");}
 // The SAME exclusive handles validate existing schemas. Creation and schema
 // initialization stay disabled even if files disappear during acquisition.
 p->primary=std::make_unique<ChainStore>(p->primary_path,ChainStore::OpenMode::ReadWriteExisting);
 p->witness=std::make_unique<ChainStore>(p->witness_path,ChainStore::OpenMode::ReadWriteExisting);
 require(directory(p->primary_path)==p->primary_id&&directory(p->witness_path)==p->witness_id,"checkpoint GC copies replaced during exclusive open");
 check();return std::unique_ptr<CheckpointBlobGcCopies>(new CheckpointBlobGcCopies(std::move(p)));
}
CheckpointBlobGcPlan CheckpointBlobGcCopies::plan_locked(const CapellaForkBlockRules& rules,
 const CapellaStoreSnapshotLimits& limits,const CheckpointBlobGcBudget& budget,const std::function<void()>& check){
 auto& p=*impl_;validate_budget(budget);require(!p.unavailable,"checkpoint GC incomplete; destroy and reopen required");
 require(directory(p.primary_path)==p.primary_id&&directory(p.witness_path)==p.witness_id,"checkpoint GC copy directory changed");
 require(directory(p.primary_source)==p.primary_source_id&&directory(p.witness_source)==p.witness_source_id&&checkpoint_blob_gc_copy_inventory(p.primary_source,budget.inventory_files,budget.inventory_bytes,check)==p.primary_source_inventory&&checkpoint_blob_gc_copy_inventory(p.witness_source,budget.inventory_files,budget.inventory_bytes,check)==p.witness_source_inventory,"checkpoint GC original source changed");
 const auto& j=p.request;const Json encoded_budget=Json::array({quantity(Int(std::to_string(budget.inventory_files))),quantity(Int(std::to_string(budget.inventory_bytes))),quantity(Int(std::to_string(budget.scan_records))),quantity(Int(std::to_string(budget.scan_bytes))),quantity(Int(std::to_string(budget.delete_records))),quantity(Int(std::to_string(budget.delete_bytes))),quantity(Int(std::to_string(budget.batch_bytes))),quantity(Int(std::to_string(budget.duration_ms)))});require(encoded_budget==j.at("budget"),"checkpoint GC administrative budget differs from pinned request");
 require(hash_text(capella_fork_profile(rules).dump())==sha_text(j.at("profileSha256")),"checkpoint GC profile differs from pinned request");
 const Json encoded_limits=Json::array({quantity(Int(std::to_string(limits.envelope_bytes))),quantity(Int(std::to_string(limits.blocks))),quantity(Int(std::to_string(limits.messages))),quantity(Int(std::to_string(limits.equivocations))),quantity(Int(std::to_string(limits.state_bytes))),quantity(Int(std::to_string(limits.total_state_bytes))),quantity(Int(std::to_string(limits.replay_slots))),quantity(Int(std::to_string(limits.sync_trials)))});require(encoded_limits==j.at("limits"),"checkpoint GC snapshot limits changed");
 require(rules.transition.chain_id==quantity(j.at("chainId")),"checkpoint GC profile/chain binding");
 std::scoped_lock stores(p.primary->mutex_,p.witness->mutex_);
 const auto chain=quantity(j.at("chainId"));const auto genesis=root(j.at("genesisHash"));
 for(auto pair:{std::pair{p.primary.get(),"beacon"},std::pair{p.witness.get(),"nano-capella-recovery-v1"}}){const auto identity=pair.first->read("meta/identity");require(identity&&identity->at("consensus")==pair.second,"checkpoint GC store role refused");const auto schema=pair.first->get("meta/schema");require(schema&&(*schema=="nano/1"||*schema=="nano/2-state-pruned"),"checkpoint GC unknown schema");}
 require(limits.envelope_bytes<=(SIZE_MAX-2048)/8,"checkpoint GC witness limit overflow");const auto witness_limit=2048+8*limits.envelope_bytes;
 const auto primary=p.primary->fork_checkpoint_impl(chain,genesis,limits.envelope_bytes),witness=p.witness->fork_checkpoint_impl(chain,genesis,witness_limit);
 require(primary&&witness&&primary->token==token(j.at("primary").at("token"))&&witness->token==token(j.at("witness").at("token")),"checkpoint GC independently pinned primary/witness mismatch");
 require(p.primary->get("execution-checkpoint/v1").has_value(),"checkpoint GC requires coupled execution primary");
 const auto genesis_block=p.primary->read("block/"+hex(bytes(0,8)));require(genesis_block&&Block::from_json(*genesis_block).header.hash()==genesis,"checkpoint GC canonical genesis binding");
 const auto intent=Json::parse(witness->snapshot);shape(intent,{"format","previous","next","snapshot","archive"});require(intent.at("format")=="nano-capella-recovery-v1"&&intent.dump()==witness->snapshot,"checkpoint GC unknown recovery namespace/encoding");
 const auto next=token(intent.at("next"));const auto previous=intent.at("previous").is_null()?std::optional<ForkCheckpointToken>{}:std::optional(token(intent.at("previous")));
 require((!previous||previous->sequence<UINT64_MAX)&&next.sequence==(previous?previous->sequence+1:1)&&next==token(j.at("next")),"checkpoint GC pending-next external binding mismatch");
 require(primary->token==next||(previous&&primary->token==*previous),"checkpoint GC primary is neither completed nor pending previous");const bool pending=primary->token!=next;
 require(intent.at("snapshot").is_string(),"checkpoint GC recovery snapshot required");const auto& next_snapshot=intent.at("snapshot").get_ref<const std::string&>();require(pending||primary->snapshot==next_snapshot,"checkpoint GC completed snapshot mismatch");
 const auto fetch=[&](ChainStore& db){return [&db](const Bytes& id,size_t maximum)->std::optional<std::string>{const auto payload=db.get(std::string(blob_prefix)+hex(id));if(payload)require(!payload->empty()&&payload->size()<=maximum&&sha256(Bytes(payload->begin(),payload->end()))==id,"checkpoint GC corrupt referenced blob");return payload;};};
 const auto a=authenticate_capella_checkpoint_blob_closure(primary->snapshot,primary->token.digest,rules,limits,fetch(*p.primary),{},check);
 const auto b=authenticate_capella_checkpoint_blob_closure(next_snapshot,next.digest,rules,limits,fetch(*p.witness),archives(intent.at("archive"),limits.envelope_bytes),check);
 const auto tip_key=p.primary->get("meta/tip");const auto tip=tip_key?p.primary->read("block/"+*tip_key):std::nullopt;const auto execution=tip_key?p.primary->read("state/"+*tip_key):std::nullopt;
 require(tip&&execution&&Block::from_json(*tip).header.hash()==root(a.execution_head_.at("hash"))&&quantity(tip->at("number"))==quantity(a.execution_head_.at("number"))&&State::from_json(*execution).root()==root(a.execution_head_.at("stateRoot")),"checkpoint GC primary execution head/state binding");
 require(a.identity_==b.identity_,"checkpoint GC previous/next protocol identity differs");CheckpointBlobGcPlan plan;plan.owner_=&p;plan.identity_=a.identity_;plan.rules_=rules;plan.limits_=limits;plan.budget_=budget;
 plan.primary_token_=primary->token;plan.witness_token_=witness->token;plan.next_token_=next;plan.primary_snapshot_=primary->snapshot;plan.witness_snapshot_=witness->snapshot;plan.protected_=a.references_;
 for(const auto& [id,n]:b.references_){const auto [where,added]=plan.protected_.emplace(id,n);require(added||where->second==n,"checkpoint GC conflicting protected lengths");}
 for(const auto& [id,n]:plan.protected_){
  const auto pp=fetch(*p.primary)(id,n),ww=fetch(*p.witness)(id,n);
  require((pp&&pp->size()==n)||(pending&&b.references_.contains(id)&&!a.references_.contains(id)),"checkpoint GC missing protected primary reference");
  require(ww&&ww->size()==n,"checkpoint GC missing protected witness reference");if(pp)require(*pp==*ww,"checkpoint GC protected stores differ");
 }
 const auto primary_scan=scan(*p.primary->db_,budget,limits.envelope_bytes,check),witness_scan=scan(*p.witness->db_,budget,limits.envelope_bytes,check);plan.non_blob_primary_=primary_scan.non_blobs;plan.non_blob_witness_=witness_scan.non_blobs;
 uint64_t deletes=0,deleted_bytes=0;
 const auto candidates=[&](const Scan& source,auto& output){for(const auto& [id,n]:source.blobs)if(!plan.protected_.contains(id)){add(deletes,1);add(deleted_bytes,n);require(deletes<=budget.delete_records&&deleted_bytes<=budget.delete_bytes,"checkpoint GC delete budget exhausted before complete plan");const auto key=std::string(blob_prefix)+hex(id);rocksdb::WriteBatch one;one.Delete(key);require(one.GetDataSize()<=budget.batch_bytes,"checkpoint GC single Delete exceeds batch budget before plan");output.push_back({key,id,n});}};
 candidates(primary_scan,plan.primary_);candidates(witness_scan,plan.witness_);plan.id_=hash_text(plan.description().dump());if(check)check();return plan;
}
CheckpointBlobGcPlan CheckpointBlobGcCopies::plan(const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const CheckpointBlobGcBudget& budget,const std::function<void()>& check){
 std::lock_guard lock(impl_->mutex);Busy busy(impl_->busy);require(!impl_->finished,"checkpoint GC session already finished; reopen required");return plan_locked(rules,limits,budget,deadline(budget.duration_ms,check));
}
Json CheckpointBlobGcCopies::collect(const CheckpointBlobGcPlan& plan,const std::function<void()>& outer){
 auto& p=*impl_;std::lock_guard lock(p.mutex);Busy busy(p.busy);require(!p.finished&&!p.unavailable&&plan.owner_==&p,"checkpoint GC plan ownership/session mismatch");const auto check=deadline(plan.budget_.duration_ms,outer);
 const auto fresh=plan_locked(plan.rules_,plan.limits_,plan.budget_,check);require(fresh.id_==plan.id_&&fresh.description()==plan.description(),"checkpoint GC stale plan/candidate inventory refused");
 Json counts=Json::array();bool changed=false;
 try{
  for(auto pair:{std::pair{p.primary.get(),&plan.primary_},std::pair{p.witness.get(),&plan.witness_}}){
   auto& db=*pair.first;uint64_t deleted=0,bytes=0;
   {std::lock_guard gate(db.mutex_);rocksdb::WriteBatch batch;
    auto flush=[&]{if(!batch.Count())return;check();rocksdb::WriteOptions write;write.sync=true;const auto result=db.db_->Write(write,&batch);if(!result.ok())db.fork_checkpoint_write_uncertain_=true;status(result);changed=true;batch.Clear();};
    for(const auto& row:*pair.second){check();const auto current=db.get(row.key);require(current&&current->size()==row.bytes&&hash_text(*current)==hex(row.digest).substr(2),"checkpoint GC candidate changed before Delete");
     rocksdb::WriteBatch one;one.Delete(row.key);require(one.GetDataSize()<=plan.budget_.batch_bytes,"checkpoint GC single Delete exceeds batch budget");
     if(batch.GetDataSize()+one.GetDataSize()>plan.budget_.batch_bytes)flush();batch.Delete(row.key);require(batch.GetDataSize()<=plan.budget_.batch_bytes,"checkpoint GC batch overflow");add(deleted,1);add(bytes,row.bytes);
    }flush();
    check();rocksdb::FlushOptions options;options.wait=true;status(db.db_->Flush(options));
    const std::string begin(blob_prefix),end="checkpoint-blob/v10";const rocksdb::Slice a(begin),b(end);rocksdb::CompactRangeOptions compact;compact.exclusive_manual_compaction=true;compact.max_subcompactions=1;check();status(db.db_->CompactRange(compact,&a,&b));
   }counts.push_back({{"deletedRecords",deleted},{"deletedLogicalPayloadBytes",bytes}});
  }
  // No old provider/cache/lease exists. Reopen both stores through the RO API.
  p.primary.reset();p.witness.reset();p.primary=std::make_unique<ChainStore>(p.primary_path,ChainStore::OpenMode::ReadOnly);p.witness=std::make_unique<ChainStore>(p.witness_path,ChainStore::OpenMode::ReadOnly);
  const auto after=plan_locked(plan.rules_,plan.limits_,plan.budget_,check);
  require(after.primary_.empty()&&after.witness_.empty()&&after.protected_==plan.protected_&&after.identity_==plan.identity_&&after.primary_token_==plan.primary_token_&&after.witness_token_==plan.witness_token_&&after.next_token_==plan.next_token_&&after.primary_snapshot_==plan.primary_snapshot_&&after.witness_snapshot_==plan.witness_snapshot_&&after.non_blob_primary_==plan.non_blob_primary_&&after.non_blob_witness_==plan.non_blob_witness_,"checkpoint GC cold reopen/unchanged namespaces failed");
  check();p.finished=true;return {{"format","nano-owned-checkpoint-blob-gc-result-v1"},{"planId",plan.id_},{"stores",counts},{"noWork",plan.primary_.empty()&&plan.witness_.empty()},{"allProtectedReferencesReopened",true},{"nonGcNamespacesUnchanged",true},{"originalSourceInventoriesUnchanged",true},{"primary",token_json(plan.primary_token_)},{"witness",token_json(plan.witness_token_)},{"pendingNext",token_json(plan.next_token_)},{"primaryAndWitnessAtomicTransaction",false},{"copiedDataOnly",true},{"physicalSpaceReductionGuaranteed",false},{"physicalPowerLossQualified",false},{"runtimeDutiesResumed",false},{"productionQualified",false}};
 }catch(...){p.unavailable=true;p.primary.reset();p.witness.reset();(void)changed;throw;}
}
}
