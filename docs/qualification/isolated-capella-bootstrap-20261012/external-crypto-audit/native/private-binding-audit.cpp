#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_beacon_body.hpp"
#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_deposit.hpp"
#include "nano/consensus_capella_bls_change.hpp"
#include "nano/core.hpp"
#include "blst.h"
#include <openssl/crypto.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <set>
#include <array>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

// Independent integration checker: no key generation, signature creation,
// signer callback, node, RPC, or network API is called by this executable.
namespace fs=std::filesystem;
using namespace nano;
namespace {
const char* stage="arguments";
void require(bool value){if(!value)throw std::runtime_error("audit criterion refused");}
struct Fd {int value;explicit Fd(int v):value(v){require(v>=0);}~Fd(){::close(value);}Fd(const Fd&)=delete;};
struct SecretString {std::string value;~SecretString(){OPENSSL_cleanse(value.data(),value.size());}};
struct SecretBytes {Bytes value;~SecretBytes(){OPENSSL_cleanse(value.data(),value.size());}};
void wipe_json(Json& j){if(j.is_string()){auto& s=j.get_ref<std::string&>();OPENSSL_cleanse(s.data(),s.size());}else if(j.is_array()||j.is_object())for(auto& v:j)wipe_json(v);}
struct PrivateJson {Json value;~PrivateJson(){wipe_json(value);}};
struct Scalar {blst_scalar value{};~Scalar(){OPENSSL_cleanse(&value,sizeof(value));}};
bool same_stat(const struct stat& a,const struct stat& b){
 const bool common=a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_size==b.st_size&&a.st_uid==b.st_uid&&a.st_mode==b.st_mode;
#ifdef __APPLE__
 return common&&a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec&&a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return common&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
void no_symlink_components(const fs::path& path){
 require(path.is_absolute()&&path==path.lexically_normal());fs::path current;
 for(const auto& part:path){current/=part;struct stat s{};require(::lstat(current.c_str(),&s)==0&&!S_ISLNK(s.st_mode));}
}
Bytes read_at(int directory,const std::string& name,size_t maximum,bool private_file=false){
 require(!name.empty()&&name.find('/')==std::string::npos);
 Fd file(::openat(directory,name.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));struct stat before{},after{};
 require(::fstat(file.value,&before)==0&&S_ISREG(before.st_mode)&&before.st_size>=0&&uint64_t(before.st_size)<=maximum);
 if(private_file)require(before.st_uid==::geteuid()&&(before.st_mode&07777)==0600);
 Bytes out(size_t(before.st_size));size_t at=0;
 while(at<out.size()){const auto n=::read(file.value,out.data()+at,out.size()-at);if(n<0&&errno==EINTR)continue;require(n>0);at+=size_t(n);}
 uint8_t extra=0;ssize_t extra_count;do{extra_count=::read(file.value,&extra,1);}while(extra_count<0&&errno==EINTR);
 require(extra_count==0&&::fstat(file.value,&after)==0&&same_stat(before,after));return out;
}
Json parse(const Bytes& bytes){
 // Do not print exceptions: parser messages may include private input bytes.
 std::vector<std::set<std::string>> keys;
 return Json::parse(bytes.begin(),bytes.end(),[&](int depth,Json::parse_event_t event,Json& value){
  require(depth<=32);if(event==Json::parse_event_t::object_start)keys.emplace_back();
  else if(event==Json::parse_event_t::object_end)keys.pop_back();
  else if(event==Json::parse_event_t::key)require(keys.back().insert(value.get<std::string>()).second);return true;
 });
}
Bytes encoded(const Json& value,size_t width){require(value.is_string());const auto& text=value.get_ref<const std::string&>();require(text.size()==2+2*width&&text.starts_with("0x"));auto out=unhex(text);require(out.size()==width&&hex(out)==text);return out;}
template<size_t N>std::array<uint8_t,N> fixed(const Bytes& b,size_t at=0){require(at<=b.size()&&N<=b.size()-at);std::array<uint8_t,N> out{};std::copy_n(b.begin()+at,N,out.begin());return out;}
ConsensusRoot root(const Json& value){return fixed<32>(encoded(value,32));}
std::string text_root(const ConsensusRoot& value){return hex(Bytes(value.begin(),value.end()));}
uint64_t little(const Bytes& b,size_t at,size_t width=8){require(at<=b.size()&&width<=b.size()-at&&width<=8);uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
Bytes slice(const Bytes& b,size_t at,size_t width){require(at<=b.size()&&width<=b.size()-at);return Bytes(b.begin()+at,b.begin()+at+width);}
Json token(const ForkCheckpointToken& value){return Json{{"sequence",quantity(Int(std::to_string(value.sequence)))},{"digest",hex(value.digest)}};}
void new_directory(const fs::path& path){require(!fs::exists(path)&&!fs::is_symlink(path));no_symlink_components(path.parent_path());require(::mkdir(path.c_str(),0700)==0);}
void save_new(const fs::path& path,const Json& value){
 const auto data=value.dump();Fd file(::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));size_t at=0;
 while(at<data.size()){const auto n=::write(file.value,data.data()+at,data.size()-at);if(n<0&&errno==EINTR)continue;require(n>0);at+=size_t(n);}
 require(::fsync(file.value)==0);Fd directory(::open(path.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));require(::fsync(directory.value)==0);
}
}
int main(int argc,char** argv){
 try{
  require(argc==2||argc==3);const fs::path directory(argv[1]);no_symlink_components(directory);
  Fd dir(::open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));struct stat ds{};require(::fstat(dir.value,&ds)==0&&ds.st_uid==::geteuid()&&(ds.st_mode&07777)==0700);
  stage="public-file-bindings";const auto manifest_bytes=read_at(dir.value,"isolated-domain.json",4*1024*1024);const auto manifest=parse(manifest_bytes);
  require(manifest.at("format")=="nano-isolated-capella-fixture-domain-v1"&&manifest.at("productionQualified")==false);
  require(manifest.at("publicKeys").size()==65&&manifest.at("withdrawalPublicKeys").size()==64);
  std::map<std::string,Bytes> files;for(const auto& [name,desc]:manifest.at("files").items()){
   const auto path=desc.at("path").get<std::string>();auto bytes=read_at(dir.value,path,16*1024*1024);
   require(desc.at("bytes").get<size_t>()==bytes.size()&&hex(sha256(bytes)).substr(2)==desc.at("sha256").get<std::string>());files[name]=std::move(bytes);
  }
  require(files.size()==5);const auto& profile_bytes=files.at("profile");const auto rules=load_capella_fork_profile(std::string(profile_bytes.begin(),profile_bytes.end()),sha256(profile_bytes),profile_bytes.size());const auto& r=rules.transition;const auto& l=r.consensus.state;
  require(r.chain_id==quantity(manifest.at("chainId")));const auto slots=r.consensus.epoch.ffg.slots_per_epoch;
  const auto& genesis_state=files.at("genesisState");const auto& registry=files.at("genesisValidators");
  require(registry.size()==64*121&&little(genesis_state,40)==0&&fixed<32>(genesis_state,8)==root(manifest.at("genesisValidatorsRoot")));
  require(capella_beacon_state_root(genesis_state,l,r.execution,r.maximum_state_bytes)==root(manifest.at("genesisStateRoot")));
  std::array<Bytes,65> public_keys;std::array<Bytes,64> withdrawals;std::set<Bytes> unique;
  stage="public-bls-key-group-membership";
  for(size_t i=0;i<65;++i){const auto& item=manifest.at("publicKeys").at(i);require(item.at("index").get<size_t>()==i);public_keys[i]=encoded(item.at("publicKey"),48);require(unique.insert(public_keys[i]).second&&aggregate_consensus_bls_keys(std::vector<Bytes>{public_keys[i]}).has_value());}
  for(size_t i=0;i<64;++i){const auto& item=manifest.at("withdrawalPublicKeys").at(i);require(item.at("index").get<size_t>()==i);withdrawals[i]=encoded(item.at("publicKey"),48);require(unique.insert(withdrawals[i]).second&&aggregate_consensus_bls_keys(std::vector<Bytes>{withdrawals[i]}).has_value());auto credentials=sha256(withdrawals[i]);credentials[0]=0;require(slice(registry,121*i,48)==public_keys[i]&&slice(registry,121*i+48,32)==credentials);}
  stage="private-env-scalar-public-bindings";std::set<size_t> private_indices;std::array<std::set<size_t>,2> by_env;
  for(size_t n=0;n<2;++n){SecretBytes raw{read_at(dir.value,"env-"+std::to_string(n)+".json",65536,true)};PrivateJson env{parse(raw.value)};require(env.value.is_object());
   for(const auto& [name,value]:env.value.items()){
    if(name=="NANO_TEST_NETWORK_TRACE"){require(value=="1");continue;}const std::string prefix="NANO_TEST_CAPELLA_NETWORK_KEY_";require(name.starts_with(prefix));const auto suffix=name.substr(prefix.size());require(!suffix.empty()&&suffix.size()<=2&&std::all_of(suffix.begin(),suffix.end(),[](char c){return c>='0'&&c<='9';}));const size_t index=std::stoul(suffix);require(index<65&&std::to_string(index)==suffix&&private_indices.insert(index).second);by_env[n].insert(index);
    SecretString scalar_text{value.get<std::string>()};require(scalar_text.value.size()==66&&scalar_text.value.starts_with("0x"));SecretBytes scalar_bytes{unhex(scalar_text.value)};require(scalar_bytes.value.size()==32&&hex(scalar_bytes.value)==scalar_text.value);Scalar scalar;blst_scalar_from_bendian(&scalar.value,scalar_bytes.value.data());require(blst_sk_check(&scalar.value));std::array<uint8_t,32> roundtrip{};blst_bendian_from_scalar(roundtrip.data(),&scalar.value);const bool roundtrip_ok=std::equal(roundtrip.begin(),roundtrip.end(),scalar_bytes.value.begin());OPENSSL_cleanse(roundtrip.data(),roundtrip.size());require(roundtrip_ok);
    blst_p1 point;blst_sk_to_pk_in_g1(&point,&scalar.value);Bytes actual(48);blst_p1_compress(actual.data(),&point);require(actual==public_keys[index]);
   }
  }
  require(private_indices.size()==65);stage="exported-config-key-ownership";
  for(size_t n=0;n<2;++n){const auto node=parse(read_at(dir.value,"node-"+std::to_string(n)+".json",4*1024*1024));std::set<size_t> configured;
   for(const auto& k:node.at("capella").at("validator").at("keys")){const auto name=k.at("keyEnv").get<std::string>();const std::string prefix="NANO_TEST_CAPELLA_NETWORK_KEY_";require(name.starts_with(prefix));const size_t index=std::stoul(name.substr(prefix.size()));require(index<65&&configured.insert(index).second&&by_env[n].contains(index));if(index==64)require(k.at("index").is_null());else require(u64(quantity(k.at("index")))==index);require(k.at("restart").is_null());}
   require(configured==by_env[n]);
  }
  stage="sync-committee-aggregate-integration";const size_t historical=176+64*l.slots_per_historical_root;const size_t previous=historical+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;const size_t current=previous+133;const size_t width=48*(l.sync_committee_size+1);
  for(const auto offset:{current,current+width}){std::vector<Bytes> keys;for(size_t i=0;i<l.sync_committee_size;++i){auto key=slice(genesis_state,offset+48*i,48);require(std::find(public_keys.begin(),public_keys.begin()+64,key)!=public_keys.begin()+64);keys.push_back(std::move(key));}const auto aggregate=aggregate_consensus_bls_keys(keys);require(aggregate&&*aggregate==slice(genesis_state,offset+48*l.sync_committee_size,48));}
  stage="bootstrap-wire-and-signing-roots";const auto& history=manifest.at("bootstrapSigningHistory");const auto wire=unhex(history.at("block").at("ssz").get<std::string>());require(wire.size()>=184&&little(wire,0,4)==100);const auto message=slice(wire,100,wire.size()-100);require(little(message,80,4)==84);const auto body=slice(message,84,message.size()-84);
  const ConsensusBeaconHeader header{little(message,0),little(message,8),fixed<32>(message,16),fixed<32>(message,48),capella_beacon_body_root(body,r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes)};
  require(header.slot==1&&header.proposer_index<64&&history.at("block").at("validatorIndex").get<uint64_t>()==header.proposer_index&&header.state_root==root(manifest.at("initialStateRoot"))&&consensus_beacon_header_root(header)==root(manifest.at("initialHeadRoot")));
  const auto genesis_root=root(manifest.at("genesisValidatorsRoot"));const auto version=fixed<4>(genesis_state,little(genesis_state,56)>0?48:52);
  const auto proposal_root=consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},version,genesis_root));require(proposal_root==root(history.at("block").at("signingRoot"))&&slice(wire,4,96)==encoded(history.at("block").at("signature"),96));
  stage="bootstrap-proposal-signature";require(verify_consensus_bls(public_keys[header.proposer_index],proposal_root,slice(wire,4,96)));
  const auto randao_root=consensus_signing_root(ConsensusRoot{},consensus_domain({2,0,0,0},version,genesis_root));stage="bootstrap-randao-signature";require(history.at("randao").at("epoch")==0&&history.at("randao").at("validatorIndex").get<uint64_t>()==header.proposer_index&&randao_root==root(history.at("randao").at("signingRoot"))&&slice(body,0,96)==encoded(history.at("randao").at("signature"),96)&&verify_consensus_bls(public_keys[header.proposer_index],randao_root,slice(body,0,96)));
  const size_t sync_bytes=(l.sync_committee_size+7)/8;const auto changes_offset=little(body,320+sync_bytes,4);require(changes_offset<=body.size()&&body.size()-changes_offset==172);const auto change=decode_capella_bls_change(slice(body,changes_offset,172));require(change&&change->validator_index==0&&Bytes(change->from_bls_pubkey.begin(),change->from_bls_pubkey.end())==withdrawals[0]);const auto change_root=consensus_signing_root(capella_bls_change_root(*change),consensus_domain({10,0,0,0},r.genesis_version,genesis_root));stage="bootstrap-withdrawal-change-signature";require(change_root==root(history.at("blsToExecutionChange").at("signingRoot"))&&encoded(history.at("blsToExecutionChange").at("publicKey"),48)==withdrawals[0]&&encoded(history.at("blsToExecutionChange").at("signature"),96)==Bytes(change->signature.begin(),change->signature.end())&&verify_consensus_bls(withdrawals[0],change_root,change->signature));
  stage="bootstrap-deposit-signature";const auto& deposit=history.at("deposit");ConsensusDepositMessage deposit_message{fixed<48>(encoded(deposit.at("publicKey"),48)),root(deposit.at("withdrawalCredentials")),deposit.at("amountGwei").get<uint64_t>()};const auto deposit_root=consensus_signing_root(consensus_deposit_root(deposit_message),consensus_domain({3,0,0,0},r.genesis_version,ConsensusRoot{}));const auto deposit_signature=encoded(deposit.at("signature"),96);require(deposit.at("validatorIndex")==64&&encoded(deposit.at("publicKey"),48)==public_keys[64]&&deposit_root==root(deposit.at("signingRoot"))&&verify_consensus_deposit_signature(deposit_message,deposit_signature,r.genesis_version));
  stage="deposit-transaction-ecdsa-sender";const auto deposit_json=parse(read_at(dir.value,"deposit.json",4*1024*1024));const auto raw=unhex(deposit_json.at("raw").get<std::string>());const auto signed_tx=SignedTransaction::decode(raw);require(signed_tx.raw()==raw&&signed_tx.tx.type==2&&signed_tx.tx.chain_id==r.chain_id&&signed_tx.tx.to&&*signed_tx.tx.to=="0x00000000219ab540356cbb839cbe05303d7705fa"&&signed_tx.tx.value==Int(std::to_string(deposit_message.amount_gwei))*1000000000&&hex(signed_tx.hash())==deposit_json.at("hash").get<std::string>());
  const auto sender_key=recover_public_key(signed_tx.tx.signing_hash(),{unsigned(u64(signed_tx.v)),signed_tx.r,signed_tx.s});const auto sender=address(signed_tx.sender());require(sender_key==encoded(manifest.at("transactionSender").at("publicKey"),64)&&sender==manifest.at("transactionSender").at("address").get<std::string>()&&sender==address(deposit_json.at("sender").get<std::string>())&&sender==address(public_address(sender_key)));
  const std::string method="deposit(bytes,bytes,bytes,bytes32)";auto expected_call=keccak(Bytes(method.begin(),method.end()));expected_call.resize(4);Bytes args(416);auto word=[&](size_t offset,size_t value){const auto b=bytes(Int(value),32);std::copy(b.begin(),b.end(),args.begin()+offset);};word(0,128);word(32,224);word(64,288);
  // DepositData root includes the signature; use the qualified Phase0 helper.
  Phase0Deposit deposit_data{};deposit_data.message=deposit_message;std::copy(deposit_signature.begin(),deposit_signature.end(),deposit_data.signature.begin());const auto actual_data_root=consensus_deposit_data_root(deposit_data);std::copy(actual_data_root.begin(),actual_data_root.end(),args.begin()+96);word(128,48);std::copy(public_keys[64].begin(),public_keys[64].end(),args.begin()+160);word(224,32);std::copy(deposit_message.withdrawal_credentials.begin(),deposit_message.withdrawal_credentials.end(),args.begin()+256);word(288,96);std::copy(deposit_signature.begin(),deposit_signature.end(),args.begin()+320);expected_call.insert(expected_call.end(),args.begin(),args.end());require(signed_tx.tx.data==expected_call);
  const auto genesis=parse(files.at("genesis"));const auto funded=State::from_json(genesis.at("alloc"));require(funded.get(sender).balance>=signed_tx.tx.value+signed_tx.tx.gas*signed_tx.tx.max_fee);
  Json report{{"format","nano-isolated-fixture-private-binding-audit-v1"},{"passed",true},{"inputManifestSha256",hex(sha256(manifest_bytes)).substr(2)},{"campaignId",manifest.at("campaignId")},{"privateEnvFiles",2},{"privateEnvOwner0600Verified",true},{"validatorScalarsBigEndianRoundTrip",65},{"validatorPrivatePublicBindings",65},{"configKeyBindings",65},{"duplicateActiveExportOwnership",false},{"blsPublicKeysGroupChecked",129},{"withdrawalCredentialsChecked",64},{"syncAggregatesChecked",2},{"bootstrapSigningRootsRecomputed",4},{"bootstrapBlsSignaturesVerified",4},{"depositValidatorIndex",64},{"depositRawEcdsaSenderVerified",true},{"senderPrivateScalarAvailable",false},{"withdrawalPrivateScalarsAvailable",false},{"seedRead",false},{"privateInputsCopied",false},{"newSignaturesCreated",0},{"signerCallbacksInvoked",0},{"runtimeStarted",false},{"networkLaunched",false},{"productionQualified",false}};
  if(argc==3){
   stage="offline-bootstrap-journal-import";const fs::path output(argv[2]);require(output.is_absolute()&&output==output.lexically_normal());new_directory(output);Json entries=Json::array();
   for(size_t i=0;i<65;++i){Json blocks=Json::array();if(i==header.proposer_index)blocks.push_back({{"slot","1"},{"signing_root",text_root(proposal_root)}});entries.push_back({{"pubkey",hex(public_keys[i])},{"signed_blocks",blocks},{"signed_attestations",Json::array()}});}
   const Json interchange{{"metadata",{{"interchange_format_version","5"},{"genesis_validators_root",text_root(genesis_root)}}},{"data",entries}};save_new(output/"bootstrap-interchange.json",interchange);
   Json journals=Json::array();const auto execution_genesis=encoded(manifest.at("executionGenesisHash"),32);
   for(size_t i=0;i<65;++i){const auto primary=output/("primary-"+std::to_string(i)),witness=output/("witness-"+std::to_string(i));new_directory(primary);new_directory(witness);ForkCheckpointToken commitment{0,{}},witness_commitment{0,{}};
    {ChainStore primary_store(primary),witness_store(witness);BeaconSigningJournal journal(primary_store,r.chain_id,execution_genesis,genesis_root,public_keys[i],slots,std::nullopt,&witness_store);require(journal.import_interchange(interchange.dump(),1024*1024)==1);require(!journal.is_retired()&&journal.latest_attestation_target()==std::nullopt&&journal.latest_proposal_slot()==(i==header.proposer_index?std::optional<uint64_t>(1):std::nullopt));commitment=journal.commitment();const auto recovered=witness_store.fork_checkpoint(r.chain_id,execution_genesis,16384);require(bool(recovered));witness_commitment=recovered->token;}
    {ChainStore primary_store(primary),witness_store(witness);BeaconSigningJournal reopened(primary_store,r.chain_id,execution_genesis,genesis_root,public_keys[i],slots,commitment,&witness_store);require(reopened.commitment()==commitment&&reopened.latest_proposal_slot()==(i==header.proposer_index?std::optional<uint64_t>(1):std::nullopt)&&!reopened.latest_attestation_target());const auto recovered=witness_store.fork_checkpoint(r.chain_id,execution_genesis,16384,witness_commitment);require(bool(recovered));}
    journals.push_back({{"validatorIndex",i},{"publicKey",hex(public_keys[i])},{"primary",primary.string()},{"witness",witness.string()},{"primaryCommitment",token(commitment)},{"witnessCommitment",token(witness_commitment)},{"proposalSlot",i==header.proposer_index?Json(1):Json(nullptr)},{"attestationTarget",nullptr}});
   }
   const Json journal_manifest{{"format","nano-isolated-bootstrap-journal-bindings-v1"},{"campaignId",manifest.at("campaignId")},{"inputManifestSha256",report.at("inputManifestSha256")},{"chainId",manifest.at("chainId")},{"executionGenesisHash",manifest.at("executionGenesisHash")},{"genesisValidatorsRoot",manifest.at("genesisValidatorsRoot")},{"slotsPerEpoch",slots},{"proposerIndex",header.proposer_index},{"maximumPresignedProposalSlot",1},{"minimumNewProposalSlot",2},{"journals",journals},{"signatureCallbacksInvoked",0},{"newSignaturesCreated",0},{"runtimeStarted",false},{"productionQualified",false}};
   save_new(output/"journal-bindings.json",journal_manifest);report["offlineJournalPrimariesCreated"]=65;report["offlineJournalWitnessesCreated"]=65;report["offlineJournalPairsReopened"]=65;report["bootstrapProposalImports"]=1;const auto serialized_bindings=journal_manifest.dump();report["journalBindingsSha256"]=hex(sha256(Bytes(serialized_bindings.begin(),serialized_bindings.end()))).substr(2);
  }
  stage="completed";std::cout<<report.dump()<<'\n';return 0;
 }catch(...){std::cout<<Json{{"format","nano-isolated-fixture-private-binding-audit-v1"},{"passed",false},{"refusedStage",stage},{"errorDetailsSuppressed",true},{"newSignaturesCreated",0},{"signerCallbacksInvoked",0},{"runtimeStarted",false},{"networkLaunched",false},{"productionQualified",false}}.dump()<<'\n';return 1;}
}
