from pathlib import Path
r=Path('/Users/boris/Dev/blockchain-nano')
p=r/'network/capella_relay_schedule.hpp';s=p.read_text().replace('bool attestation_=false;','bool attestation_=false,history_first_=false;');s=s.replace('public:\n unsigned next()', '''public:
 // Called only when next() grants a block turn. A moving/unresponsive head
 // cannot consume every historical catch-up opportunity on this peer.
 bool next_block_history_first(){const bool selected=history_first_;history_first_=!history_first_;return selected;}
 unsigned next()''');p.write_text(s)
p=r/'network/capella_head_catalog_relay.hpp';s=p.read_text();s=s.replace('bool fair_,started_', 'const bool history_first_;std::optional<Json> history_result_;bool fair_,started_')
old='void fail(const char* reason){finish(Json{{"success",false},{"reason",reason},{"stage",catalog_?"catalog":"head"},{"failureStage",catalog_?"catalog":"head"},{"closed",!catalog_}});}'
new='''void fail(const char* reason){auto result=history_result_.value_or(Json::object());const auto phase=history_result_?"head":catalog_?"catalog":"head";result["success"]=false;result["reason"]=reason;result["stage"]=phase;result["failureStage"]=phase;if(!result.contains("closed"))result["closed"]=!catalog_;finish(std::move(result));}
 void status(){auto reply=callback([self=shared_from_this()](Json value){self->selected(std::move(value));});try{core_("nano_capellaStatus",Json::array({token_}),reply);}catch(...){reply(nullptr);}}'''
assert old in s;s=s.replace(old,new)
old='self->finish(std::move(result));}),known_,fair_)';new='''if(self->history_first_){self->history_result_=std::move(result);self->status();}else self->finish(std::move(result));}),known_,fair_)''';assert old in s;s=s.replace(old,new)
old='selected_head_=id;\n  if(id==anchor_)';new='''selected_head_=id;
  if(history_first_){
   // History has its own alternating turn under the same total deadline.
   // Status is local authenticated metadata, never a remote admission ACK.
   priority_=id==anchor_||knowledge_->acknowledged(id)?"historical-first-head-confirmed":"historical-first-head-unconfirmed";
   finish(std::move(*history_result_));return;
  }
  if(id==anchor_)''';assert old in s;s=s.replace(old,new)
s=s.replace('std::shared_ptr<CapellaExportCache> exports={}):io_', 'std::shared_ptr<CapellaExportCache> exports={},bool history_first=false):io_')
s=s.replace('exports_(std::move(exports)),fair_(fair)', 'exports_(std::move(exports)),history_first_(history_first),fair_(fair)')
old='auto reply=callback([self=shared_from_this()](Json status){self->selected(std::move(status));});try{core_("nano_capellaStatus",Json::array({token_}),reply);}catch(...){reply(nullptr);}'
assert old in s;s=s.replace(old,'if(history_first_){priority_="historical-first";catalog();}else status();')
s=s.replace('// Prioritize the local authenticated LMD-GHOST head, then preserve the complete', '// Alternate local authenticated LMD-GHOST priority and a dedicated historical turn.\n// Preserve the complete')
p.write_text(s)
p=r/'network/main.cpp';s=p.read_text();old='std::move(known),quantum,fair,export_cache);(*active_head)->start();';new='std::move(known),quantum,fair,export_cache,peer.capella_schedule.next_block_history_first());(*active_head)->start();';assert s.count(old)==1;s=s.replace(old,new)
s=s.replace('capella_generation=std::make_shared<int>(0);peers_.at(id).capella_exports.reset();','capella_generation=std::make_shared<int>(0);peers_.at(id).capella_schedule=CapellaRelaySchedule{};peers_.at(id).capella_exports.reset();')
s=s.replace('capella_generation=std::make_shared<int>(0);it->second.capella_exports.reset();','capella_generation=std::make_shared<int>(0);it->second.capella_schedule=CapellaRelaySchedule{};it->second.capella_exports.reset();')
p.write_text(s)
p=r/'network/test_capella_relay_schedule.cpp';s=p.read_text();mark=' // A simulated catalog completion delay'
new=''' // Block order alternates independently of other categories and peers. Every
 // 20 category opportunities contain a head turn and a historical turn.
 CapellaRelaySchedule order;unsigned blocks=0;for(unsigned i=0;i<10000;++i){if(order.next()==0){check(order.next_block_history_first()==bool(blocks%2),"moving head starved historical block turn");++blocks;}}check(blocks==1000,"missing block opportunities");
 CapellaRelaySchedule independent;check(!independent.next_block_history_first(),"peer inherited historical phase");check(!CapellaRelaySchedule{}.next_block_history_first(),"replacement inherited historical phase");
'''
assert mark in s;s=s.replace(mark,new+mark).replace('4 scheduler scenarios passed','5 scheduler scenarios passed');p.write_text(s)
p=r/'network/CMakeLists.txt';s=p.read_text();s+='''\nadd_executable(nano-capella-head-history-fairness-tests test_capella_head_history_fairness.cpp)
target_include_directories(nano-capella-head-history-fairness-tests PRIVATE ../third_party)
target_compile_features(nano-capella-head-history-fairness-tests PRIVATE cxx_std_20)
target_compile_definitions(nano-capella-head-history-fairness-tests PRIVATE BOOST_ERROR_CODE_HEADER_ONLY NANO_HEAD_HISTORY_FIRST)
target_link_libraries(nano-capella-head-history-fairness-tests PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
add_test(NAME capella_head_history_fairness COMMAND nano-capella-head-history-fairness-tests)
set_tests_properties(capella_head_history_fairness PROPERTIES TIMEOUT 20)
''';p.write_text(s)
