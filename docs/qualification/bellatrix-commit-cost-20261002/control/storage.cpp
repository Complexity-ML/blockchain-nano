#include "/control/profile_timer.hpp"
#include "nano/storage.hpp"
#include "nano/block_auth.hpp"
#include "nano/finality.hpp"
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <memory>
#include <set>
#include <limits>
namespace nano {
namespace {
std::string height_key(const Int& number){return hex(bytes(number,8));}
void require_ok(const rocksdb::Status& status){if(!status.ok())throw std::runtime_error("RocksDB: "+status.ToString());}
// Detect the API, rather than guessing the release where DB** was removed.
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_database(const rocksdb::Options& options,const std::string& path){
 std::unique_ptr<Database> opened;
 if constexpr(requires { Database::Open(options,path,&opened); }){
  require_ok(Database::Open(options,path,&opened));
 }else{
  Database* raw=nullptr;
  auto status=Database::Open(options,path,&raw);
  opened.reset(raw);
  require_ok(status);
 }
 return opened;
}
template<class Database=rocksdb::DB>
std::unique_ptr<Database> open_read_only_database(const rocksdb::Options& options,const std::string& path){
 std::unique_ptr<Database> opened;
 if constexpr(requires { Database::OpenForReadOnly(options,path,&opened); }){
  require_ok(Database::OpenForReadOnly(options,path,&opened));
 }else{
  Database* raw=nullptr;auto status=Database::OpenForReadOnly(options,path,&raw);
  opened.reset(raw);require_ok(status);
 }
 return opened;
}
std::string vote_slot(const FinalityVote& vote){return height_key(Int(std::to_string(vote.height)))+"/"+height_key(Int(std::to_string(vote.round)))+"/"+hex(bytes(Int(static_cast<unsigned>(vote.step)),1));}
Json vote_fields(const FinalityVote& vote){return {{"height",quantity(Int(std::to_string(vote.height)))},{"round",quantity(Int(std::to_string(vote.round)))},
 {"step",static_cast<unsigned>(vote.step)},{"validatorSetHash",hex(vote.validator_set_hash)},
 {"blockHash",vote.block_hash?Json(hex(*vote.block_hash)):Json(nullptr)},{"validator",address(vote.validator)}};}
Json encode_members(const VotingSet& members){Json result=Json::array();for(const auto& [member,power]:members)result.push_back({{"validator",address(member)},{"power",quantity(power)}});return result;}
VotingSet decode_members(const Json& members){
 if(!members.is_array()||members.empty())throw std::runtime_error("invalid persisted voting set");
 VotingSet result;for(const auto& member:members)result.emplace_back(address(member.at("validator").get<std::string>()),quantity(member.at("power")));voting_set_hash(result);return result;
}
Json encode_certificate(const Block& block,const BlockCertificate& certificate){
 Json votes=Json::array();for(const auto& vote:certificate.votes)votes.push_back({{"validator",address(vote.validator)},{"signature",hex(vote.signature)}});
 return {{"height",quantity(block.header.number)},{"blockHash",hex(block.header.hash())},{"round",quantity(Int(std::to_string(certificate.round)))},
 {"validators",encode_members(certificate.validators)},{"nextValidators",encode_members(certificate.next_validators)},{"precommits",votes}};
}
constexpr size_t proposal_inline_limit=16*1024*1024;
constexpr size_t proposal_chunk_size=1024*1024;
const std::string proposal_chunk_tag="nano/proposal/chunks-v1\n";
std::string proposal_chunk_key(const std::string& key,size_t index){return key+"/chunks/"+std::to_string(index);}
std::string proposal_digest(const std::string& data){return hex(keccak(Bytes(data.begin(),data.end())));}
void put_proposal(rocksdb::WriteBatch& batch,const std::string& key,const std::string& data){
 if(data.size()<=proposal_inline_limit){batch.Put(key,data);return;}
 const size_t count=(data.size()-1)/proposal_chunk_size+1;
 for(size_t i=0;i<count;++i){const size_t offset=i*proposal_chunk_size;batch.Put(proposal_chunk_key(key,i),rocksdb::Slice(data.data()+offset,std::min(proposal_chunk_size,data.size()-offset)));}
 batch.Put(key,proposal_chunk_tag+Json{{"bytes",data.size()},{"chunks",count},{"keccak",proposal_digest(data)}}.dump());
}
rocksdb::WriteOptions durable_write(){rocksdb::WriteOptions options;options.sync=true;return options;}
}
std::optional<std::string> ChainStore::get(const std::string& key)const{
 if(fork_checkpoint_write_uncertain_)throw std::runtime_error("checkpoint write uncertain; reopen required");
 std::string value;auto status=db_->Get(rocksdb::ReadOptions(),key,&value);
 if(status.IsNotFound())return std::nullopt;require_ok(status);return value;
}
std::optional<Json> ChainStore::read(const std::string& key)const{auto value=get(key);return value?std::optional<Json>(Json::parse(*value)):std::nullopt;}
std::optional<std::string> ChainStore::proposal_data_impl(const std::string& key)const{
 auto encoded=get(key);if(!encoded)return std::nullopt;
 if(!encoded->starts_with(proposal_chunk_tag)){
  if(encoded->starts_with("nano/proposal/"))throw std::runtime_error("unknown voting proposal storage version");
  if(encoded->size()>proposal_inline_limit)throw std::runtime_error("stored inline voting proposal exceeds limit");return encoded;
 }
 if(encoded->size()>1024)throw std::runtime_error("voting proposal manifest exceeds limit");
 auto manifest=Json::parse(encoded->substr(proposal_chunk_tag.size()));
 if(!manifest.is_object()||manifest.size()!=3||!manifest.at("bytes").is_number_unsigned()||!manifest.at("chunks").is_number_unsigned()||!manifest.at("keccak").is_string())throw std::runtime_error("invalid voting proposal manifest");
 const auto total=manifest.at("bytes").get<uint64_t>(),count=manifest.at("chunks").get<uint64_t>();
 const auto digest=manifest.at("keccak").get<std::string>();
 if(total<=proposal_inline_limit||total>std::string{}.max_size()||count!=(total-1)/proposal_chunk_size+1||digest.size()!=66||hex(unhex(digest))!=digest||unhex(digest).size()!=32)throw std::runtime_error("invalid voting proposal manifest bounds");
 // Do not allocate from untrusted metadata. Grow only from actual bounded
 // chunks, and verify all bytes before parsing JSON or resuming a vote.
 std::string result;
 for(uint64_t i=0;i<count;++i){auto chunk=get(proposal_chunk_key(key,static_cast<size_t>(i)));const auto expected=std::min<uint64_t>(proposal_chunk_size,total-i*proposal_chunk_size);if(!chunk||chunk->size()!=expected)throw std::runtime_error("missing or truncated voting proposal chunk");result.append(*chunk);}
 if(result.size()!=total||proposal_digest(result)!=digest)throw std::runtime_error("voting proposal chunk digest mismatch");return result;
}
ChainStore::ChainStore(const std::filesystem::path& path,OpenMode mode){
 const bool read_only=mode==OpenMode::ReadOnly;
 if(!read_only&&path.has_parent_path())std::filesystem::create_directories(path.parent_path());
 rocksdb::Options options;options.create_if_missing=!read_only;options.paranoid_checks=true;
 // A validator opens separate signing journals and recovery witnesses. RocksDB's
 // default file preallocation multiplies reserved disk space across these tiny
 // databases. Allocate on demand; keep WAL and synchronous durability unchanged.
 options.allow_fallocate=false;
 db_=(read_only?open_read_only_database(options,path.string()):open_database(options,path.string())).release();
 try{
  auto schema=get("meta/schema");
  if(schema&&*schema!="nano/1")throw std::runtime_error("unsupported Nano database schema");
  if(!schema){
   if(read_only)throw std::runtime_error("read-only database has no Nano schema");
   std::unique_ptr<rocksdb::Iterator> iterator(db_->NewIterator(rocksdb::ReadOptions()));iterator->SeekToFirst();require_ok(iterator->status());
   if(iterator->Valid())throw std::runtime_error("database has no Nano schema");
   require_ok(db_->Put(durable_write(),"meta/schema","nano/1"));
  }
 }catch(...){delete db_;db_=nullptr;throw;}
}
ChainStore::~ChainStore(){delete db_;}
void ChainStore::bind_chain(const Int& chain_id,const Bytes& genesis_hash,const std::string& consensus,const std::string& staking_contract){
 std::lock_guard lock(mutex_);Json identity={{"chainId",quantity(chain_id)},{"genesisHash",hex(genesis_hash)},{"consensus",consensus}};
 if(!staking_contract.empty())identity["stakingContract"]=address(staking_contract);
 auto previous=read("meta/identity");
 if(previous){if(*previous!=identity)throw std::invalid_argument("database belongs to another chain");return;}
 auto first=read("block/"+height_key(0));
 if(first&&first->at("hash")!=hex(genesis_hash))throw std::invalid_argument("genesis does not match database");
 require_ok(db_->Put(durable_write(),"meta/identity",identity.dump()));
}
std::optional<Json> ChainStore::voting_watermark_impl(const std::string& prefix)const{
 auto watermark=read(prefix+"watermark");
 std::unique_ptr<rocksdb::Iterator> iterator(db_->NewIterator(rocksdb::ReadOptions()));
 // Fixed-width slots start with 0x; the next lexical prefix bounds this index.
 iterator->SeekForPrev(prefix+"1");require_ok(iterator->status());
 std::optional<std::string> latest;
 if(iterator->Valid()&&iterator->key().ToString().starts_with(prefix+"0x"))latest=iterator->key().ToString().substr(prefix.size());
 if(!watermark){
  if(latest)throw std::runtime_error("missing voting watermark");
  for(const auto& section:{"reserved/","lock/"}){auto key=prefix+section;iterator->Seek(key);require_ok(iterator->status());if(iterator->Valid()&&iterator->key().ToString().starts_with(key))throw std::runtime_error("orphaned voting records without watermark");}
  return std::nullopt;
 }
 auto slot=watermark->at("slot").get<std::string>();
 if(!latest||slot!=*latest||slot.size()!=42||slot[18]!='/'||slot[37]!='/'||
    (slot.substr(38)!="0x01"&&slot.substr(38)!="0x02")||quantity(slot.substr(0,18))<=0||
    height_key(quantity(slot.substr(0,18)))!=slot.substr(0,18)||height_key(quantity(slot.substr(19,18)))!=slot.substr(19,18))throw std::runtime_error("voting watermark is not the latest canonical slot");
 auto digest=get(prefix+slot);if(!digest||*digest!=watermark->at("digest").get<std::string>()||unhex(*digest).size()!=32||hex(unhex(*digest))!=*digest)throw std::runtime_error("corrupt voting watermark");
 return watermark;
}
void ChainStore::reserve_vote(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash){
 reserve_vote_impl(vote,chain_id,genesis_hash,nullptr,{});
}
void ChainStore::reserve_consensus_vote(const FinalityVote& vote,const VotingSet& validators,const std::vector<FinalityVote>& justification,const Int& chain_id,const Bytes& genesis_hash,const Block* proposal){
 reserve_vote_impl(vote,chain_id,genesis_hash,&validators,justification,proposal);
}
void ChainStore::reserve_vote_impl(const FinalityVote& vote,const Int& chain_id,const Bytes& genesis_hash,const VotingSet* validators,const std::vector<FinalityVote>& justification,const Block* proposal){
 auto digest=hex(vote_signing_hash(vote,chain_id,genesis_hash));
 auto prefix="voting/v1/"+hex(bytes(chain_id,32))+"/"+hex(genesis_hash)+"/"+address(vote.validator)+"/";
 auto slot=vote_slot(vote);
 std::optional<std::string> proposal_data;std::string proposal_key;bool proposal_exists=false;
 if(proposal){
  if(!validators||!vote.block_hash||proposal->header.number!=Int(std::to_string(vote.height))||proposal->header.hash()!=*vote.block_hash||!proposal->uncles.empty()||!verify_block_signature(*proposal,chain_id,genesis_hash))throw std::invalid_argument("proposal does not match authenticated vote target");
  proposal_data=proposal->to_transport_json().dump();
  proposal_key=prefix+"proposal/"+height_key(Int(std::to_string(vote.height)))+"/"+hex(*vote.block_hash);
 }
 std::lock_guard guard(mutex_);
 if(proposal_data){auto saved=proposal_data_impl(proposal_key);if(saved){
  // Old journals contain the expanded representation. Accept only the exact
  // legacy serialization of this same block, and retain its original bytes.
  if(*saved!=*proposal_data&&*saved!=proposal->to_json(true).dump())throw std::runtime_error("conflicting stored voting proposal");
  proposal_exists=true;
 }}
 auto watermark=voting_watermark_impl(prefix);auto previous=get(prefix+slot);
 const std::string mode=validators?"locked":"raw";
 auto stored_mode=get(prefix+"mode");
 if((stored_mode&&*stored_mode!=mode)||(!stored_mode&&watermark&&validators))throw std::runtime_error("voting journal mode mismatch");
 auto height=hex(bytes(Int(std::to_string(vote.height)),8));auto lock_key=prefix+"lock/"+height;
 VoteLock lock{vote.height,vote.validator_set_hash,std::nullopt,std::nullopt};
 if(validators){
  if(voting_set_hash(*validators)!=vote.validator_set_hash)throw std::invalid_argument("vote validator set mismatch");
  auto saved=read(lock_key);
  if(saved){
   lock.validator_set_hash=unhex(saved->at("validatorSetHash").get<std::string>());
   if(!saved->at("round").is_null())lock.round=u64(quantity(saved->at("round")));
   if(!saved->at("blockHash").is_null())lock.block_hash=unhex(saved->at("blockHash").get<std::string>());
   if(lock.validator_set_hash!=vote.validator_set_hash||lock.round.has_value()!=lock.block_hash.has_value()||(lock.block_hash&&lock.block_hash->size()!=32))throw std::runtime_error("corrupt voting lock");
  }else if(previous||(watermark&&watermark->at("slot").get<std::string>().starts_with(height+"/")))throw std::runtime_error("missing voting lock");
 }
 if(previous){if(*previous!=digest)throw std::runtime_error("conflicting vote refused at height/round/step");
  auto fields=read(prefix+"reserved/"+slot);if(fields&&*fields!=vote_fields(vote))throw std::runtime_error("corrupt reserved vote fields");
  if(proposal_data&&!proposal_exists){rocksdb::WriteBatch repair;put_proposal(repair,proposal_key,*proposal_data);require_ok(db_->Write(durable_write(),&repair));}return;}
 if(watermark&&slot<=watermark->at("slot").get<std::string>())throw std::runtime_error("new vote would regress voting watermark");
 auto active=validators?read(prefix+"active"):std::nullopt;
 if(validators){
  if(active){if(quantity(active->at("height"))!=Int(std::to_string(vote.height))||active->at("validatorSetHash")!=hex(vote.validator_set_hash))throw std::runtime_error("vote differs from certified active height or validator set");}
  else if(stored_mode||watermark||vote.height!=1)throw std::runtime_error("voting must start at height one with certified progression");
  lock=advance_vote_lock(lock,vote,justification,*validators,chain_id,genesis_hash);
 }

 rocksdb::WriteBatch batch;if(proposal_data&&!proposal_exists)put_proposal(batch,proposal_key,*proposal_data);batch.Put(prefix+slot,digest);batch.Put(prefix+"mode",mode);
 if(validators)batch.Put(prefix+"reserved/"+slot,vote_fields(vote).dump());
 if(validators&&!active)batch.Put(prefix+"active",Json{{"height","0x1"},{"validatorSetHash",hex(vote.validator_set_hash)}}.dump());
 if(validators)batch.Put(lock_key,Json{{"validatorSetHash",hex(lock.validator_set_hash)},
   {"round",lock.round?Json(quantity(Int(std::to_string(*lock.round)))):Json(nullptr)},
   {"blockHash",lock.block_hash?Json(hex(*lock.block_hash)):Json(nullptr)}}.dump());
 batch.Put(prefix+"watermark",Json{{"slot",slot},{"digest",digest}}.dump());
 require_ok(db_->Write(durable_write(),&batch));
}
std::optional<Block> ChainStore::voting_proposal(const std::string& validator,uint64_t height,const Bytes& block_hash,const Int& chain_id,const Bytes& genesis_hash)const{
 if(!height||block_hash.size()!=32||chain_id<=0||chain_id>=(Int(1)<<256)||genesis_hash.size()!=32)throw std::invalid_argument("invalid voting proposal domain");
 auto prefix="voting/v1/"+hex(bytes(chain_id,32))+"/"+hex(genesis_hash)+"/"+address(validator)+"/";
 std::lock_guard lock(mutex_);voting_watermark_impl(prefix);auto encoded=proposal_data_impl(prefix+"proposal/"+height_key(Int(std::to_string(height)))+"/"+hex(block_hash));if(!encoded)return std::nullopt;
 auto block=Block::from_json(Json::parse(*encoded));
 if(block.header.number!=Int(std::to_string(height))||block.header.hash()!=block_hash||!block.uncles.empty()||!verify_block_signature(block,chain_id,genesis_hash))throw std::runtime_error("corrupt stored voting proposal");return block;
}
std::optional<FinalityVote> ChainStore::latest_consensus_vote(const std::string& validator,const Int& chain_id,const Bytes& genesis_hash)const{
 if(chain_id<=0||chain_id>=(Int(1)<<256)||genesis_hash.size()!=32)throw std::invalid_argument("invalid voting domain");
 auto signer=address(validator);auto prefix="voting/v1/"+hex(bytes(chain_id,32))+"/"+hex(genesis_hash)+"/"+signer+"/";
 std::lock_guard lock(mutex_);auto mode=get(prefix+"mode");auto watermark=voting_watermark_impl(prefix);
 if(mode&&*mode!="locked")throw std::runtime_error("voting journal mode mismatch");
 if(!watermark)return std::nullopt;
 if(!mode)throw std::runtime_error("missing voting journal mode");
 auto slot=watermark->at("slot").get<std::string>();auto digest=get(prefix+slot);auto fields=read(prefix+"reserved/"+slot);
 if(!digest||*digest!=watermark->at("digest").get<std::string>())throw std::runtime_error("corrupt voting watermark");
 if(!fields)throw std::runtime_error("reserved vote fields unavailable; cannot reconstruct vote");
 FinalityVote vote;vote.height=u64(quantity(fields->at("height")));vote.round=u64(quantity(fields->at("round")));
 if(!fields->at("step").is_number_unsigned())throw std::runtime_error("invalid reserved vote step");
 auto step=fields->at("step").get<uint64_t>();if(step<1||step>2)throw std::runtime_error("invalid reserved vote step");vote.step=static_cast<VoteStep>(step);
 vote.validator_set_hash=unhex(fields->at("validatorSetHash"));vote.validator=fields->at("validator").get<std::string>();
 if(!fields->at("blockHash").is_null())vote.block_hash=unhex(fields->at("blockHash"));
 if(vote.validator!=signer||vote_slot(vote)!=slot||vote_fields(vote)!=*fields||hex(vote_signing_hash(vote,chain_id,genesis_hash))!=*digest)throw std::runtime_error("corrupt reserved vote fields");
 return vote;
}
std::optional<VoteLock> ChainStore::voting_lock_impl(const std::string& prefix)const{
 auto watermark=voting_watermark_impl(prefix);auto mode=get(prefix+"mode");auto active=read(prefix+"active");
 if(mode&&*mode!="locked")throw std::runtime_error("voting journal mode mismatch");
 if(!active){if(mode||watermark)throw std::runtime_error("missing active voting height");return std::nullopt;}
 if(!mode)throw std::runtime_error("missing voting journal mode");
 VoteLock result{u64(quantity(active->at("height"))),unhex(active->at("validatorSetHash")),std::nullopt,std::nullopt};
 if(!result.height||result.validator_set_hash.size()!=32||*active!=Json{{"height",quantity(Int(std::to_string(result.height)))},{"validatorSetHash",hex(result.validator_set_hash)}})throw std::runtime_error("invalid active voting position");
 uint64_t last_height=0,last_round=0;
 if(watermark){auto slot=watermark->at("slot").get<std::string>();last_height=u64(quantity(slot.substr(0,18)));last_round=u64(quantity(slot.substr(19,18)));if(last_height>result.height)throw std::runtime_error("voting position precedes reserved vote");}
 auto saved=read(prefix+"lock/"+height_key(Int(std::to_string(result.height))));
 if(!saved){if(last_height==result.height)throw std::runtime_error("missing voting lock");return result;}
 if(last_height!=result.height)throw std::runtime_error("voting lock has no active-height reservation");
 if(!saved->at("round").is_null())result.round=u64(quantity(saved->at("round")));
 if(!saved->at("blockHash").is_null())result.block_hash=unhex(saved->at("blockHash"));
 Json expected={{"validatorSetHash",hex(result.validator_set_hash)},{"round",result.round?Json(quantity(Int(std::to_string(*result.round)))):Json(nullptr)},{"blockHash",result.block_hash?Json(hex(*result.block_hash)):Json(nullptr)}};
 if(result.round.has_value()!=result.block_hash.has_value()||(result.round&&*result.round>last_round)||(result.block_hash&&result.block_hash->size()!=32)||*saved!=expected)throw std::runtime_error("corrupt active voting lock");
 return result;
}
std::optional<VoteLock> ChainStore::voting_lock(const std::string& validator,const Int& chain_id,const Bytes& genesis_hash)const{
 if(chain_id<=0||chain_id>=(Int(1)<<256)||genesis_hash.size()!=32)throw std::invalid_argument("invalid voting domain");
 auto prefix="voting/v1/"+hex(bytes(chain_id,32))+"/"+hex(genesis_hash)+"/"+address(validator)+"/";
 std::lock_guard lock(mutex_);return voting_lock_impl(prefix);
}
std::optional<VotingPosition> ChainStore::voting_position(const std::string& validator,const Int& chain_id,const Bytes& genesis_hash)const{
 auto lock=voting_lock(validator,chain_id,genesis_hash);
 if(!lock)return std::nullopt;return VotingPosition{lock->height,lock->validator_set_hash};
}
void ChainStore::advance_voting_height(const std::string& validator,uint64_t height,uint64_t round,const Bytes& block_hash,const std::vector<FinalityVote>& certificate,const VotingSet& current_set,const VotingSet& next_set,const Int& chain_id,const Bytes& genesis_hash){
 if(height==std::numeric_limits<uint64_t>::max()||!verify_commit_certificate(certificate,current_set,height,round,block_hash,chain_id,genesis_hash))throw std::invalid_argument("invalid height progression certificate");
 auto current_hash=hex(voting_set_hash(current_set)),next_hash=hex(voting_set_hash(next_set));
 auto prefix="voting/v1/"+hex(bytes(chain_id,32))+"/"+hex(genesis_hash)+"/"+address(validator)+"/";
 auto cert_key=prefix+"certificate/"+hex(bytes(Int(std::to_string(height)),8));
 std::lock_guard guard(mutex_);voting_watermark_impl(prefix);auto mode=get(prefix+"mode");
 if((mode&&*mode!="locked")||(!mode&&get(prefix+"watermark")))throw std::runtime_error("voting journal mode mismatch");
 auto active=read(prefix+"active");if(mode&&!active)throw std::runtime_error("missing active voting height");
 auto existing=read(cert_key);
 if(existing){if(existing->at("blockHash")!=hex(block_hash)||existing->at("validatorSetHash")!=current_hash||existing->at("nextValidatorSetHash")!=next_hash)throw std::runtime_error("conflicting certified height");return;}
 if(active){if(quantity(active->at("height"))!=Int(std::to_string(height))||active->at("validatorSetHash")!=current_hash)throw std::runtime_error("certificate does not advance active voting height");}
 else if(height!=1||get(prefix+"watermark"))throw std::runtime_error("certificate sequence must start at height one");
 Json votes=Json::array();for(const auto& vote:certificate)votes.push_back({{"validator",address(vote.validator)},{"signature",hex(vote.signature)}});
 Json members=Json::array();for(const auto& [member,power]:current_set)members.push_back({{"validator",address(member)},{"power",quantity(power)}});
 Json record={{"height",quantity(Int(std::to_string(height)))},{"round",quantity(Int(std::to_string(round)))},{"blockHash",hex(block_hash)},
  {"validatorSetHash",current_hash},{"nextValidatorSetHash",next_hash},{"validators",members},{"precommits",votes}};
 rocksdb::WriteBatch batch;batch.Put(cert_key,record.dump());batch.Put(prefix+"mode","locked");
 batch.Put(prefix+"active",Json{{"height",quantity(Int(std::to_string(height+1)))},{"validatorSetHash",next_hash}}.dump());
 require_ok(db_->Write(durable_write(),&batch));
}
std::optional<Block> ChainStore::signing_proposal_impl(const std::string& key,const Int& chain_id,const Bytes& genesis_hash,const std::string& producer,const Int& height,bool required)const{
 auto encoded=read(key+"/proposal");auto digest=get(key);
 if(!encoded){if(required)throw std::runtime_error("reserved signing proposal unavailable");return std::nullopt;}
 auto block=Block::from_json(*encoded);
 if(!digest||block.header.number!=height||address(block.header.coinbase)!=address(producer)||hex(block_signing_hash(block,chain_id,genesis_hash))!=*digest)throw std::runtime_error("corrupt signing proposal");
 return block;
}
void ChainStore::reserve_signature(const Int& chain_id,const Bytes& genesis_hash,const std::string& producer,const Int& height,const Bytes& digest,const std::optional<Block>& proposal,std::optional<uint64_t> round){
 if(chain_id<=0||genesis_hash.size()!=32||digest.size()!=32||height<=0)throw std::invalid_argument("invalid signing reservation");
 const auto base="signing/v1/"+hex(bytes(chain_id,32))+"/"+hex(genesis_hash)+"/"+address(producer)+"/"+height_key(height);auto key=base;
 if(proposal&&(proposal->header.number!=height||address(proposal->header.coinbase)!=address(producer)||block_signing_hash(*proposal,chain_id,genesis_hash)!=digest))throw std::invalid_argument("proposal differs from signing reservation");
 if(round&&(!proposal||bft_proposal_round(*proposal)!=*round))throw std::invalid_argument("proposal differs from signing round");
 std::lock_guard lock(mutex_);auto legacy=get(base);const auto encoded=hex(digest);constexpr auto guard="nano/bft-round-signing/v1";
 if(round){
  if(legacy&&*legacy!=guard){auto old=signing_proposal_impl(base,chain_id,genesis_hash,producer,height,true);if(bft_proposal_round(*old)==*round&&*legacy!=encoded)throw std::runtime_error("conflicting legacy BFT signature refused");}
  key+="/round/v1/"+height_key(Int(std::to_string(*round)));
 }
 auto previous=get(key);
 if(previous){
  if(*previous!=encoded)throw std::runtime_error("conflicting block signature refused: producer already reserved this slot");
  if(round){auto saved=signing_proposal_impl(key,chain_id,genesis_hash,producer,height,true);if(!legacy||bft_proposal_round(*saved)!=*round)throw std::runtime_error("corrupt BFT signing slot");}
  return;
 }
 if(round&&get(key+"/proposal"))throw std::runtime_error("orphaned BFT signing proposal");
 // Guard and round reservation are durable together. An older height-only
 // writer cannot bypass a round reservation after a software downgrade.
 rocksdb::WriteBatch batch;if(round&&!legacy)batch.Put(base,guard);batch.Put(key,encoded);if(proposal)batch.Put(key+"/proposal",proposal->to_json(true).dump());
 require_ok(db_->Write(durable_write(),&batch));
}
std::optional<Block> ChainStore::signing_proposal(const Int& chain_id,const Bytes& genesis_hash,const std::string& producer,const Int& height,std::optional<uint64_t> round)const{
 const auto base="signing/v1/"+hex(bytes(chain_id,32))+"/"+hex(genesis_hash)+"/"+address(producer)+"/"+height_key(height);
 std::lock_guard lock(mutex_);
 if(!round){if(get(base)==std::optional<std::string>("nano/bft-round-signing/v1"))throw std::runtime_error("round-aware signer required");return signing_proposal_impl(base,chain_id,genesis_hash,producer,height,false);}
 auto legacy=get(base);std::optional<Block> fallback;
 if(legacy&&*legacy!="nano/bft-round-signing/v1"){
  auto old=signing_proposal_impl(base,chain_id,genesis_hash,producer,height,true);if(bft_proposal_round(*old)==*round)fallback=std::move(old);
 }
 const auto slot=base+"/round/v1/"+height_key(Int(std::to_string(*round)));auto stored=signing_proposal_impl(slot,chain_id,genesis_hash,producer,height,get(slot).has_value());
 if(stored&&(!legacy||bft_proposal_round(*stored)!=*round||(fallback&&block_signing_hash(*fallback,chain_id,genesis_hash)!=block_signing_hash(*stored,chain_id,genesis_hash))))throw std::runtime_error("corrupt BFT signing slot");
 return stored?stored:fallback;
}
std::optional<std::string> ChainStore::certified_tip_impl()const{
 auto certified=get("meta/certified-tip");auto tip=get("meta/tip");
 if(certified){
  if(certified->size()!=18||*certified!=height_key(integer(unhex(*certified)))||integer(unhex(*certified))==0||!tip||*tip!=*certified||!get("certificate/"+*certified))throw std::runtime_error("incomplete certified chain metadata");
 }else{
  // A missing marker must never silently re-enable ordinary imports or rewind.
  // Seek is bounded by the index, without scanning the block history.
  std::unique_ptr<rocksdb::Iterator> iterator(db_->NewIterator(rocksdb::ReadOptions()));
  iterator->Seek("certificate/");require_ok(iterator->status());
  if(iterator->Valid()&&iterator->key().starts_with(rocksdb::Slice("certificate/")))throw std::runtime_error("missing certified chain marker");
 }
 return certified;
}
std::optional<BlockCertificate> ChainStore::certificate_impl(const Int& height)const{
 auto record=read("certificate/"+height_key(height));if(!record)return std::nullopt;
 auto body=read("block/"+height_key(height));auto identity=read("meta/identity");
 if(!body||!identity)throw std::runtime_error("certificate lacks canonical block or chain identity");
 auto block=Block::from_json(*body);
 if(block.header.number!=height||quantity(record->at("height"))!=height||record->at("blockHash")!=hex(block.header.hash()))throw std::runtime_error("certificate block index mismatch");
 BlockCertificate result;result.round=u64(quantity(record->at("round")));result.validators=decode_members(record->at("validators"));result.next_validators=decode_members(record->at("nextValidators"));
 const auto& votes=record->at("precommits");if(!votes.is_array()||votes.size()>result.validators.size())throw std::runtime_error("invalid persisted certificate length");
 for(const auto& entry:votes){FinalityVote vote;vote.height=u64(height);vote.round=result.round;vote.step=VoteStep::precommit;vote.block_hash=block.header.hash();vote.validator_set_hash=voting_set_hash(result.validators);vote.validator=entry.at("validator").get<std::string>();vote.signature=unhex(entry.at("signature"));result.votes.push_back(vote);}
 if(!verify_commit_certificate(result.votes,result.validators,u64(height),result.round,block.header.hash(),quantity(identity->at("chainId")),unhex(identity->at("genesisHash"))))throw std::runtime_error("invalid persisted block certificate");
 return result;
}
std::optional<BlockCertificate> ChainStore::certificate_at(const Int& height)const{
 std::lock_guard lock(mutex_);auto certified=certified_tip_impl();auto result=certificate_impl(height);
 if(certified&&height>0&&height<=integer(unhex(*certified))&&!result)throw std::runtime_error("missing canonical block certificate");return result;
}
void ChainStore::rewind(const Int& number){
 std::lock_guard lock(mutex_);if(get("execution-checkpoint/v1"))throw std::invalid_argument("coupled execution requires atomic checkpoint");auto tip=get("meta/tip"),difficulty=get("meta/difficulty"),count=get("meta/count");
 if(!tip||!difficulty||!count)throw std::runtime_error("missing chain metadata");
 auto certified=certified_tip_impl();if(certified&&number<integer(unhex(*certified)))throw std::invalid_argument("cannot rewind certified block");
 Int height=integer(unhex(*tip));if(number<0||number>height)throw std::invalid_argument("invalid rewind height");
 auto target=read("block/"+height_key(number));if(!target||!get("state/"+height_key(number)))throw std::runtime_error("missing rewind state");
 Int total(*difficulty),blocks(*count);rocksdb::WriteBatch batch;
 for(Int cursor=height;cursor>number;--cursor){
  auto key=height_key(cursor);auto block=read("block/"+key);if(!block)throw std::runtime_error("missing rewind block");
  total-=quantity(block->at("difficulty"));--blocks;
  for(const auto& transaction:block->at("transactions")){auto hash=transaction.at("hash").get<std::string>();batch.Delete("tx/"+hash);batch.Delete("receipt/"+hash);}
  batch.Delete("hash/"+block->at("hash").get<std::string>());batch.Delete("block/"+key);batch.Delete("state/"+key);batch.Delete("consensus/"+key);
 }
 batch.Put("meta/tip",height_key(number));batch.Put("meta/difficulty",total.get_str());batch.Put("meta/count",blocks.get_str());require_ok(db_->Write(durable_write(),&batch));
}
void ChainStore::commit(const Block& block,const State& state,const std::vector<Receipt>& receipts,const std::optional<Json>& consensus){commit_impl(block,state,receipts,consensus,nullptr);}
void ChainStore::commit_certified(const Block& block,const State& state,const std::vector<Receipt>& receipts,const Json& consensus,const BlockCertificate& certificate){commit_impl(block,state,receipts,consensus,&certificate);}
void ChainStore::commit_impl(const Block& block,const State& state,const std::vector<Receipt>& receipts,const std::optional<Json>& consensus,const BlockCertificate* certificate){
 if(state.root()!=block.header.state_root)throw std::invalid_argument("stored state root mismatch");
 if(receipts.size()!=block.transactions.size())throw std::invalid_argument("receipt count mismatch");
 if(block.header.number!=0&&(transactions_root(block.transactions)!=block.header.transactions_root||receipts_root(receipts)!=block.header.receipts_root))throw std::invalid_argument("stored transaction/receipt root mismatch");
 auto hash=block.header.hash();Int cumulative=0;std::vector<Log> logs;
 for(size_t i=0;i<receipts.size();++i){const auto& r=receipts[i];cumulative+=r.gas_used;if(r.transaction_hash!=block.transactions[i].hash()||r.block_hash!=hash||r.block_number!=block.header.number||r.index!=Int(i)||r.cumulative_gas_used!=cumulative)throw std::invalid_argument("receipt metadata mismatch");logs.insert(logs.end(),r.logs.begin(),r.logs.end());}
 if(cumulative!=block.header.gas_used||logs_bloom(logs)!=block.header.logs_bloom)throw std::invalid_argument("stored gas/bloom mismatch");
 auto number=height_key(block.header.number);auto block_json=block.to_json(true);block_json["uncleHeaders"]=Json::array();for(const auto& uncle:block.uncles)block_json["uncleHeaders"].push_back(uncle.to_json());auto body=block_json.dump(),state_data=state.to_json().dump();
 std::lock_guard lock(mutex_);
 if(get("execution-checkpoint/v1"))throw std::invalid_argument("coupled execution requires atomic checkpoint");
 auto certified=certified_tip_impl();
 if(certified&&!certificate)throw std::invalid_argument("certified chain requires block certificate");
 if(certificate){
  auto identity=read("meta/identity");
  if(!identity||identity->at("consensus")!="pos"||!consensus||!verify_commit_certificate(certificate->votes,certificate->validators,u64(block.header.number),certificate->round,hash,quantity(identity->at("chainId")),unhex(identity->at("genesisHash"))))throw std::invalid_argument("invalid canonical block certificate");
  voting_set_hash(certificate->next_validators);
  if(certified){
   if(block.header.number!=integer(unhex(*certified))+1)throw std::invalid_argument("certificate sequence mismatch");
   auto parent=certificate_impl(block.header.number-1);
   if(!parent||voting_set_hash(parent->next_validators)!=voting_set_hash(certificate->validators))throw std::invalid_argument("certificate validator continuity mismatch");
  }else if(block.header.number!=1)throw std::invalid_argument("certified chain must start at height one");
 }
 auto tip=get("meta/tip");Int total=block.header.difficulty;size_t previous_count=0;
 if(tip){
  auto previous=read("block/"+*tip);auto difficulty=get("meta/difficulty");auto count=get("meta/count");
  if(!previous||!difficulty||!count)throw std::runtime_error("incomplete chain metadata");
  if(block.header.number!=integer(unhex(*tip))+1||hex(block.header.parent_hash)!=previous->at("hash").get<std::string>())throw std::invalid_argument("block does not extend stored chain");
  total+=Int(*difficulty);previous_count=std::stoull(*count);
 }else if(block.header.number!=0)throw std::invalid_argument("first stored block must be genesis");
 if(get("block/"+number)||get("hash/"+hex(hash)))throw std::invalid_argument("duplicate block");
 rocksdb::WriteBatch batch;
 if(consensus)batch.Put("consensus/"+number,consensus->dump());
 batch.Put("block/"+number,body);batch.Put("state/"+number,state_data);batch.Put("hash/"+hex(hash),number);
 std::set<std::string> seen;
 for(size_t i=0;i<receipts.size();++i){
  auto txhash=hex(block.transactions[i].hash());
  if(!seen.insert(txhash).second||get("tx/"+txhash))throw std::invalid_argument("duplicate transaction");
  auto transaction=block.transactions[i].to_json();transaction["blockHash"]=hex(hash);transaction["blockNumber"]=quantity(block.header.number);transaction["transactionIndex"]=quantity(Int(i));
  batch.Put("tx/"+txhash,transaction.dump());batch.Put("receipt/"+txhash,receipts[i].to_json().dump());
 }
 if(certificate){batch.Put("certificate/"+number,encode_certificate(block,*certificate).dump());batch.Put("meta/certified-tip",number);}
 batch.Put("meta/tip",number);batch.Put("meta/difficulty",total.get_str());batch.Put("meta/count",std::to_string(previous_count+1));
 require_ok(db_->Write(durable_write(),&batch));
}
std::optional<Block> ChainStore::block_by_number(const Int& n)const{std::lock_guard lock(mutex_);auto j=read("block/"+height_key(n));if(!j)return std::nullopt;auto block=Block::from_json(*j);if(block.header.number!=n)throw std::runtime_error("block height index mismatch");return block;}
std::optional<Block> ChainStore::block_by_hash(const Bytes& hash)const{std::lock_guard lock(mutex_);auto number=get("hash/"+hex(hash));if(!number)return std::nullopt;auto j=read("block/"+*number);if(!j)throw std::runtime_error("broken block hash index");auto block=Block::from_json(*j);if(block.header.hash()!=hash||height_key(block.header.number)!=*number)throw std::runtime_error("block hash index mismatch");return block;}
std::optional<Block> ChainStore::latest()const{std::lock_guard lock(mutex_);auto certified=certified_tip_impl();auto number=get("meta/tip");if(!number)return std::nullopt;auto j=read("block/"+*number);if(!j)throw std::runtime_error("missing chain tip");auto block=Block::from_json(*j);if(height_key(block.header.number)!=*number)throw std::runtime_error("chain tip height mismatch");if(certified&&(*certified!=*number||!certificate_impl(block.header.number)))throw std::runtime_error("certified tip mismatch");return block;}
std::optional<State> ChainStore::state_at(const Int& n)const{std::lock_guard lock(mutex_);auto j=read("state/"+height_key(n));if(!j)return std::nullopt;auto state=State::from_json(*j);auto body=read("block/"+height_key(n));if(!body||state.root()!=unhex(body->at("stateRoot")))throw std::runtime_error("corrupted persisted state");return state;}
std::optional<Json> ChainStore::transaction(const Bytes& h)const{std::lock_guard lock(mutex_);return read("tx/"+hex(h));}
std::optional<Receipt> ChainStore::receipt(const Bytes& h)const{std::lock_guard lock(mutex_);auto j=read("receipt/"+hex(h));return j?std::optional<Receipt>(Receipt::from_json(*j)):std::nullopt;}
std::optional<Json> ChainStore::consensus_at(const Int& number)const{std::lock_guard lock(mutex_);return read("consensus/"+height_key(number));}
Int ChainStore::total_difficulty()const{std::lock_guard lock(mutex_);auto value=get("meta/difficulty");return value?Int(*value):Int(0);}
size_t ChainStore::count()const{std::lock_guard lock(mutex_);auto value=get("meta/count");return value?size_t(std::stoull(*value)):0;}
}

