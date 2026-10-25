#pragma once
// Component fixture: real EVM/contract histories, synthetic Beacon envelopes.
// The two public BLS unit keys/signatures are exactly those of test_deposit_history.
// No operator identity, private seed, signer journal or network campaign is read.
#include "capella_stream_signed_fixture.hpp"
#include "nano/consensus_capella_deposits.hpp"
#include "nano/storage_memory_policy.hpp"
#include <openssl/evp.h>
#include <future>
#include <atomic>
#include <thread>
#include <tuple>

namespace nano::test::deposit_context_fixture {
namespace f=nano::test::canonical_stream_fixture;
namespace fs=std::filesystem;
inline constexpr size_t execution_blocks=96,nonzero_deposits=32,warm_records=8;
inline constexpr const char* execution_input="tests/fixtures/capella-bootstrap-anchor-cb532/genesis.json";
inline constexpr const char* execution_input_sha="3373c76de09192ad440625613db97c17b50fbfeddb5adb3190c8f0eb83a22005";
inline constexpr size_t execution_input_bytes=15996;
inline constexpr ChainStoreMemoryPolicy memory_policy{4*1024*1024,2,16*1024*1024,0};
inline uint64_t fixed_deposit_signatures=0;

inline Phase0Deposit public_deposit(unsigned seed){
 f::checkpoint();f::check(seed==1||seed==2,"existing public unit deposit key only");
 Phase0Deposit d{};std::array<uint8_t,32> entropy{};entropy[0]=uint8_t(seed);blst_scalar sk{};
 blst_keygen(&sk,entropy.data(),entropy.size(),nullptr,0);blst_p1 pk;blst_sk_to_pk_in_g1(&pk,&sk);blst_p1_compress(d.message.public_key.data(),&pk);
 d.message.withdrawal_credentials[0]=1;d.message.withdrawal_credentials[31]=uint8_t(seed);d.message.amount_gwei=32000000000;
 const auto message=consensus_signing_root(consensus_deposit_root(d.message),consensus_domain({3,0,0,0},{},{}));
 static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;
 blst_hash_to_g2(&h,message.data(),message.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&sk);blst_p2_compress(d.signature.data(),&s);
 OPENSSL_cleanse(&sk,sizeof(sk));OPENSSL_cleanse(&h,sizeof(h));OPENSSL_cleanse(&s,sizeof(s));++fixed_deposit_signatures;
 f::check(verify_consensus_deposit_signature(d.message,d.signature,{}),"fixed unit deposit signature verifies");return d;
}
inline Bytes calldata(const Phase0Deposit& d){
 const std::string signature="deposit(bytes,bytes,bytes,bytes32)";auto out=keccak(Bytes(signature.begin(),signature.end()));out.resize(4);Bytes args(416);
 auto word=[&](size_t at,size_t n){const auto value=bytes(Int(n),32);std::copy(value.begin(),value.end(),args.begin()+at);};
 word(0,128);word(32,224);word(64,288);const auto leaf=consensus_deposit_data_root(d);std::copy(leaf.begin(),leaf.end(),args.begin()+96);
 word(128,48);std::copy(d.message.public_key.begin(),d.message.public_key.end(),args.begin()+160);word(224,32);std::copy(d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end(),args.begin()+256);word(288,96);std::copy(d.signature.begin(),d.signature.end(),args.begin()+320);out.insert(out.end(),args.begin(),args.end());return out;
}
inline Bytes vote(const ExecutionDepositHistory& history){const auto root=history.root();Bytes value(root.begin(),root.end());for(unsigned n=0;n<8;++n)value.push_back(uint8_t(history.count()>>(8*n)));value.insert(value.end(),history.execution_hash().begin(),history.execution_hash().end());return value;}
inline Bytes advanced(std::span<const uint8_t> selected,uint64_t consumed=0,std::span<const uint8_t> repeated={}){
 f::check(selected.size()==72&&(repeated.empty()||repeated.size()==72),"fixed selected vote width");Bytes out(272);std::copy(selected.begin(),selected.end(),out.begin()+180);
 auto put=[&](size_t at,uint64_t n,size_t width){for(size_t j=0;j<width;++j)out[at+j]=uint8_t(n>>(8*j));};
 put(252,272,4);put(256,consumed,8);put(264,272+repeated.size(),4);out.insert(out.end(),repeated.begin(),repeated.end());return out;
}
inline Bytes encoded_deposit(const Phase0Deposit& d){Bytes out;out.reserve(1240);for(const auto& sibling:d.proof)out.insert(out.end(),sibling.begin(),sibling.end());out.insert(out.end(),d.message.public_key.begin(),d.message.public_key.end());out.insert(out.end(),d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end());for(unsigned j=0;j<8;++j)out.push_back(uint8_t(d.message.amount_gwei>>(8*j)));out.insert(out.end(),d.signature.begin(),d.signature.end());return out;}
inline void same_proof(const Phase0Deposit& actual,const Phase0Deposit& expected){f::check(encoded_deposit(actual)==encoded_deposit(expected),"all33 proof roots and deposit bytes equal memory oracle");}
template<class Operation>inline void rejects(Operation operation,const char* label){bool refused=false;try{operation();}catch(const std::invalid_argument&){refused=true;}catch(const std::out_of_range&){refused=true;}catch(const std::length_error&){refused=true;}catch(const std::runtime_error&){refused=true;}f::check(refused,label);}

struct Fixture {
 State initial;
 Bytes code_hash;
 std::string sender;
 std::array<Phase0Deposit,2> deposits{};
 std::vector<Block> blocks;
 std::vector<State> states;
 std::vector<std::vector<Receipt>> receipts;
 std::vector<ExecutionDepositHistory> histories;
 std::vector<ConsensusRoot> ids;
 CapellaForkStore full;
 CapellaForkBlockRules rules{};
 // Both have slot/count equal to canonical block10. The first also has the
 // exact same deposit root; execution extraData changes its execution hash.
 std::optional<CapellaForkBlock> same_root_sibling,different_root_sibling;

