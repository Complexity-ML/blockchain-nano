#include "nano/http.hpp"
#include "nano/trie_proof.hpp"
#include "nano/simulation_queue.hpp"
#include "nano/storage_memory_policy.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <array>
#include <iostream>
#include <thread>
#include <unistd.h>
using namespace nano;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;namespace websocket=beast::websocket;using tcp=asio::ip::tcp;
namespace {
size_t checks=0,refusals=0,groups=0,vectors=0;std::string stage="arguments";Json last_response;
void check(bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);}
struct OwnedDirectory {
 std::filesystem::path path;
 OwnedDirectory(){auto text=(std::filesystem::temp_directory_path()/"nano-rpc-state-readers-XXXXXX").string();std::vector<char> name(text.begin(),text.end());name.push_back('\0');auto made=::mkdtemp(name.data());if(!made)throw std::runtime_error("cannot create owned fixture namespace");path=made;}
 ~OwnedDirectory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
struct Fixture {
 Bytes key=bytes(Int(777),32); // Existing public synthetic unit-test key, never a consensus/campaign key.
 std::string sender=public_address(public_key(key)),target=create_address(sender,0);
 Genesis genesis;std::unique_ptr<Blockchain> chain;Block a,b;State a_state,b_state;
 explicit Fixture(const std::filesystem::path& path,const char* fork="Shanghai"){
  genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork",fork}}},{"alloc",{{sender,{{"balance","0x100000000000000000000"}}}}}});
  chain=std::make_unique<Blockchain>(path/"canonical",genesis);
  a=publish(*chain,1,false);a_state=chain->state();
  {Blockchain other(path/"other-branch",genesis);b=publish(other,2,true);b_state=other.state();}
  check(a.header.number==1&&b.header.number==1&&a.header.parent_hash==genesis.block.header.hash()&&b.header.parent_hash==a.header.parent_hash&&a.header.hash()!=b.header.hash(),"fixture must contain two genuine sibling blocks");
  check(a_state.get(target).balance==1&&b_state.get(target).balance==2&&a_state.get_storage(target,0)==1&&b_state.get_storage(target,0)==2&&a_state.get(sender).nonce==1&&b_state.get(sender).nonce==2&&a_state.get(target).code!=b_state.get(target).code,"all four fixture values must distinguish the genuine branches");
 }
 Block publish(Blockchain& owner,unsigned marker,bool second){
  const auto runtime=unhex(marker==1?"0x600160005260206000f3":"0x600260005260206000f3");
  // Store two markers, then install a ten-byte runtime. No external fixture or compiler.
  Bytes init={0x60,static_cast<unsigned char>(marker),0x60,0x00,0x55,0x60,static_cast<unsigned char>(marker+1),0x60,0x01,0x55,0x60,0x0a,0x60,0x16,0x60,0x00,0x39,0x60,0x0a,0x60,0x00,0xf3};init.insert(init.end(),runtime.begin(),runtime.end());
  Transaction tx;tx.chain_id=1337;tx.nonce=0;tx.gas=200000;tx.gas_price=2000000000;tx.value=marker;tx.data=init;
  const auto creation=SignedTransaction::sign(tx,key);check(owner.submit(creation),"signed synthetic deployment must enter its fixture pool");
  if(second){tx.nonce=1;tx.to="0x00000000000000000000000000000000000000b1";tx.data.clear();tx.value=1;tx.gas=21000;check(owner.submit(SignedTransaction::sign(tx,key)),"second branch must have a distinct sender nonce");}
  auto block=owner.mine("0x00000000000000000000000000000000000000cc",12);
  const auto receipt=owner.store().receipt(creation.hash());check(receipt&&receipt->success&&receipt->contract_address==target&&owner.pending_count()==0,"deployment must really execute, persist a receipt and drain the pool");return block;
 }
 std::string account(size_t method) const{return method==2?sender:target;}
 void select(const Block& block){check(chain->restore(0,genesis.block.header.hash()),"fixture reorg must rewind through the real Blockchain API");chain->import_block(block,12);check(chain->tip().header.hash()==block.header.hash(),"fixture reorg must publish the exact imported sibling");}
};
Json request(const std::string& account,const Json& keys=Json::array(),const Json& selector="latest"){
 return {{"jsonrpc","2.0"},{"id",1},{"method","eth_getProof"},{"params",Json::array({account,keys,selector})}};
}
Json result(const Json& reply){last_response=reply;check(reply.is_object()&&reply.contains("result")&&!reply.contains("error"),"proof request must return only a result");return reply.at("result");}
void refused(const Json& reply,int code,const std::string& message={}){last_response=reply;check(reply.contains("error")&&!reply.contains("result")&&reply.at("error").at("code")==code&&(message.empty()||reply.at("error").at("message")==message),"proof request must refuse with its precise error");++refusals;}
void emit(const char* name){++groups;std::cout<<Json{{"scenario",name},{"passed",true},{"checks",checks},{"expectedRefusals",refusals}}.dump()<<std::endl;}
// Read-only path verifier independent of the builder; final Root verification
// additionally uses the existing JS account verifier and a separate storage walker.
std::optional<Bytes> verify(const Bytes& root,const Bytes& key,const std::vector<Bytes>& proof){
 if(root==keccak(Rlp(Bytes{}).encode())){check(proof.empty(),"empty trie needs no node");return {};}
 Bytes path;for(auto byte:key){path.push_back(byte>>4);path.push_back(byte&15);}size_t used=0,offset=0;Rlp expected(root);
 const auto finish=[&](std::optional<Bytes> value){check(used==proof.size(),"proof must have no unused nodes");return value;};
 for(size_t steps=0;steps<130;++steps){Rlp node;
  if(expected.list){check(expected.encode().size()<32,"inline references must stay below 32 bytes");node=expected;}
  else{check(expected.data.size()==32&&used<proof.size(),"hash reference needs a supplied node");const auto& raw=proof.at(used++);check(keccak(raw)==expected.data,"node hash must match its parent");node=Rlp::decode(raw);check(node.encode()==raw,"node RLP must be canonical");}
  check(node.list,"proof nodes are lists");
  if(node.items.size()==17){if(offset==path.size())return finish(node[16].blob().empty()?std::optional<Bytes>{}:std::optional<Bytes>{node[16].blob()});expected=node[path[offset++]];if(!expected.list&&expected.data.empty())return finish({});continue;}
  check(node.items.size()==2,"short node arity");const auto& compact=node[0].blob();check(!compact.empty()&&(compact[0]>>4)<=3,"compact flags");const unsigned flag=compact[0]>>4;check((flag&1)||(compact[0]&15)==0,"compact padding");Bytes segment;if(flag&1)segment.push_back(compact[0]&15);for(size_t i=1;i<compact.size();++i){segment.push_back(compact[i]>>4);segment.push_back(compact[i]&15);}
  if(segment.size()>path.size()-offset||!std::equal(segment.begin(),segment.end(),path.begin()+offset))return finish({});offset+=segment.size();
  if(flag&2)return finish(offset==path.size()?std::optional<Bytes>{node[1].blob()}:std::optional<Bytes>{});check(!segment.empty(),"extension path cannot be empty");expected=node[1];
 }
 throw std::runtime_error("proof verifier step limit");
}
std::vector<Bytes> nodes(const Json& value){check(value.is_array(),"proof must be an array");std::vector<Bytes> out;for(const auto& raw:value)out.push_back(unhex(raw.get<std::string>()));return out;}
Json verified(const Json& reply,const Header& header,const State& state,const std::string& account,const Json& keys,bool publish=true){
 auto value=result(reply);check(value.size()==7&&value.at("address")==address(account),"proof response has exactly the documented fields");const auto& stored=state.get(account);const bool present=!stored.empty();const Account empty;const auto& expected=present?stored:empty;
 const auto leaf=verify(header.state_root,keccak(unhex(account)),nodes(value.at("accountProof")));check(bool(leaf)==present,"account presence must be authenticated");
 check(value.at("nonce")==quantity(expected.nonce)&&value.at("balance")==quantity(expected.balance)&&value.at("codeHash")==hex(keccak(expected.code))&&value.at("storageHash")==hex(expected.storage_root()),"all account fields must match the authenticated account");
 if(leaf){const auto decoded=Rlp::decode(*leaf);check(decoded.list&&decoded.items.size()==4&&decoded[0].number()==expected.nonce&&decoded[1].number()==expected.balance&&decoded[2].blob()==expected.storage_root()&&decoded[3].blob()==keccak(expected.code),"account leaf binds nonce balance storage and code hashes");}
 check(value.at("storageProof").size()==keys.size(),"duplicate storage keys preserve order and length");Json expected_values=Json::array();
 for(size_t i=0;i<keys.size();++i){const auto key=quantity(keys[i]);const auto& entry=value.at("storageProof").at(i);const auto slot=present?state.get_storage(account,key):Int(0);check(entry.size()==3&&entry.at("key")==quantity(key)&&entry.at("value")==quantity(slot),"storage fields and ordering must match the request");const auto raw=verify(expected.storage_root(),keccak(bytes(key,32)),nodes(entry.at("proof")));check(slot==0?!raw:(raw&&Rlp::decode(*raw).number()==slot),"storage inclusion or exclusion must verify");expected_values.push_back(quantity(slot));}
 if(publish){++vectors;std::cout<<Json{{"scenario","proof-vector"},{"root",hex(header.state_root)},{"address",address(account)},{"keys",keys},{"expectedPresent",present},{"expectedValues",expected_values},{"response",value}}.dump()<<std::endl;}return value;
}
void tries(){
 stage="mpt-shapes";const std::vector<std::map<Bytes,Bytes>> sets={ {},{{Bytes{0x12},Bytes{1}}},{{Bytes{0x12},Bytes{1}},{Bytes{0x13},Bytes{2}}},{{Bytes{0x12},Bytes(40,1)},{Bytes{0x13},Bytes(40,2)},{Bytes{0xf0},Bytes{3}}},{{Bytes{},Bytes{4}},{Bytes{0x12},Bytes{5}}} };
 for(const auto& entries:sets){MptProofTrie trie(entries);check(trie.root()==trie_root(entries),"proof builder must preserve the existing consensus trie root");for(const auto& [key,value]:entries)check(verify(trie.root(),key,trie.proof(key))==std::optional<Bytes>(value),"every inserted leaf must verify");check(!verify(trie.root(),Bytes{0x99},trie.proof(Bytes{0x99})),"different paths and empty branches must prove exclusion");}
 for(const auto& raw:std::vector<Bytes>{{0x81,0x01},{0xb8,0x01,0x00},{0xc1},{0x80,0x80}}){bool rejected=false;try{(void)Rlp::decode(raw);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"noncanonical or truncated RLP must be rejected");++refusals;}
 std::map<Bytes,Bytes> entries;for(unsigned i=0;i<128;++i)entries.emplace(keccak(bytes(Int(i),32)),Rlp(Int(i+1)).encode());unsigned polls=0;bool cancelled=false;try{MptProofTrie trie(entries,{},[&]{if(++polls==20)throw ExecutionCancelled("synthetic MPT cancellation");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&polls==20,"MPT generation must stop at a cooperative checkpoint");++refusals;
 MptProofLimits limits;limits.maximum_entries=1;bool limited=false;try{MptProofTrie trie(entries,limits);}catch(const MptProofLimitExceeded&){limited=true;}check(limited,"entry budget must refuse before materializing a large trie");++refusals;emit("get-proof-native-mpt-shapes-and-canonical-rlp");
}

void indexed_proof_body_budget(const std::filesystem::path& root){
 stage="indexed-proof-body-larger-than-budget";Fixture fixture(root);auto& chain=*fixture.chain;
 Transaction tx;tx.chain_id=1337;tx.nonce=1;tx.to="0x00000000000000000000000000000000000000b1";tx.gas=600000;tx.gas_price=2000000000;tx.data=Bytes(32768,0xa5);
 const auto signed_tx=SignedTransaction::sign(tx,fixture.key);check(chain.submit(signed_tx),"large-body proof fixture transfer must be admitted normally");const auto block=chain.mine("0x00000000000000000000000000000000000000cc",24);const auto state=chain.state();const auto receipt=chain.store().receipt(signed_tx.hash());
 constexpr size_t cap=8192;check(receipt&&receipt->success&&block.to_json(true).dump().size()>cap&&block.header.to_json().dump().size()<cap&&state.to_json().dump().size()<cap,"proof fixture has a genuine large body and a bounded indexed header/state");
 RpcSimulationLimits limits;limits.state_read.maximum_snapshot_bytes=cap;Rpc rpc(chain,{},AdmissionBudget::Clock::now,limits);const auto before=chain.state().to_json();const auto keys=Json::array({"0x0","0x1","0x2"});
 // New checks use the same independent native walker without publishing extra
 // vectors: the existing 24 actual-vector external-verifier contract is kept.
 for(const auto& selector:std::array<Json,4>{Json("latest"),Json("0x2"),Json{{"blockNumber","0x2"}},Json{{"blockHash",hex(block.header.hash())},{"requireCanonical",true}}})verified(rpc.handle(request(fixture.target,keys,selector)),block.header,state,fixture.target,keys,false);
 check(chain.state().to_json()==before&&chain.pending_count()==0,"bounded indexed proofs leave their committed state and pool unchanged");
}

void selectors(){
 stage="canonical-selectors";OwnedDirectory owned;
 for(const auto* fork:{"Paris","Shanghai"}){Fixture fixture(owned.path/fork,fork);Rpc rpc(*fixture.chain);const auto before=fixture.chain->state().to_json();const Json keys=Json::array({"0x0","0x1","0x2","0x0",hex(bytes(Int(0),32))});
  for(const auto& selector:std::vector<Json>{"latest","0x1",Json{{"blockNumber","0x1"}},Json{{"blockHash",hex(fixture.a.header.hash())}},Json{{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",true}},Json{{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",false}}})verified(rpc.handle(request(fixture.target,keys,selector)),fixture.a.header,fixture.a_state,fixture.target,keys);
  verified(rpc.handle(request(fixture.sender,Json::array(),"earliest")),fixture.genesis.block.header,fixture.genesis.state,fixture.sender,Json::array());const std::string absent="0x00000000000000000000000000000000000000ff";verified(rpc.handle(request(absent,keys)),fixture.a.header,fixture.a_state,absent,keys);
  check(fixture.chain->state().to_json()==before&&fixture.chain->pending_count()==0,"proofs must not change state or transaction pool");
 }
 {auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",Json::object()}});Blockchain chain(owned.path/"empty",genesis);Rpc rpc(chain);const std::string absent="0x00000000000000000000000000000000000000ff";auto proof=verified(rpc.handle(request(absent,Json::array({"0x0"}),"earliest")),genesis.block.header,genesis.state,absent,Json::array({"0x0"}));check(proof["accountProof"].empty()&&proof["storageProof"][0]["proof"].empty(),"empty account and storage tries need empty proofs");}
 {const std::string ghost="0x00000000000000000000000000000000000000ee";auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Paris"}}},{"alloc",{{ghost,{{"storage",{{"0x0","0x9"}}}}}}}});Blockchain chain(owned.path/"unpersisted-empty",genesis);Rpc rpc(chain);verified(rpc.handle(request(ghost,Json::array({"0x0"}),"earliest")),genesis.block.header,genesis.state,ghost,Json::array({"0x0"}));}
 indexed_proof_body_budget(owned.path/"body-budget");
 emit("get-proof-account-storage-and-object-selectors");
}
void errors_and_limits(){
 stage="errors-and-config";OwnedDirectory owned;Fixture fixture(owned.path);Rpc rpc(*fixture.chain);const auto hash=hex(fixture.a.header.hash());
 for(const auto& selector:std::vector<Json>{Json::object(),Json{{"blockNumber","0x1"},{"blockHash",hash}},Json{{"blockNumber","0x00"}},Json{{"blockNumber","latest"}},Json{{"blockNumber","0x1"},{"requireCanonical",false}},Json{{"blockHash","0x01"}},Json{{"blockHash",hash},{"requireCanonical",1}},Json{{"blockHash",hash},{"extra",true}},nullptr})refused(rpc.handle(request(fixture.target,Json::array(),selector)),-32602);
 for(const auto& key:std::vector<Json>{nullptr,1,"0x","0x00","0x-1","0xgg",std::string("0x")+std::string(65,'1')})refused(rpc.handle(request(fixture.target,Json::array({key}))),-32602);
 for(const auto& address:std::vector<Json>{nullptr,1,"0x12"}){auto req=request(fixture.target);req["params"][0]=address;refused(rpc.handle(req),-32602);}
 for(const auto& selector:std::vector<Json>{Json{{"blockHash",hex(Bytes(32,0xee))},{"requireCanonical",true}},Json{{"blockNumber","0x777"}}})refused(rpc.handle(request(fixture.target,Json::array(),selector)),-32001,"Unknown block");
 refused(rpc.handle(request(fixture.target,Json::array(),"0x777")),-32000,"Unknown block");for(const auto* tag:{"pending","safe"})refused(rpc.handle(request(fixture.target,Json::array(),tag)),-32000);
 auto bad=request(fixture.target);bad["params"]=Json::array({fixture.target});refused(rpc.handle(bad),-32602);bad=request(fixture.target);bad["params"][1]="0x0";refused(rpc.handle(bad),-32602);
 for(const auto* name:{"maximum_storage_keys","maximum_accounts","maximum_storage_entries","maximum_snapshot_bytes","maximum_trie_bytes","maximum_trie_nodes","maximum_output_bytes"}){
  const auto limits=RpcSimulationLimits::from_json({{"state_read",{{name,1}}}});Rpc limited(*fixture.chain,{},AdmissionBudget::Clock::now,limits);
  const auto keys=std::string(name)=="maximum_storage_keys"?Json::array({"0x0","0x1"}):Json::array({"0x0"});refused(limited.handle(request(fixture.target,keys)),-32005);
 }
 const auto expanded=RpcSimulationLimits::from_json({{"state_read",{{"maximum_storage_keys",128}}}});Rpc permissive(*fixture.chain,{},AdmissionBudget::Clock::now,expanded);Json many=Json::array();for(unsigned i=0;i<65;++i)many.push_back("0x0");check(result(permissive.handle(request(fixture.target,many))).at("storageProof").size()==65,"operator may raise the default storage-key limit");
 for(const auto& config:std::vector<Json>{Json{{"state_read",nullptr}},Json{{"state_read",{{"unknown",1}}}},Json{{"state_read",{{"maximum_accounts",0}}}},Json{{"state_read",{{"maximum_storage_keys",65537}}}},Json{{"state_read",{{"maximum_snapshot_bytes",true}}}}}){bool rejected=false;try{(void)RpcSimulationLimits::from_json(config);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"invalid operator limits must fail explicitly");++refusals;}
 unsigned polls=0;refused(rpc.handle(request(fixture.target,Json::array({"0x0"})),[&]{if(++polls==40)throw ExecutionCancelled("synthetic proof cancellation");}),-32002,"synthetic proof cancellation");check(polls==40,"decoder/trie checkpoints must remain cancellable");
 RpcSimulationLimits timeout;auto time=std::chrono::steady_clock::time_point{};timeout.timeout=std::chrono::milliseconds(3);timeout.now=[&]{auto out=time;time+=std::chrono::milliseconds(1);return out;};Rpc timed(*fixture.chain,{},AdmissionBudget::Clock::now,timeout);refused(timed.handle(request(fixture.target)),-32002,"RPC state-read execution timeout");emit("get-proof-errors-configurable-budgets-and-cancellation");
}
void reorg_and_retention(){
 stage="reorg-and-retention";OwnedDirectory owned;Fixture fixture(owned.path/"fork");Rpc rpc(*fixture.chain);const Json hash={{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",true}};
 unsigned polls=0;bool changed=false;auto before_capture=rpc.handle(request(fixture.target,Json::array(),hash),[&]{if(++polls==14){fixture.select(fixture.b);changed=true;}});check(changed,"first genuine reorg occurs after header selection");refused(before_capture,-32000,"Selected block is not canonical");fixture.select(fixture.a);
 // No keys and an indexed header under 4096 bytes: one/prepare/worker=1/2/3,
 // context=4/5, hash header=6..13, selected14/pre-pin15/pinned-copy16.
 // These reorgs therefore preserve the original before/after-state-pin oracle.
 polls=0;changed=false;auto pinned=rpc.handle(request(fixture.target,Json::array(),hash),[&]{if(++polls==16){fixture.select(fixture.b);changed=true;}});check(changed&&fixture.chain->tip().header.hash()==fixture.b.header.hash(),"second genuine reorg must complete outside the Store mutex");verified(pinned,fixture.a.header,fixture.a_state,fixture.target,Json::array());refused(rpc.handle(request(fixture.target,Json::array(),hash)),-32001,"Unknown block");verified(rpc.handle(request(fixture.target,Json::array(),Json{{"blockNumber","0x1"}})),fixture.b.header,fixture.b_state,fixture.target,Json::array());
 stage="retention-and-reopen";const auto path=owned.path/"retained";const std::string account="0x00000000000000000000000000000000000000a1";
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"evmFork","Shanghai"}}},{"alloc",{{account,{{"balance","0x9"},{"code","0x00"},{"storage",{{"0x0","0x7"}}}}}}}});auto chain=std::make_unique<Blockchain>(path,genesis);std::vector<Block> blocks{genesis.block};for(unsigned n=1;n<=6;++n)blocks.push_back(chain->mine("0x00000000000000000000000000000000000000cc",Int(n*12)));
 const auto state3=chain->store().state_at(3);check(bool(state3),"retention fixture state three exists");const auto tip=blocks.back().header.hash();const auto snapshot=Json{{"format","synthetic-get-proof-checkpoint-v1"},{"head",hex(tip)}}.dump();constexpr size_t budget=8*1024*1024;ForkCheckpointToken token;auto& store=const_cast<ChainStore&>(chain->store());token=store.commit_execution_checkpoint(1337,genesis.block.header.hash(),{},tip,6,tip,{},snapshot,budget,8);
 const auto prune=[&]{token=store.commit_execution_checkpoint(1337,genesis.block.header.hash(),token,tip,6,tip,{},snapshot,budget,2,{},ExecutionStateRetentionAnchor{6,tip});};prune();const auto marker=store.state_pruning();check(marker&&marker->before==3&&marker->anchor.hash==tip,"first real retention frontier is three with anchor six");
 {
  Rpc reader(*chain);refused(reader.handle(request(account,Json::array(),Json{{"blockNumber","0x2"}})),-32000,"State unavailable");refused(reader.handle(request(account,Json::array(),Json{{"blockHash",hex(blocks[2].header.hash())}})),-32000,"State unavailable");
  polls=0;bool pruned=false;auto retained=reader.handle(request(account,Json::array(),Json{{"blockHash",hex(blocks[3].header.hash())},{"requireCanonical",true}}),[&]{if(++polls==16){prune();pruned=true;}});check(pruned&&!store.state_at(3),"real pruning writer must finish while a pinned proof snapshot is detached");verified(retained,blocks[3].header,*state3,account,Json::array());refused(reader.handle(request(account,Json::array(),Json{{"blockNumber","0x3"}})),-32000,"State unavailable");check(store.block_by_number(3)->header.hash()==blocks[3].header.hash()&&store.header_by_number(3,65536)->to_json()==blocks[3].header.to_json()&&store.header_by_hash(blocks[3].header.hash(),65536)->hash()==blocks[3].header.hash(),"state pruning preserves canonical block/header history and reverse membership");
 }
 chain.reset();{ChainStore disk(path,ChainStore::OpenMode::ReadOnly);const auto info=disk.state_pruning();const auto checkpoint=disk.fork_checkpoint(1337,genesis.block.header.hash(),budget,token);check(info&&info->before==5&&checkpoint&&checkpoint->snapshot==snapshot&&disk.header_by_number(3,65536)->hash()==blocks[3].header.hash(),"cold read-only reopen retains pruned historical headers, frontier and externally held checkpoint token");}
 chain=std::make_unique<Blockchain>(path,genesis,ChainStore::OpenMode::ReadWriteExisting);{Rpc reader(*chain);refused(reader.handle(request(account,Json::array({"0x0"}),Json{{"blockHash",hex(blocks[3].header.hash())}})),-32000,"State unavailable");verified(reader.handle(request(account,Json::array({"0x0"}))),blocks[6].header,chain->state(),account,Json::array({"0x0"}));}
 emit("get-proof-pinned-reorg-real-retention-and-reopen");
}
struct Running {HttpServer& server;std::thread thread;explicit Running(HttpServer& value):server(value),thread([&value]{value.run();}){}~Running(){server.stop();if(thread.joinable())thread.join();}};
http::response<http::string_body> post(uint16_t port,const Json& input){
 asio::io_context io;beast::tcp_stream stream(io);http::request<http::string_body> message(http::verb::post,"/",11);message.set(http::field::host,"localhost");message.set(http::field::content_type,"application/json");message.body()=input.dump();message.prepare_payload();beast::flat_buffer buffer;http::response_parser<http::string_body> parser;parser.body_limit(2*1024*1024);beast::error_code failure;
 stream.expires_after(std::chrono::seconds(5));stream.async_connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),port),[&](beast::error_code error){if(error){failure=error;return;}http::async_write(stream,message,[&](beast::error_code error,size_t){if(error){failure=error;return;}http::async_read(stream,buffer,parser,[&](beast::error_code error,size_t){failure=error;});});});io.run();if(failure)throw std::runtime_error("bounded HTTP: "+failure.message());return parser.release();
}
Json ws(uint16_t port,const Json& input){
 asio::io_context io;websocket::stream<beast::tcp_stream> stream(io);stream.set_option(websocket::stream_base::timeout{std::chrono::seconds(5),std::chrono::seconds(5),false});stream.read_message_max(2*1024*1024);beast::flat_buffer buffer;const auto message=input.dump();beast::error_code failure;
 beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(5));beast::get_lowest_layer(stream).async_connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),port),[&](beast::error_code error){if(error){failure=error;return;}beast::get_lowest_layer(stream).expires_never();stream.async_handshake("localhost","/",[&](beast::error_code error){if(error){failure=error;return;}stream.async_write(asio::buffer(message),[&](beast::error_code error,size_t){if(error){failure=error;return;}stream.async_read(buffer,[&](beast::error_code error,size_t){failure=error;});});});});io.run();beast::error_code ignored;beast::get_lowest_layer(stream).socket().close(ignored);if(failure)throw std::runtime_error("bounded WebSocket: "+failure.message());return Json::parse(beast::buffers_to_string(buffer.data()));
}
void transport(){
 stage="worker-classification-and-loopback";OwnedDirectory owned;Fixture fixture(owned.path);Rpc rpc(*fixture.chain);auto good=request(fixture.target,Json::array({"0x0","0x1"}),Json{{"blockHash",hex(fixture.a.header.hash())},{"requireCanonical",true}});
 for(const auto* method:{"eth_getBalance","eth_getCode","eth_getTransactionCount","eth_getStorageAt","eth_getProof"}){auto one=good;one["method"]=method;check(simulation_count(one)==1,"all five state readers must be classified expensive before execution");one.erase("id");check(simulation_count(one)==1,"notifications consume worker capacity too");}
 Json oversized=Json::array();for(unsigned n=0;n<9;++n)oversized.push_back(good);check(simulation_count(oversized)==SimulationQueue::limit+1,"oversize proof batch must be recognized before running any item");
 {
  HttpServer server(rpc,"127.0.0.1",0);Running running(server);
  const auto reply=post(server.port(),good);check(reply.result()==http::status::ok,"HTTP proof succeeds as JSON-RPC");verified(Json::parse(reply.body()),fixture.a.header,fixture.a_state,fixture.target,good["params"][1]);
  verified(ws(server.port(),good),fixture.a.header,fixture.a_state,fixture.target,good["params"][1]);const auto refused_batch=post(server.port(),oversized);check(refused_batch.result()==http::status::service_unavailable,"HTTP rejects oversize work batch before execution");const auto ws_rejected=ws(server.port(),oversized);check(ws_rejected.is_array()&&ws_rejected.size()==9,"WebSocket rejects every item of the oversize batch before execution");for(const auto& reply:ws_rejected)refused(reply,-32005);
  const auto invalid=request(fixture.target,Json::array(),Json{{"blockNumber","latest"}});refused(Json::parse(post(server.port(),invalid).body()),-32602);refused(ws(server.port(),invalid),-32602);
  auto notification=good;notification.erase("id");const auto silent=post(server.port(),notification);check(silent.result()==http::status::no_content&&silent.body().empty(),"proof notification executes without a response body");
 }
 check(fixture.chain->tip().header.hash()==fixture.a.header.hash()&&fixture.chain->pending_count()==0,"HTTP/WS drain and shutdown leave the synthetic chain unchanged");emit("get-proof-http-websocket-capacity-and-shutdown");
}
}
int main(int argc,char** argv){try{
 if((argc!=2&&argc!=4)||std::string(argv[1])!="--get-proof-self-test"||(argc==4&&std::string(argv[2])!="--group"))throw std::invalid_argument("expected --get-proof-self-test [--group tries|selectors|limits|retention|transport]");const std::string group=argc==4?argv[3]:"all";if(group!="all"&&group!="tries"&&group!="selectors"&&group!="limits"&&group!="retention"&&group!="transport")throw std::invalid_argument("unknown proof group");
 const ChainStoreMemoryPolicy policy{4*1024*1024,2,8*1024*1024,4*1024*1024};ScopedChainStoreMemoryPolicy memory(policy);
 if(group=="all"||group=="tries")tries();if(group=="all"||group=="selectors")selectors();if(group=="all"||group=="limits")errors_and_limits();if(group=="all"||group=="retention")reorg_and_retention();if(group=="all"||group=="transport")transport();
 if(group=="all")check(groups==5&&refusals==63&&vectors==24,"full proof suite must reach all intended groups/refusals/vectors");
 std::cout<<Json{{"scenario","get-proof-summary"},{"passed",true},{"groups",groups},{"checks",checks},{"expectedRefusals",refusals},{"proofVectors",vectors},{"selectedGroup",group},{"externalNetwork",false},{"consensusModified",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& error){std::cerr<<Json{{"scenario","get-proof-summary"},{"passed",false},{"stage",stage},{"checks",checks},{"expectedRefusals",refusals},{"error",error.what()},{"lastRpcResponse",last_response}}.dump()<<std::endl;return 1;}}