namespace nano {
std::optional<ForkCheckpointRecord> ChainStore::fork_checkpoint_impl(const Int& chain_id,const Bytes& genesis_hash,size_t maximum_bytes)const{
 nano_profile::Timer read_timer{"stored-checkpoint-auth-read"};
 if(fork_checkpoint_write_uncertain_)throw std::runtime_error("fork checkpoint write uncertain; reopen required");
 const auto identity=read("meta/identity");
 if(genesis_hash.size()!=32||!identity||identity->at("chainId")!=quantity(chain_id)||identity->at("genesisHash")!=hex(genesis_hash))throw std::invalid_argument("fork checkpoint chain binding mismatch");
 const auto manifest=get("fork-checkpoint/v1/token"),payload=get("fork-checkpoint/v1/payload");
 if(!manifest&&!payload)return std::nullopt;
 if(!manifest||!payload||manifest->size()>256||payload->empty()||payload->size()>maximum_bytes)throw std::runtime_error("incomplete or oversized fork checkpoint");
 const auto j=Json::parse(*manifest);
 if(!j.is_object()||j.size()!=2||!j.at("sequence").is_string()||!j.at("digest").is_string())throw std::runtime_error("invalid fork checkpoint token");
 const auto seq=u64(quantity(j.at("sequence")));const auto& encoded=j.at("digest").get_ref<const std::string&>();
 if(!seq||quantity(Int(std::to_string(seq)))!=j.at("sequence").get<std::string>()||encoded.size()!=66)throw std::runtime_error("noncanonical fork checkpoint token");
 const auto digest=sha256(Bytes(payload->begin(),payload->end()));
 if(hex(digest)!=encoded)throw std::runtime_error("fork checkpoint payload digest mismatch");
 if(const auto linked=read("execution-checkpoint/v1")){
  const auto tip=get("meta/tip");const auto block=tip?read("block/"+*tip):std::nullopt;
  if(!linked->is_object()||linked->size()!=2||!block||linked->at("digest")!=encoded||linked->at("tip")!=block->at("hash")||hex(Block::from_json(*block).header.hash())!=linked->at("tip").get<std::string>())throw std::runtime_error("fork checkpoint execution binding mismatch");
 }
 return ForkCheckpointRecord{{seq,digest},*payload};
}
std::optional<ForkCheckpointRecord> ChainStore::fork_checkpoint(const Int& chain_id,const Bytes& genesis_hash,
 size_t maximum_bytes,const std::optional<ForkCheckpointToken>& expected)const{
 std::lock_guard lock(mutex_);auto record=fork_checkpoint_impl(chain_id,genesis_hash,maximum_bytes);
 if(expected&&(!record||record->token!=*expected))throw std::runtime_error("fork checkpoint differs from external commitment");
 return record;
}
ForkCheckpointToken ChainStore::commit_fork_checkpoint(const Int& chain_id,const Bytes& genesis_hash,
 const std::optional<ForkCheckpointToken>& expected,std::string_view snapshot,size_t maximum_bytes){
 if(snapshot.empty()||snapshot.size()>maximum_bytes)throw std::invalid_argument("fork checkpoint size limit");
 const auto digest=sha256(Bytes(snapshot.begin(),snapshot.end()));
 std::lock_guard lock(mutex_);if(get("execution-checkpoint/v1"))throw std::invalid_argument("coupled checkpoint requires execution binding");const auto previous=fork_checkpoint_impl(chain_id,genesis_hash,maximum_bytes);
 if(bool(previous)!=bool(expected)||(previous&&previous->token!=*expected))throw std::invalid_argument("stale fork checkpoint writer");
 if(previous&&previous->token.sequence==UINT64_MAX)throw std::overflow_error("fork checkpoint sequence exhausted");
 const ForkCheckpointToken token{previous?previous->token.sequence+1:1,digest};
 const Json encoded{{"sequence",quantity(Int(std::to_string(token.sequence)))},{"digest",hex(token.digest)}};
 rocksdb::WriteBatch batch;batch.Put("fork-checkpoint/v1/payload",rocksdb::Slice(snapshot.data(),snapshot.size()));batch.Put("fork-checkpoint/v1/token",encoded.dump());
 const auto status=db_->Write(durable_write(),&batch);if(!status.ok())fork_checkpoint_write_uncertain_=true;
 require_ok(status);return token;
}
}

