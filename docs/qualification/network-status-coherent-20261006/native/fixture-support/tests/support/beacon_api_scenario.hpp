#pragma once
#include "nano/beacon_api.hpp"
#include "../../src/beacon_block_json.hpp"
#include "nano/http.hpp"
#include <boost/beast.hpp>
namespace test {
inline void beacon_api_scenario(const nano::Genesis& genesis,const nano::BellatrixForkBlockRules& rules,const nano::BellatrixForkStore& store,const nano::BellatrixStoreSnapshotLimits& limits){
 using namespace nano;namespace http=boost::beast::http;namespace asio=boost::asio;using tcp=asio::ip::tcp;
 const auto& head=bellatrix_fork_head(store,rules);const auto original=head.state.ssz;
 auto number=[&](size_t at){uint64_t value=0;for(unsigned i=0;i<8;++i)value|=uint64_t(original.at(at+i))<<(8*i);return std::to_string(value);};
 auto field=[&](size_t at,size_t n){return hex(Bytes(original.begin()+at,original.begin()+at+n));};
 const auto& l=rules.transition.consensus.state;const size_t at=176+64*l.slots_per_historical_root+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+9;
 const auto checkpoints=bellatrix_beacon_read(store,rules,"/eth/v1/beacon/states/head/finality_checkpoints");
 for(const auto& item:std::vector<std::pair<std::string,size_t>>{{"previous_justified",at},{"current_justified",at+40},{"finalized",at+80}}){
  check(checkpoints.body.at("data").at(item.first)==Json{{"epoch",number(item.second)},{"root",field(item.second+8,32)}},"Beacon checkpoint differs from actual state");
 }
 const auto genesis_response=bellatrix_beacon_read(store,rules,"/eth/v1/beacon/genesis");
 check(genesis_response.body.at("data")==Json{{"genesis_time",number(0)},{"genesis_validators_root",field(8,32)},{"genesis_fork_version",hex(Bytes(rules.transition.consensus.deposit.genesis_fork_version.begin(),rules.transition.consensus.deposit.genesis_fork_version.end()))}},"Beacon genesis differs from authenticated input");
 const auto spec=bellatrix_beacon_read(store,rules,"/eth/v1/config/spec");
 check(spec.body.at("data").at("SLOTS_PER_EPOCH")==std::to_string(rules.transition.consensus.epoch.ffg.slots_per_epoch)&&spec.body.at("data").at("SECONDS_PER_SLOT")==std::to_string(rules.transition.seconds_per_slot),"Beacon spec used Ethereum defaults");
 check(spec.body.at("data").at("MAX_VALIDATORS_PER_COMMITTEE")==std::to_string(rules.transition.consensus.body.validators_per_committee)&&spec.body.at("data").at("VALIDATOR_REGISTRY_LIMIT")==std::to_string(l.validator_registry_limit),"Beacon spec confused committee and registry bounds");
 check(!spec.body.at("data").contains("CAPELLA_FORK_EPOCH")&&!spec.body.at("data").contains("PRESET_BASE"),"Beacon spec invented schedule or preset");
 for(const auto& [key,value]:spec.body.at("data").items())check(value.is_string(),"Beacon spec value not a string");
 auto changed_rules=rules;changed_rules.proposer_score_boost+=1;
 const auto changed_spec=bellatrix_beacon_read(store,changed_rules,"/eth/v1/config/spec");
 check(changed_spec.body.at("data").at("PROPOSER_SCORE_BOOST")==std::to_string(changed_rules.proposer_score_boost)&&changed_spec.body.at("data").at("CONFIG_NAME")!=spec.body.at("data").at("CONFIG_NAME"),"Beacon spec ignored changed profile");
 const auto fork=bellatrix_beacon_read(store,rules,"/eth/v1/beacon/states/head/fork");
 check(fork.body.at("data")==Json{{"previous_version",field(48,4)},{"current_version",field(52,4)},{"epoch",number(56)}},"Beacon fork projection");
 check(bellatrix_beacon_read(store,rules,"/eth/v1/beacon/states/head/root").body.at("data").at("root")==hex(Bytes(head.header.state_root.begin(),head.header.state_root.end())),"Beacon root projection");
 check(bellatrix_beacon_read(store,rules,"/eth/v1/beacon/states/999/root").status==404,"unsupported state invented");
 bool cancelled=false;try{bellatrix_beacon_read(store,rules,"/eth/v1/beacon/states/head/root",[]{throw std::runtime_error("cancel");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled,"Beacon cancellation ignored");
 auto damaged=store;damaged.blocks.at(consensus_beacon_header_root(head.header)).state.ssz[8]^=1;
 bool refused=false;try{bellatrix_beacon_read(damaged,rules,"/eth/v1/beacon/states/head/root");}catch(const std::exception&){refused=true;}check(refused,"corrupt state exposed");
 Temp location;Blockchain chain(location/"chain",genesis);
 std::atomic<uint64_t> wall=store.timed.clock.time_ms;
 auto runtime=start_bellatrix_runtime(chain,bellatrix_runtime_config(location,rules,limits,store),{},[&]{return wall.load();});
 Rpc rpc(chain,"beacon-read-test",AdmissionBudget::Clock::now,RpcSimulationLimits{},nullptr,runtime.rpc);std::atomic<bool> unavailable=false;rpc.set_beacon_sync_tracker(runtime.sync_tracker);
 BeaconReadHandler handler=[&](std::string_view path,const std::function<void()>& check){if(unavailable.load())throw std::runtime_error("private failure detail");return runtime.beacon_read(path,check);};
 bool private_bind=false;try{HttpServer invalid(rpc,"0.0.0.0",0,std::chrono::seconds(4),handler);}catch(const std::invalid_argument&){private_bind=true;}check(private_bind,"Beacon public bind allowed");
 BeaconEventJournal journal;std::mutex events_mutex;
 BeaconEventsHandler events_handler=[&](std::optional<uint64_t> cursor){std::lock_guard guard(events_mutex);return journal.read(cursor);};
 HttpServer server(rpc,"127.0.0.1",0,std::chrono::seconds(4),handler,events_handler);
 std::thread thread([&]{server.run();});
 auto request=[&](http::verb method,const char* path,bool origin=false,const std::string& accept=""){
  asio::io_context context;boost::beast::tcp_stream socket(context);socket.expires_after(std::chrono::seconds(5));socket.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),server.port()));
  http::request<http::string_body> req(method,path,11);req.set(http::field::host,"localhost");if(origin)req.set(http::field::origin,"https://example.invalid");if(!accept.empty())req.set(http::field::accept,accept);req.prepare_payload();http::write(socket,req);
  boost::beast::flat_buffer buffer;http::response<http::string_body> response;http::read(socket,buffer,response);return response;
 };
 try{
  BeaconEventJournal bounded_events;
  for(size_t i=0;i<=BeaconEventJournal::capacity;++i)bounded_events.append("block",{{"slot",std::to_string(i)}});
  bool gap=false;try{bounded_events.read(0);}catch(const std::runtime_error&){gap=true;}check(gap,"lost event cursor silently accepted");
  check(bounded_events.read(1).at("events").size()==BeaconEventJournal::capacity&&bounded_events.read(std::nullopt).at("events").empty(),"event ring/reconnect bounds");
  for(const auto* path:{"/eth/v1/events","/eth/v1/events?topics=head","/eth/v1/events?topics=block,block","/eth/v1/events?topics=block,","/eth/v1/events?topics=execution_payload_available"})check(request(http::verb::get,path).result_int()==400,"invalid/unsupported event subscription accepted");
  check(request(http::verb::post,"/eth/v1/events?topics=block").result_int()==405,"event POST accepted");
  check(request(http::verb::get,"/eth/v1/events?topics=block",true).result_int()==403,"event Origin accepted");
  check(request(http::verb::get,"/eth/v1/events?topics=block",false,"application/json").result_int()==406,"event JSON representation accepted");
  asio::io_context event_context;
  std::vector<std::unique_ptr<boost::beast::tcp_stream>> streams;
  for(const auto* path:{"/eth/v1/events?topics=block,finalized_checkpoint","/eth/v1/events?topics=block&topics=finalized_checkpoint","/eth/v1/events?topics=block%2Cfinalized_checkpoint","/eth/v1/events?topics=block"}){
   auto stream=std::make_unique<boost::beast::tcp_stream>(event_context);stream->connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"),server.port()));
   http::request<http::empty_body> req(http::verb::get,path,11);req.set(http::field::accept,"text/event-stream");http::write(*stream,req);
   boost::beast::flat_buffer buffer;http::response_parser<http::empty_body> header;http::read_header(*stream,buffer,header);
   check(header.get().result_int()==200&&header.get()[http::field::content_type]=="text/event-stream"&&header.get().chunked(),"SSE header or topic encoding rejected");streams.push_back(std::move(stream));
  }
  check(request(http::verb::get,"/eth/v1/events?topics=block").result_int()==503,"unbounded event stream admission");
  // Each stream's cursor is now 0. Overrun the ring before its next poll;
  // the server must close instead of silently dropping committed events.
  {std::lock_guard guard(events_mutex);for(size_t i=0;i<=BeaconEventJournal::capacity;++i)journal.append("block",{{"slot",std::to_string(i)}});}
  for(auto& stream:streams){
   stream->socket().non_blocking(true);const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);bool closed=false;std::array<char,4096> bytes;
   while(std::chrono::steady_clock::now()<deadline){boost::system::error_code ec;stream->socket().read_some(asio::buffer(bytes),ec);if(ec==asio::error::eof||ec==asio::error::connection_reset){closed=true;break;}if(ec&&ec!=asio::error::would_block&&ec!=asio::error::try_again)throw boost::system::system_error(ec);std::this_thread::sleep_for(std::chrono::milliseconds(10));}
   check(closed,"lagging SSE consumer kept open after cursor loss");
  }
  auto response=request(http::verb::get,"/eth/v1/beacon/states/head/finality_checkpoints");check(response.result_int()==200&&Json::parse(response.body())==checkpoints.body,"Beacon HTTP projection differs");
  check(request(http::verb::get,"/eth/v1/beacon/states/head/fork").result_int()==200,"Beacon fork HTTP");
  check(Json::parse(request(http::verb::get,"/eth/v1/beacon/genesis").body())==genesis_response.body,"Beacon genesis HTTP");
  check(Json::parse(request(http::verb::get,"/eth/v1/config/spec").body())==spec.body,"Beacon spec HTTP");
  check(Json::parse(request(http::verb::get,"/eth/v1/node/version").body()).at("data").at("version")=="Nano/0.1.0/c++20","Beacon version HTTP");
  check(request(http::verb::get,"/eth/v1/beacon/states/head/root").result_int()==200,"Beacon root HTTP");
  check(request(http::verb::get,"/eth/v1/config/spec",false,"APPLICATION/JSON;q=0.4").result_int()==200,"JSON accept quality or case failed");
  for(const auto* accept:{"text/plain","application/json;q=0,*/*;q=1","application/octet-stream"})check(request(http::verb::get,"/eth/v1/config/spec",false,accept).result_int()==406,"unsupported or excluded representation accepted");
  check(request(http::verb::post,"/eth/v1/beacon/states/head/root").result_int()==405,"Beacon write allowed");
  check(request(http::verb::get,"/eth/v1/beacon/states/head/root",true).result_int()==403,"browser Origin allowed");
  check(request(http::verb::get,"/eth/v1/beacon/headers/head").result_int()==404,"anchor HTTP signature invented");
  check(request(http::verb::get,"/eth/v1/node/syncing").result_int()==503,"missing observation reported synced");
  auto report=[&](const Json& slot,const std::string& token="beacon-read-test"){
   Json peer={{"peerId","fixture-peer"},{"bestBlock",uint64_t(999999)},{"beaconHeadSlot",slot}};
   Json observation={{"peerId","fixture-self"},{"addresses",Json::array()},{"peers",Json::array({peer})}};
   return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_networkReport"},{"params",Json::array({token,observation})}});
  };
  check(report(head.header.slot,"wrong-token").contains("error"),"unauthorized Beacon observation accepted");
  check(request(http::verb::get,"/eth/v1/node/syncing").result_int()==503,"unauthorized report changed sync state");
  check(report(head.header.slot).value("result",false),"valid Beacon report rejected");
  auto sync=Json::parse(request(http::verb::get,"/eth/v1/node/syncing").body()).at("data");
  check(sync.at("head_slot")==std::to_string(head.header.slot)&&sync.at("sync_distance")=="0"&&sync.at("is_syncing")==false,"Beacon sync confused execution height and slot");
  runtime.clock->stop();wall+=2*rules.slot_duration_ms;
  check(report(head.header.slot+2).value("result",false),"ahead peer report rejected");
  sync=Json::parse(request(http::verb::get,"/eth/v1/node/syncing").body()).at("data");
  check(sync.at("sync_distance")=="2"&&sync.at("is_syncing")==true,"Beacon catch-up not reported");
  check(report("0xffff").contains("error"),"malformed Beacon observation accepted");
  check(report(UINT64_MAX).value("result",false),"future fixture report rejected at wrong layer");
  check(request(http::verb::get,"/eth/v1/node/syncing").result_int()==503,"future peer claim reported as valid target");
  runtime.sync_tracker->report({});check(request(http::verb::get,"/eth/v1/node/syncing").result_int()==503,"isolated node declared synced");
  BeaconSyncTracker::Clock::time_point monotonic{};BeaconSyncTracker expiry([&]{return monotonic;});
  check(!expiry.current(),"initial sync sample exists");expiry.report({1});check(expiry.current().has_value(),"fresh sample absent");monotonic+=std::chrono::seconds(5);check(!expiry.current(),"expired sample retained");
  expiry.report({2});monotonic-=std::chrono::seconds(1);check(!expiry.current(),"backwards clock sample retained");
  unavailable=true;response=request(http::verb::get,"/eth/v1/beacon/states/head/root");check(response.result_int()==503&&response.body().find("private failure detail")==std::string::npos,"failure leaked internals or returned success");
 }catch(...){server.stop();thread.join();throw;}
 server.stop();thread.join();check(head.state.ssz==original,"Beacon read changed state");
 std::cout<<Json{{"beaconReadEndpoints",7},{"syncObservationsQualified",true},{"specParameters",spec.body.at("data").size()},{"httpOriginDenied",true},{"publicBindDenied",true},{"unavailableStatus",503},{"corruptStateRejected",true},{"assertoorQualified",false}}.dump()<<std::endl;
}
}

