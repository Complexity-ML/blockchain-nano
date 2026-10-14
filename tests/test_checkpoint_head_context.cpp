#include "nano/checkpoint_blob_gc.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "consensus_capella_checkpoint_fragments.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <vector>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/resource.h>
#include <type_traits>
#include <utility>
using namespace nano;
namespace fs=std::filesystem;
namespace {
static_assert(!std::is_default_constructible_v<AuthenticatedCheckpointBlobClosure>);
static_assert(!std::is_reference_v<decltype(std::declval<const AuthenticatedCheckpointBlobClosure&>().head_context())>);
const auto started=std::chrono::steady_clock::now();
void need(bool ok){if(!ok)throw std::runtime_error("Public authenticated head context check failed");}
uint64_t peak_kib(){rusage r{};need(::getrusage(RUSAGE_SELF,&r)==0);
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void checkpoint(){need(std::chrono::steady_clock::now()-started<std::chrono::seconds(60)&&peak_kib()<=131072);}
Bytes digest(std::string_view s){return sha256(Bytes(s.begin(),s.end()));}
template<class F>void refused(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}need(failed);}
std::string read(const fs::path& p){need(fs::is_regular_file(p)&&!fs::is_symlink(p)&&fs::file_size(p)<=1024*1024);std::ifstream in(p,std::ios::binary);need(bool(in));std::string s((std::istreambuf_iterator<char>(in)),{});need(!in.bad()&&s.size()<=1024*1024);return s;}
bool equal(const Phase0ForkSlashingContext& a,const Phase0ForkSlashingContext& b){return a.block_root==b.block_root&&a.registry==b.registry&&a.genesis_root==b.genesis_root&&a.fork.previous_version==b.fork.previous_version&&a.fork.current_version==b.fork.current_version&&a.fork.epoch==b.fork.epoch;}
void emit(const char* name){std::cout<<Json{{"scenario",name},{"result","PASS"},{"newSignaturesCreated",0},{"databaseOpened",false},{"privateInputsRead",false},{"runtimeDutiesAuthorized",false},{"resumeAuthorized",false},{"productionQualified",false}}.dump()<<'\n';}
using Blobs=std::map<Bytes,std::string>;
using Reader=std::function<std::optional<std::string>(const Bytes&,size_t)>;
Reader reader(const Blobs& data){return [&data](const Bytes& id,size_t bytes)->std::optional<std::string>{checkpoint();const auto found=data.find(id);if(found==data.end())return std::nullopt;need(found->second.size()==bytes);return found->second;};}
struct Fixture {CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;Phase0ForkSlashingContext expected;Json identity,execution_head;std::string hot,cold,fragmented;Blobs cold_blobs,fragment_blobs;};
Fixture fixture(const fs::path& dir){
 const auto manifest=read(dir/"manifest.json");need(hex(digest(manifest))=="0xec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6");const auto m=Json::parse(manifest);
 const auto file=[&](const char* name){const auto& item=m.at("files").at(name);const auto relative=item.at("path").get<std::string>();need(fs::path(relative).filename()==relative);const auto bytes=read(dir/relative);need(bytes.size()==item.at("bytes").get<size_t>()&&hex(digest(bytes)).substr(2)==item.at("sha256").get<std::string>());return bytes;};
 const auto profile=file("profile");auto rules=load_capella_fork_profile(profile,digest(profile),1024*1024);const auto maximum=rules.transition.maximum_state_bytes;need(maximum<=SIZE_MAX/4);
 const CapellaStoreSnapshotLimits limits{16*1024*1024,4,64,64,maximum,4*maximum,64,100000};const auto hot=file("initial");const auto store=capella_store_from_snapshot(hot,digest(hot),rules,limits,checkpoint);const auto head=capella_fork_head(store,rules,checkpoint);
 auto context=capella_fork_block_context(head->state,head->header,{rules.transition.consensus.epoch,rules.transition.execution,maximum},rules.transition.consensus.state,checkpoint);const auto network=capella_network_anchor(store);
 Json identity{{"profile",capella_fork_profile(rules)},{"networkAnchor",hex(Bytes(network.begin(),network.end()))},{"genesisValidatorsRoot",hex(Bytes(context.genesis_root.begin(),context.genesis_root.end()))},{"genesisTimeMs",quantity(Int(std::to_string(store.timed.clock.genesis_time_ms)))}};
 Json execution{{"number",quantity(head->execution_block.header.number)},{"hash",hex(head->execution_block.header.hash())},{"stateRoot",hex(head->execution_state.root())}};
 Blobs cold_blobs;const auto reference=capella_store_reference_snapshot(store,rules,limits,[&](const CapellaColdReference& ref,std::string_view bytes){need(ref.digest==digest(bytes)&&bytes.size()==ref.bytes&&cold_blobs.emplace(ref.digest,std::string(bytes)).second);},checkpoint);need(!cold_blobs.empty());
 const auto cold=Json{{"format","nano-capella-cold-checkpoint-v1"},{"cache",Json::array({"0x1","0x1000000"})},{"checkpoint",Json::parse(reference)}}.dump();const auto fragments=capella_checkpoint_detail::fragment_checkpoint(hot,limits.envelope_bytes,limits.blocks,checkpoint);Blobs fragment_blobs;for(const auto& blob:fragments.blobs)need(fragment_blobs.emplace(blob.digest,blob.payload).second);need(!fragment_blobs.empty());
 return {std::move(rules),limits,std::move(context),std::move(identity),std::move(execution),hot,cold,fragments.manifest,std::move(cold_blobs),std::move(fragment_blobs)};
}
Json expected_description(const Fixture& f,const Blobs& blobs){Json refs=Json::array();for(const auto& [id,bytes]:blobs)refs.push_back(Json::array({hex(id),quantity(Int(std::to_string(bytes.size())))}));return {{"identity",f.identity},{"executionHead",f.execution_head},{"references",std::move(refs)}};}
void run(const Fixture& f){
 const Blobs none;const auto hot=authenticate_capella_checkpoint_blob_closure(f.hot,digest(f.hot),f.rules,f.limits,{}, {},checkpoint);
 const auto cold=authenticate_capella_checkpoint_blob_closure(f.cold,digest(f.cold),f.rules,f.limits,reader(f.cold_blobs),{},checkpoint);
 const auto fragments=authenticate_capella_checkpoint_blob_closure(f.fragmented,digest(f.fragmented),f.rules,f.limits,reader(f.fragment_blobs),{},checkpoint);
 need(equal(hot.head_context(),f.expected)&&equal(cold.head_context(),f.expected)&&equal(fragments.head_context(),f.expected));need(hot.description()==expected_description(f,none)&&cold.description()==expected_description(f,f.cold_blobs)&&fragments.description()==expected_description(f,f.fragment_blobs));emit("hot-cold-fragmented-complete-head-context-and-description");
 const auto before=cold.description();auto returned=cold.head_context();returned.registry.validators.clear();returned.registry.slashings.clear();returned.block_root[0]^=1;returned.genesis_root[0]^=1;returned.fork.current_version[0]^=1;auto copy=cold;auto copied=copy.head_context();copied.registry.validators.clear();auto description_copy=copy.description();description_copy.clear();need(equal(copy.head_context(),f.expected)&&equal(cold.head_context(),f.expected)&&cold.description().dump()==before.dump()&&copy.description().dump()==before.dump());emit("owned-context-and-closure-copies-leave-description-byte-exact");
 auto wrong_digest=digest(f.hot);wrong_digest[0]^=1;refused([&]{authenticate_capella_checkpoint_blob_closure(f.hot,wrong_digest,f.rules,f.limits,{}, {},checkpoint);});auto wrong_rules=f.rules;++wrong_rules.slot_duration_ms;refused([&]{authenticate_capella_checkpoint_blob_closure(f.hot,digest(f.hot),wrong_rules,f.limits,{}, {},checkpoint);});auto bad=Json::parse(f.hot);bad.at("blocks").at(0).at(1).at(3)="0x"+std::string(64,'0');const auto malformed=bad.dump();refused([&]{authenticate_capella_checkpoint_blob_closure(malformed,digest(malformed),f.rules,f.limits,{}, {},checkpoint);});need(equal(hot.head_context(),f.expected)&&hot.description()==expected_description(f,none));emit("digest-profile-and-head-state-binding-refused-before-return");
 for(const auto& input:std::vector<std::pair<std::string,const Blobs*>>{{f.cold,&f.cold_blobs},{f.fragmented,&f.fragment_blobs}}){const auto fetch=reader(*input.second);const auto empty=[](const Bytes&,size_t)->std::optional<std::string>{return std::nullopt;};refused([&]{authenticate_capella_checkpoint_blob_closure(input.first,digest(input.first),f.rules,f.limits,empty,{},checkpoint);});const auto corrupt=[&](const Bytes& id,size_t bytes)->std::optional<std::string>{auto row=fetch(id,bytes);need(row&&!row->empty());(*row)[0]^=1;return row;};refused([&]{authenticate_capella_checkpoint_blob_closure(input.first,digest(input.first),f.rules,f.limits,corrupt,{},checkpoint);});}need(cold.description()==before&&equal(cold.head_context(),f.expected));emit("cold-and-fragment-readers-refuse-missing-or-corrupt-blobs");
 for(unsigned version=1;version<=4;++version){Json envelope{{"format","nano-capella-durable-v"+std::to_string(version)},{"store",Json::parse(f.hot)},{"signedVotes",Json::array()}};const auto wire=Bytes(version==3?172:version==4?112:1,0);const auto id=hex(sha256(wire));if(version>=2)envelope["signedSlashings"]=Json::array();if(version>=3)envelope["blsChanges"]=Json::array();if(version==1)envelope["signedVotes"].push_back(Json::array({id,hex(wire)}));if(version==2)envelope["signedSlashings"].push_back(Json::array({id,hex(wire)}));if(version==3)envelope["blsChanges"].push_back(Json::array({"0x0","0x"+std::string(64,'1'),hex(wire)}));if(version==4)envelope["signedExits"]=Json::array({Json::array({id,"0x"+std::string(64,'1'),hex(wire)})});const auto bytes=envelope.dump();refused([&]{authenticate_capella_checkpoint_blob_closure(bytes,digest(bytes),f.rules,f.limits,{}, {},checkpoint);});}emit("durable-v1-v4-malformed-queues-cannot-yield-head-certificate");
 struct Cancelled{};size_t checks=0;const auto counting=[&]{checkpoint();++checks;};const auto counted=authenticate_capella_checkpoint_blob_closure(f.hot,digest(f.hot),f.rules,f.limits,{}, {},counting);need(checks>1&&equal(counted.head_context(),f.expected));for(const size_t stop:std::array<size_t,2>{1,checks}){size_t calls=0;bool cancelled=false;try{authenticate_capella_checkpoint_blob_closure(f.hot,digest(f.hot),f.rules,f.limits,{}, {},[&]{checkpoint();if(++calls==stop)throw Cancelled{};});}catch(const Cancelled&){cancelled=true;}need(cancelled&&calls==stop&&equal(counted.head_context(),f.expected)&&counted.description()==expected_description(f,none));}emit("typed-cancellation-at-first-and-final-checkpoint-yields-no-new-result");
}
}
int main(int argc,char** argv){try{need(argc==3&&std::string(argv[1])=="--self-test");run(fixture(fs::path(argv[2])));emit("authenticated-head-context-public-summary");return 0;}catch(...){std::cerr<<"Public authenticated head context test refused\n";return 1;}}