namespace nano {
ForkCheckpointToken ChainStore::commit_execution_checkpoint(const Int& chain_id,const Bytes& genesis_hash,
 const std::optional<ForkCheckpointToken>& expected,const Bytes& expected_tip,
 const Int& common_height,const Bytes& common_hash,const std::vector<ExecutionCheckpointBlock>& suffix,
 std::string_view snapshot,size_t maximum_bytes,size_t maximum_blocks,const std::vector<CheckpointArchiveEntry>& archive){
 if(snapshot.empty()||snapshot.size()>maximum_bytes||suffix.size()>maximum_blocks||archive.size()>maximum_blocks||common_height<0||common_height>Int("18446744073709551615")||expected_tip.size()!=32||common_hash.size()!=32)
  throw std::invalid_argument("execution checkpoint bounds");
 const auto digest=sha256(Bytes(snapshot.begin(),snapshot.end()));
 std::lock_guard lock(mutex_);const auto previous=fork_checkpoint_impl(chain_id,genesis_hash,maximum_bytes);
 if(bool(previous)!=bool(expected)||(previous&&previous->token!=*expected))throw std::invalid_argument("stale execution checkpoint writer");
 if(previous&&previous->token.sequence==UINT64_MAX)throw std::overflow_error("checkpoint sequence exhausted");
 if(certified_tip_impl())throw std::invalid_argument("cannot replace certified consensus with fork checkpoint");
 const auto tip_key=get("meta/tip"),total_data=get("meta/difficulty"),count_data=get("meta/count");
 if(!tip_key||!total_data||!count_data)throw std::runtime_error("execution checkpoint requires canonical genesis");
 const auto tip=read("block/"+*tip_key),common=read("block/"+height_key(common_height)),genesis=read("block/"+height_key(0));
 if(!tip||!common||!genesis||tip->at("hash")!=hex(expected_tip)||common->at("hash")!=hex(common_hash)||genesis->at("hash")!=hex(genesis_hash))throw std::invalid_argument("execution checkpoint chain CAS mismatch");
 if(Block::from_json(*tip).header.hash()!=expected_tip||Block::from_json(*common).header.hash()!=common_hash||Block::from_json(*genesis).header.hash()!=genesis_hash)throw std::runtime_error("execution checkpoint stored header corrupt");
 const auto height=quantity(tip->at("number"));
 if(height_key(height)!=*tip_key||common_height>height||height-common_height>Int(std::to_string(maximum_blocks)))throw std::invalid_argument("execution checkpoint reorg bounds");
 if(const auto linked=read("execution-checkpoint/v1")){if(!previous||linked->at("digest")!=hex(previous->token.digest)||linked->at("tip")!=hex(expected_tip))throw std::runtime_error("execution checkpoint binding corrupt");}
 const auto common_state=read("state/"+height_key(common_height));if(!common_state||State::from_json(*common_state).root()!=unhex(common->at("stateRoot")))throw std::runtime_error("execution checkpoint ancestor state corrupt");
 Int total(*total_data),count(*count_data);if(count!=height+1)throw std::runtime_error("execution checkpoint count corrupt");
 rocksdb::WriteBatch batch;std::set<std::string> removed,inserted;
 auto bounded=[&]{if(batch.GetDataSize()>maximum_bytes)throw std::length_error("execution checkpoint batch budget");};
 for(Int at=height;at>common_height;--at){
  const auto key=height_key(at);const auto old=read("block/"+key);if(!old||quantity(old->at("number"))!=at)throw std::runtime_error("missing execution checkpoint suffix");
  for(const auto& tx:old->at("transactions")){const auto hash=tx.at("hash").get<std::string>();removed.insert(hash);batch.Delete("tx/"+hash);batch.Delete("receipt/"+hash);}
  batch.Delete("hash/"+old->at("hash").get<std::string>());batch.Delete("block/"+key);batch.Delete("state/"+key);batch.Delete("consensus/"+key);total-=quantity(old->at("difficulty"));--count;bounded();
 }
 // History survives a canonical reorg. Conflicting bytes under an existing
 // consensus root are never replaced, including duplicates within one batch.
 std::map<std::string,std::string> archived;
 for(const auto& entry:archive){
  if(entry.root.size()!=32||entry.payload.empty()||entry.payload.size()>maximum_bytes)
   throw std::invalid_argument("checkpoint archive bounds");
  const auto key="checkpoint-archive/v1/"+hex(entry.root);
  const auto encoded=Json{{"slot",quantity(Int(std::to_string(entry.slot)))},{"payload",hex(entry.payload)},{"digest",hex(sha256(entry.payload))}}.dump();
  if(encoded.size()>maximum_bytes)throw std::length_error("checkpoint archive record budget");
  const auto [it,added]=archived.emplace(key,encoded);
  if(!added){if(it->second!=encoded)throw std::invalid_argument("conflicting checkpoint archive duplicate");continue;}
  const auto old=get(key);
  if(old){if(*old!=encoded)throw std::invalid_argument("conflicting checkpoint archive record");continue;}
  batch.Put(key,encoded);
  batch.Put("checkpoint-archive-slot/v1/"+height_key(Int(std::to_string(entry.slot)))+"/"+hex(entry.root),"");bounded();
 }
 auto parent=Block::from_json(*common).header;
 for(const auto& record:suffix){
  const auto& block=record.block;const auto& h=block.header;const auto& receipts=record.receipts;const auto hash=h.hash();
  if(h.number<0||h.number>Int("18446744073709551615")||h.number!=parent.number+1||h.parent_hash!=parent.hash()||record.state.root()!=h.state_root||receipts.size()!=block.transactions.size()||transactions_root(block.transactions)!=h.transactions_root||receipts_root(receipts)!=h.receipts_root)throw std::invalid_argument("execution checkpoint candidate binding");
  if(!block.withdrawals.is_array()||(h.withdrawals_root?*h.withdrawals_root!=withdrawals_root(block.withdrawals):!block.withdrawals.empty()))throw std::invalid_argument("execution checkpoint withdrawal binding");
  Int cumulative=0;std::vector<Log> logs;
  for(size_t i=0;i<receipts.size();++i){const auto& receipt=receipts[i];cumulative+=receipt.gas_used;logs.insert(logs.end(),receipt.logs.begin(),receipt.logs.end());if(receipt.gas_used<0||receipt.transaction_hash!=block.transactions[i].hash()||receipt.block_hash!=hash||receipt.block_number!=h.number||receipt.index!=Int(i)||receipt.cumulative_gas_used!=cumulative)throw std::invalid_argument("execution checkpoint receipt binding");}
  if(cumulative!=h.gas_used||logs_bloom(logs)!=h.logs_bloom)throw std::invalid_argument("execution checkpoint gas/bloom binding");
  const auto key=height_key(h.number);auto body=block.to_json(true);body["uncleHeaders"]=Json::array();for(const auto& uncle:block.uncles)body["uncleHeaders"].push_back(uncle.to_json());
  batch.Put("block/"+key,body.dump());batch.Put("state/"+key,record.state.to_json().dump());batch.Put("hash/"+hex(hash),key);
  for(size_t i=0;i<receipts.size();++i){const auto txhash=hex(block.transactions[i].hash());if(!inserted.insert(txhash).second||(get("tx/"+txhash)&&!removed.contains(txhash)))throw std::invalid_argument("execution checkpoint duplicate transaction");auto tx=block.transactions[i].to_json();tx["blockHash"]=hex(hash);tx["blockNumber"]=quantity(h.number);tx["transactionIndex"]=quantity(Int(i));batch.Put("tx/"+txhash,tx.dump());batch.Put("receipt/"+txhash,receipts[i].to_json().dump());}
  total+=h.difficulty;++count;parent=h;bounded();
 }
 const ForkCheckpointToken token{previous?previous->token.sequence+1:1,digest};
 batch.Put("meta/tip",height_key(parent.number));batch.Put("meta/difficulty",total.get_str());batch.Put("meta/count",count.get_str());
 batch.Put("fork-checkpoint/v1/payload",rocksdb::Slice(snapshot.data(),snapshot.size()));batch.Put("fork-checkpoint/v1/token",Json{{"sequence",quantity(Int(std::to_string(token.sequence)))},{"digest",hex(digest)}}.dump());
 batch.Put("execution-checkpoint/v1",Json{{"digest",hex(digest)},{"tip",hex(parent.hash())}}.dump());bounded();
 const auto status=[&]{nano_profile::Timer timer{"execution-db-sync-write"};return db_->Write(durable_write(),&batch);}();if(!status.ok())fork_checkpoint_write_uncertain_=true;require_ok(status);return token;
}
}