namespace test {
inline void beacon_header_scenario(const nano::BellatrixPublishedStore& snapshot,const nano::BellatrixForkBlockRules& rules,const nano::Bytes& wire){
 using namespace nano;
 const auto& head=bellatrix_fork_head(*snapshot.state,rules);const auto id=consensus_beacon_header_root(head.header);const auto idhex=hex(Bytes(id.begin(),id.end()));
 const auto route=std::string("/eth/v1/beacon/headers/");
 const auto response=bellatrix_beacon_read(snapshot,rules,route+"head");
 check(response.status==200,"retained signed head unavailable");
 const auto full=bellatrix_beacon_read(snapshot,rules,"/eth/v2/beacon/blocks/head");
 check(full.status==200&&full.ssz==wire&&full.version=="bellatrix","full block lost archived SSZ");
 const auto& payload=full.body.at("data").at("message").at("body").at("execution_payload");
 check(payload.at("block_hash")==hex(head.execution_block.header.hash())&&payload.at("base_fee_per_gas")==head.execution_block.header.base_fee->get_str(),"full block execution fields differ");
 check(full.body.at("data").at("message").at("state_root")==hex(Bytes(head.header.state_root.begin(),head.header.state_root.end())),"full block Beacon state root differs");

 const auto& data=response.body.at("data");check(data.at("canonical")==true&&data.at("root")==idhex,"signed header canonical root differs");
 check(data.at("header").at("signature")==hex(Bytes(wire.begin()+4,wire.begin()+100)),"signed header signature invented");
 check(data.at("header").at("message").at("slot")==std::to_string(head.header.slot),"signed header slot differs");
 check(bellatrix_beacon_read(snapshot,rules,route+idhex).body==response.body,"header root lookup differs");
 check(bellatrix_beacon_read(snapshot,rules,route+std::to_string(head.header.slot)).body==response.body,"header slot lookup differs");
 check(bellatrix_beacon_read(snapshot,rules,route+std::to_string(head.header.slot+1)).status==404,"future header invented");
 for(const auto& bad:{"", "-1", "0x01", "18446744073709551616", "2?foo=1"})check(bellatrix_beacon_read(snapshot,rules,route+bad).status==400,"invalid header selector accepted");
 check(bellatrix_beacon_read(snapshot,rules,route+hex(Bytes(32,255))).status==404,"unknown root invented");
 const auto anchor=snapshot.state->timed.view.anchor;
 check(bellatrix_beacon_read(snapshot,rules,route+hex(Bytes(anchor.begin(),anchor.end()))).status==404,"anchor signature invented");
 for(const size_t at:{size_t(4),size_t(184)}){
  auto corrupted=snapshot;auto archive=std::make_shared<BellatrixSignedBlockArchive>(*snapshot.signed_blocks);auto changed=wire;changed.at(at)^=1;(*archive)[id]=std::make_shared<const Bytes>(std::move(changed));corrupted.signed_blocks=archive;
  bool refused=false;try{bellatrix_beacon_read(corrupted,rules,route+"head");}catch(const std::exception&){refused=true;}check(refused,"corrupt archived block exposed");
 }
 auto absent=snapshot;absent.signed_blocks.reset();check(bellatrix_beacon_read(absent,rules,route+"head").status==404,"missing archive became success");
 bool cancelled=false;try{bellatrix_beacon_read(snapshot,rules,route+"head",[]{throw std::runtime_error("cancel");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled,"header cancellation ignored");
 if(const auto* output=std::getenv("NANO_TEST_BEACON_BLOCK_EXPORT")){
  const fs::path dest(output);check(!fs::exists(dest),"Beacon block export exists");fs::create_directories(dest);
  save(dest/"spec.json",bellatrix_beacon_read(*snapshot.state,rules,"/eth/v1/config/spec").body.at("data"));
  const auto source=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-bellatrix-containers-official";
  size_t count=0;Json operations=Json::object();
  const auto manifest=load(source/"manifest.json");
  for(const auto& row:manifest.at("cases")){
   const auto name=row.at("name").get<std::string>();if(!name.starts_with("BeaconBlockBody/"))continue;
   const auto raw=phase0_minimal::binary(source/name/"serialized.ssz");check(hex(sha256(raw)).substr(2)==row.at("files").at("serialized.ssz").at("sha256").get<std::string>(),"body fixture digest mismatch");
   const auto body_root=bellatrix_beacon_body_root(raw,rules.transition.consensus.body,rules.transition.consensus.state.sync_committee_size,rules.transition.execution,rules.transition.consensus.maximum_block_bytes);
   const auto roots=read(source/name/"roots.yaml");check(roots.find(hex(Bytes(body_root.begin(),body_root.end())))!=std::string::npos,"body reference root differs");
   Bytes synthetic(184);synthetic[0]=100;synthetic[180]=84;synthetic.insert(synthetic.end(),raw.begin(),raw.end());
   const auto json=beacon_block_json(synthetic,rules,{});save(dest/(std::to_string(count)+".json"),json);
   std::ofstream file(dest/(std::to_string(count)+".ssz"),std::ios::binary);file.write(reinterpret_cast<const char*>(synthetic.data()),synthetic.size());check(bool(file),"SSZ fixture export failed");
   for(const auto* kind:{"proposer_slashings","attester_slashings","attestations","deposits","voluntary_exits"})operations[kind]=operations.value(kind,size_t(0))+json.at("message").at("body").at(kind).size();
   ++count;
  }
  check(count==8,"expected eight body fixtures");std::cout<<Json{{"bodySerializationFixtures",count},{"operations",operations},{"syntheticEnvelope",true}}.dump()<<std::endl;
 }
 std::cout<<Json{{"signedHeaderSelectors",{"head","root","slot"}},{"anchorWithoutSignature",404},{"corruptSignatureRejected",true},{"corruptBodyRejected",true},{"assertoorQualified",false}}.dump()<<std::endl;
}
}