 Fixture(){
  f::test_stage="nonzero-unit-contract-fixture";
  deposits={public_deposit(1),public_deposit(2)};
  const auto text=f::read(fs::path(NANO_TEST_SOURCE)/execution_input);f::check(text.size()==execution_input_bytes&&hex(sha256(f::bytes_of(text))).substr(2)==execution_input_sha,"literal public execution fixture pin");
  // The existing bootstrap fixture already embeds the Ethereum deposit
  // contract and its zero-hash storage. No new multi-megabyte JSON input is
  // required just to obtain that same public code and empty tree.
  const auto genesis=Genesis::from_json(Json::parse(text));initial.get(ethereum_deposit_contract)=genesis.state.get(ethereum_deposit_contract);code_hash=keccak(initial.get(ethereum_deposit_contract).code);
  f::check(hex(code_hash)=="0x6c029a231254fadb724d63be769f75eedd66362df034a3e663252b49d062a666","unchanged public Ethereum deposit contract");
  Transaction sample;sample.chain_id=1337;sample.to=ethereum_deposit_contract;sample.gas=1000000;sample.gas_price=10;
  const auto secret=bytes(Int(1),32);sender=SignedTransaction::sign(sample,secret).sender();initial.get(sender).balance=Int("1000000000000000000000000");
  Block anchor;anchor.header.difficulty=0;anchor.header.base_fee=1;anchor.header.state_root=initial.root();anchor.header.transactions_root=transactions_root({});anchor.header.receipts_root=receipts_root({});
  blocks.push_back(anchor);states.push_back(initial);receipts.emplace_back();histories.emplace_back(anchor.header.hash());ExecutionEngine engine(1337,{},EvmRevision::Paris);
  auto make=[&](size_t parent,uint64_t nonce,const Phase0Deposit* deposit,bool alternate){
   Block block;block.header=blocks.at(parent).header;block.header.parent_hash=blocks.at(parent).header.hash();block.header.number=blocks.at(parent).header.number+1;block.header.timestamp=blocks.at(parent).header.timestamp+12;block.header.base_fee=next_base_fee(blocks.at(parent).header);
   if(alternate)block.header.extra_data=f::bytes_of("unit-sibling");
   if(deposit){auto tx=sample;tx.nonce=Int(std::to_string(nonce));tx.value=Int(std::to_string(deposit->message.amount_gwei))*1000000000;tx.data=calldata(*deposit);block.transactions.push_back(SignedTransaction::sign(tx,secret));}
   auto state=states.at(parent);const auto executed=engine.execute_block(state,block,false);
   auto [history,batch]=histories.at(parent).extend(engine,states.at(parent),blocks.at(parent).header,block,block.header.hash(),code_hash,1,f::checkpoint);
   f::check(state.root()==batch.state.root()&&executed.size()==batch.receipts.size(),"real execution and memory replay match");
   return std::tuple{std::move(block),std::move(batch.state),std::move(batch.receipts),std::move(history)};
  };
  for(size_t i=1;i<=execution_blocks;++i){
   f::test_slot=i;f::checkpoint();const auto* deposit=i<=nonzero_deposits?&deposits[(i-1)%2]:nullptr;
   auto [block,state,rs,history]=make(i-1,std::min(i-1,nonzero_deposits),deposit,false);blocks.push_back(std::move(block));states.push_back(std::move(state));receipts.push_back(std::move(rs));histories.push_back(std::move(history));
  }
  for(size_t i=0;i<blocks.size();++i){
   ConsensusBeaconHeader beacon{};beacon.slot=i;if(i)beacon.parent_root=ids.back();beacon.state_root=histories[i].root();beacon.body_root=f::test_root(blocks[i].header.hash());const auto id=consensus_beacon_header_root(beacon);ids.push_back(id);
   f::check(full.blocks.emplace(id,CapellaForkBlock{beacon,{},blocks[i],states[i],receipts[i],std::make_shared<const ExecutionDepositHistory>(histories[i])}).second,"distinct unit history Beacon root");
  }
  full.timed.view.anchor=ids.front();full.network_anchor=ids.front();rules.native_deposit_votes=true;rules.deposit_contract_code_hash=code_hash;rules.transition.consensus.state.eth1_votes_limit=64;rules.transition.consensus.body.deposits=16;rules.transition.consensus.maximum_block_bytes=100000;
  auto sibling=[&](const Phase0Deposit& deposit){auto [block,state,rs,history]=make(9,9,&deposit,true);ConsensusBeaconHeader beacon{};beacon.slot=10;beacon.parent_root=ids[9];beacon.state_root=history.root();beacon.body_root=f::test_root(block.header.hash());return CapellaForkBlock{beacon,{},std::move(block),std::move(state),std::move(rs),std::make_shared<const ExecutionDepositHistory>(std::move(history))};};
  same_root_sibling=sibling(deposits[1]);different_root_sibling=sibling(deposits[0]);
  f::check(same_root_sibling->deposit_history->count()==histories[10].count()&&same_root_sibling->deposit_history->root()==histories[10].root()&&same_root_sibling->execution_block.header.hash()!=blocks[10].header.hash(),"same-count/root/slot sibling premise");
  f::check(different_root_sibling->deposit_history->count()==histories[10].count()&&different_root_sibling->deposit_history->root()!=histories[10].root(),"same-count different-root sibling premise");
  f::check(histories.back().count()==nonzero_deposits&&states.back().get(ethereum_deposit_contract).balance-initial.get(ethereum_deposit_contract).balance==Int("1024000000000000000000"),"32 deposits funded by real contract transfers");
  histories.back().validate_execution(blocks.back().header,states.back(),code_hash,f::checkpoint);
 }
 void persist(const fs::path& directory)const{
  ChainStore disk(directory);disk.bind_chain(1337,blocks.front().header.hash(),"beacon");for(size_t i=0;i<blocks.size();++i){f::checkpoint();disk.commit(blocks[i],states[i],receipts[i]);}
 }
};

struct Physical {bool directory;std::array<std::string,9> fields;Bytes sha;bool operator==(const Physical&)const=default;};
using Inventory=std::map<std::string,Physical>;
inline uint64_t files_opened=0,files_closed=0;
inline std::array<std::string,9> nine(const struct stat& value){
#ifdef __APPLE__
 const auto mt=value.st_mtimespec,ct=value.st_ctimespec;
#else
 const auto mt=value.st_mtim,ct=value.st_ctim;
#endif
 return {std::to_string(value.st_dev),std::to_string(value.st_ino),std::to_string(value.st_mode),std::to_string(value.st_nlink),std::to_string(value.st_uid),std::to_string(value.st_gid),std::to_string(value.st_size),std::to_string(mt.tv_sec)+":"+std::to_string(mt.tv_nsec),std::to_string(ct.tv_sec)+":"+std::to_string(ct.tv_nsec)};
}
inline Physical physical(const fs::path& path){
 f::checkpoint();struct stat before{},after{},named{};f::check(::lstat(path.c_str(),&before)==0&&!S_ISLNK(before.st_mode),"read-only database namespace");
 if(S_ISDIR(before.st_mode))return {true,nine(before),{}};
 f::check(S_ISREG(before.st_mode)&&before.st_nlink==1&&before.st_size>=0&&uint64_t(before.st_size)<=64*1024*1024,"regular bounded database file");
 struct File {int fd;~File(){if(fd>=0&&::close(fd)==0)++files_closed;}} file{::open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC)};f::check(file.fd>=0,"database fence open");++files_opened;
 f::check(::fstat(file.fd,&after)==0&&nine(after)==nine(before),"database open identity");
 std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> hash(EVP_MD_CTX_new(),EVP_MD_CTX_free);f::check(bool(hash)&&EVP_DigestInit_ex(hash.get(),EVP_sha256(),nullptr)==1,"database fence SHA");std::array<uint8_t,65536> buffer{};uint64_t size=0;
 for(;;){f::checkpoint();const auto n=::read(file.fd,buffer.data(),buffer.size());if(n<0&&errno==EINTR)continue;f::check(n>=0,"database fence read");if(!n)break;size+=uint64_t(n);f::check(size<=uint64_t(before.st_size)&&EVP_DigestUpdate(hash.get(),buffer.data(),size_t(n))==1,"database fence bound");}
 Bytes sha(32);unsigned length=0;f::check(EVP_DigestFinal_ex(hash.get(),sha.data(),&length)==1&&length==32&&size==uint64_t(before.st_size),"database complete SHA");f::check(::fstat(file.fd,&after)==0&&::lstat(path.c_str(),&named)==0&&nine(before)==nine(after)&&nine(after)==nine(named),"database source full9 unchanged");const auto fd=std::exchange(file.fd,-1);f::check(::close(fd)==0,"database fence close");++files_closed;return {false,nine(after),std::move(sha)};
}
inline Inventory inventory(const fs::path& root){Inventory out;out.emplace("",physical(root));for(const auto& item:fs::recursive_directory_iterator(root)){f::checkpoint();f::check(out.size()<10000,"database namespace count");out.emplace(item.path().lexically_relative(root).generic_string(),physical(item.path()));}return out;}
}