namespace nano {
std::optional<CheckpointArchiveEntry> ChainStore::checkpoint_archive(const Bytes& root,size_t maximum_bytes)const{
 if(root.size()!=32)throw std::invalid_argument("checkpoint archive root");
 std::lock_guard lock(mutex_);const auto encoded=get("checkpoint-archive/v1/"+hex(root));
 if(!encoded)return std::nullopt;
 if(encoded->size()>maximum_bytes)throw std::length_error("checkpoint archive read budget");
 const auto data=Json::parse(*encoded);
 if(!data.is_object()||data.size()!=3)throw std::runtime_error("checkpoint archive encoding");
 const auto slot=quantity(data.at("slot"));
 if(slot<0||slot>Int("18446744073709551615"))throw std::runtime_error("checkpoint archive slot");
 auto payload=unhex(data.at("payload").get<std::string>());
 if(payload.empty()||hex(sha256(payload))!=data.at("digest").get<std::string>())throw std::runtime_error("checkpoint archive digest");
 const auto canonical=Json{{"slot",quantity(slot)},{"payload",hex(payload)},{"digest",hex(sha256(payload))}}.dump();
 if(canonical!=*encoded)throw std::runtime_error("checkpoint archive noncanonical encoding");
 return CheckpointArchiveEntry{root,u64(slot),std::move(payload)};
}
}
