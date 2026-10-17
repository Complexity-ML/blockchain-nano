#include "supervisor.hpp"
#include "json_stream.hpp"
#include "block_download.hpp"
#include "block_reference.hpp"
#include "bootstrap.hpp"
#include "peer_status.hpp"
#include "inbound_budget.hpp"
#include "gossip_progress.hpp"
#include "bft_vote_knowledge.hpp"
#include "timed_reply.hpp"
#include "altair_core_recovery.hpp"
#include "altair_stream_session.hpp"
#include "altair_stream_sender.hpp"
#include "capella_send_pacer.hpp"
#include "altair_peer.hpp"
#include "altair_archive_relay.hpp"
#include "capella_catalog_relay.hpp"
#include "capella_attestation_delivery.hpp"
#include "capella_head_catalog_relay.hpp"
#include "capella_relay_schedule.hpp"
#include "capella_peer_core.hpp"
#include "capella_block_delivery.hpp"
#include "altair_vote_delivery.hpp"
#include "peer_relay_budget.hpp"
#include <sstream>
#include <libp2p/network/impl/dnsaddr_resolver_impl.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/security/noise.hpp>
#include <libp2p/transport/tcp.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <fcntl.h>
#include <unistd.h>
#include <limits>
using Host=std::shared_ptr<libp2p::Host>;
using PeerInfo=libp2p::peer::PeerInfo;
const std::string reliable_gossip="/nano/txgossip/2.0.0";
const std::string bft="/nano/bft/1.0.0";
const std::string announce_ref="/nano/blockannounce/2.0.0";
const std::string bft_manifest="/nano/bft/2.0.0",bft_parts="/nano/bftparts/1.0.0";
const std::string blockparts="/nano/blockparts/1.0.0";
const std::string handshake="/nanopy/handshake/1.0.0",blocksync="/nanopy/blocksync/1.0.0",announce="/nanopy/blockannounce/1.0.0",gossip="/nanopy/txgossip/1.0.0",chainsync="/nanopy/chainsync/1.0.0";
libp2p::crypto::KeyPair identity(const std::filesystem::path& path){
 std::vector<uint8_t> secret(32),pub(32);if(std::filesystem::exists(path)){std::ifstream in(path,std::ios::binary);in.read(reinterpret_cast<char*>(secret.data()),32);if(in.gcount()!=32||in.peek()!=EOF)throw std::runtime_error("Invalid P2P identity file");}
 else{if(path.has_parent_path())std::filesystem::create_directories(path.parent_path());if(RAND_bytes(secret.data(),secret.size())!=1)throw std::runtime_error("P2P randomness failed");int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)throw std::runtime_error("Cannot create P2P identity");auto n=::write(fd,secret.data(),secret.size());int sync=fsync(fd);close(fd);if(n!=32||sync!=0)throw std::runtime_error("Cannot persist P2P identity");}
 auto* key=EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,nullptr,secret.data(),secret.size());if(!key)throw std::runtime_error("Invalid Ed25519 identity");size_t size=pub.size();int ok=EVP_PKEY_get_raw_public_key(key,pub.data(),&size);EVP_PKEY_free(key);if(ok!=1||size!=32)throw std::runtime_error("Cannot derive P2P identity");using namespace libp2p::crypto;return {PublicKey{{Key::Type::Ed25519,std::move(pub)}},PrivateKey{{Key::Type::Ed25519,std::move(secret)}}};
}
class Network {
 asio::io_context& io_;Host host_;Core core_;asio::steady_timer timer_,bft_timer_,consensus_relay_timer_;
 std::chrono::milliseconds consensus_relay_interval_{100};
 std::shared_ptr<AltairCoreRecovery> altair_recovery_;std::shared_ptr<AltairCoreDispatch> altair_dispatch_;
 std::shared_ptr<AltairMessageIngress> altair_ingress_;Json altair_advertisement_;
 Json altair_advertisement()const{return altair_ingress_&&altair_dispatch_&&!altair_dispatch_->blocked()?altair_advertisement_:Json(nullptr);}
 bool bellatrix_mode_=false,capella_mode_=false;
 Json capella_relay_;std::string capella_session_;
 bool execution_consensus()const{return bellatrix_mode_||capella_mode_;}
 bool beacon_status_pending_=false;std::optional<uint64_t> beacon_head_slot_;std::chrono::steady_clock::time_point beacon_head_seen_{};
 std::string consensus_protocol()const{return capella_mode_?"/nano/capella/1.0.0":bellatrix_mode_?"/nano/bellatrix/1.0.0":altair_protocol;}
 static constexpr const char* session_protocol="/nano/capella/session/1.0.0";
 bool session_peer(const Json& remote)const{return capella_mode_&&consensus_peer(remote)&&remote.value("capellaStreamProtocol",Json())==session_protocol;}
 std::string consensus_field()const{return capella_mode_?"capella":bellatrix_mode_?"bellatrix":"altair";}
 void consensus_call(std::string method,Json params,Reply reply){
  if(execution_consensus()&&method.starts_with("nano_altair"))method.replace(0,11,capella_mode_?"nano_capella":"nano_bellatrix");
  if(execution_consensus()&&(method=="nano_bellatrixStatus"||method=="nano_capellaStatus")){
   core_.call(method,std::move(params),[reply=std::move(reply)](Json result)mutable{if(!result.is_object()||result.value("executionBound",Json())!=true){reply(nullptr);return;}reply(std::move(result));});
  }else core_.call(method,std::move(params),std::move(reply));
 }
 bool consensus_peer(const Json& remote)const{if(capella_mode_){const auto session=remote.value("capellaSession",Json());if(!session.is_string()||session.get_ref<const std::string&>().size()!=66||!session.get_ref<const std::string&>().starts_with("0x")||!std::all_of(session.get_ref<const std::string&>().begin()+2,session.get_ref<const std::string&>().end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))return false;}return compatible_altair_peer(remote,status_,consensus_field(),consensus_protocol());}
 bool altair_started_=false;PeerRelayBudget altair_relay_budget_{4};std::string altair_last_peer_;
 void recover_altair(){
  if(altair_started_||(status_.value("altairIpc",Json())!=true&&status_.value("bellatrixIpc",Json())!=true&&status_.value("capellaIpc",Json())!=true))return;
  if(unsigned(status_.value("altairIpc",false))+unsigned(status_.value("bellatrixIpc",false))+unsigned(status_.value("capellaIpc",false))!=1)throw std::runtime_error("Ambiguous consensus IPC");
  bellatrix_mode_=status_.value("bellatrixIpc",false);capella_mode_=status_.value("capellaIpc",false);altair_started_=true;
  if(report_token_.empty()){std::cerr<<"Altair IPC recovery requires the supervisor token\n";return;}
  std::ostringstream chain;chain<<"0x"<<std::hex<<status_.at("chain_id").get<uint64_t>();
  auto call=[this](const std::string& method,Json params,Reply reply){consensus_call(method,std::move(params),std::move(reply));};
  // Local IPC recovery budgets, independent of validator registry size.
  // Peer admission additionally requires explicit core configuration.
  altair_recovery_=std::make_shared<AltairCoreRecovery>(io_,call,report_token_,chain.str(),status_.at("genesis_hash").get<std::string>(),3,std::chrono::milliseconds(100),std::chrono::seconds(30),[this,call](Json result){
   altair_recovery_.reset();
   if(result.is_null()){std::cerr<<Json{{"event","altair-ipc-recovery-failed"},{"networkProtocolActive",false}}.dump()<<std::endl;return;}
   try{
    const auto& w=result.at("wire");AltairWireProfile profile{result.at("chainId").get<std::string>(),result.at("genesisHash").get<std::string>(),result.at("forkDigest").get<std::string>(),w.at("frameBytes").get<size_t>(),w.at("blockBytes").get<size_t>(),w.at("attestationBytes").get<size_t>(),w.at("slashingBytes").get<size_t>()};
    if(execution_consensus())profile.consensus_identity=result.at("consensusIdentity").get<std::string>();
    profile.bls_changes=capella_mode_;profile.sync_messages=capella_mode_;profile.proposer_slashings=capella_mode_;
    altair_dispatch_=std::make_shared<AltairCoreDispatch>(io_,call,profile,report_token_,result.at("session").get<std::string>(),result.at("highWater").get<uint64_t>(),result.at("workerJobs").get<size_t>(),3,std::chrono::milliseconds(100),trace_network_?AltairCoreDispatch::Trace{[](const char* phase,uint64_t high_water,size_t pending){std::cout<<Json{{"event","test-core-dispatch-transition"},{"phase",phase},{"highWater",high_water},{"pending",pending},{"timeMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()}}.dump()<<std::endl;}}:AltairCoreDispatch::Trace{});
    if(capella_mode_){capella_relay_=result.at("capellaRelay");capella_session_=result.at("session").get<std::string>();status_["capellaSession"]=capella_session_;}
    if(result.at("networkAdmissionEnabled")==true){
     altair_ingress_=std::make_shared<AltairMessageIngress>(profile,inbound_budget_);
     altair_advertisement_={{"protocol",consensus_protocol()},{"consensusIdentity",result.at("consensusIdentity")},{"forkDigest",profile.fork_digest}};
     status_[consensus_field()]=altair_advertisement();
     host_->setProtocolHandler({consensus_protocol()},[this](libp2p::StreamAndProtocol stream){inbound(consensus_protocol(),std::move(stream.stream));});
     if(capella_mode_){status_["capellaStreamProtocol"]=session_protocol;host_->setProtocolHandler({session_protocol},[this](libp2p::StreamAndProtocol stream){inbound(session_protocol,std::move(stream.stream));});}
     std::cout<<Json{{"event","altair-ingress-ready"},{"consensusIdentity",result.at("consensusIdentity")},{"executionBound",execution_consensus()}}.dump()<<std::endl;
    }
    std::cout<<Json{{"event","altair-ipc-ready"},{"profile",consensus_field()},{"consensusIdentity",result.at("consensusIdentity")},{"networkProtocolActive",bool(altair_ingress_)}}.dump()<<std::endl;
   }catch(const std::exception&){if(trace_network_)std::cerr<<Json{{"event","test-core-recovery-stage"},{"stage","setup"},{"outcome","validation-rejected"}}.dump()<<std::endl;altair_ingress_.reset();altair_advertisement_=nullptr;status_[consensus_field()]=nullptr;if(altair_dispatch_)altair_dispatch_->stop();std::cerr<<Json{{"event","altair-ipc-recovery-failed"},{"networkProtocolActive",false}}.dump()<<std::endl;}
  },trace_network_?AltairCoreRecovery::Trace{[](const Json& row){if(row.at("outcome")=="request")return;auto record=row;record["event"]="test-core-recovery-stage";std::cerr<<record.dump()<<std::endl;}}:AltairCoreRecovery::Trace{});altair_recovery_->start();
 }
 Json status_;uint64_t cursor_=0;bool initialized_=false;bool previous_bft_page_=false;bool download_busy_=false,bft_download_busy_=false;
 std::string report_token_,event_epoch_;bool event_polling_=false;unsigned reference_trace_count_=0;InboundBudget inbound_budget_;
 bool drop_altair_vote_reply_=std::getenv("NANO_TEST_DROP_ALTAIR_VOTE_REPLY_ONCE")!=nullptr;
 bool drop_altair_reply_=std::getenv("NANO_TEST_DROP_ALTAIR_REPLY_ONCE")!=nullptr;
 bool drop_block_fragment_=std::getenv("NANO_TEST_DROP_BLOCK_FRAGMENT_ONCE")!=nullptr;
 bool trace_network_=std::getenv("NANO_TEST_NETWORK_TRACE")!=nullptr;
 unsigned attestation_trace_count_=0;
 std::map<std::string,uint64_t> budget_rejections_;uint64_t failed_status_=0;unsigned trace_ticks_=0;
 unsigned retry_trace_count_=0,periodic_trace_count_=0,vote_delta_trace_count_=0;
 std::map<std::string,std::shared_ptr<GossipProgress>> gossip_progress_;size_t gossip_active_=0;std::string gossip_last_;bool drop_gossip_reply_=std::getenv("NANO_TEST_DROP_GOSSIP_REPLY_ONCE")!=nullptr;
 struct Peer {PeerInfo info;Json status;std::array<std::shared_ptr<CapellaCatalogSelection>,CapellaRelaySchedule::category_count> capella_selections{};std::shared_ptr<CapellaDependencyPreparation> capella_preparation;CapellaRelaySchedule capella_schedule;CapellaSendPacer capella_pacer;std::shared_ptr<AltairStreamSender> capella_sender;std::shared_ptr<int> capella_generation=std::make_shared<int>(0);std::shared_ptr<AltairRelayProgress> capella_blocks,capella_votes,capella_slashings,capella_bls_changes,capella_sync_messages,capella_exits,capella_proposer_slashings;std::shared_ptr<AltairRelayProgress> altair_progress=std::make_shared<AltairRelayProgress>(),altair_vote_progress=std::make_shared<AltairRelayProgress>();std::shared_ptr<AltairRelayProgress> slashing_progress=std::make_shared<AltairRelayProgress>(),exit_progress=std::make_shared<AltairRelayProgress>();std::chrono::steady_clock::time_point status_seen=std::chrono::steady_clock::now();unsigned next_bellatrix_kind=0;bool next_altair_vote=false;std::shared_ptr<BftVoteKnowledge> bft_known=std::make_shared<BftVoteKnowledge>();bool syncing=false;bool previous_bft_page=false;std::chrono::steady_clock::time_point fragment_retry{};std::shared_ptr<CapellaExportCache> capella_exports;};
 std::map<std::string,Peer> peers_;std::vector<PeerInfo> seeds_;std::set<std::string> connecting_,polling_,bft_polling_;
 std::vector<libp2p::multi::Multiaddress> bootstrap_;
 std::shared_ptr<libp2p::network::DnsaddrResolver> resolver_;
 std::map<size_t,std::vector<PeerInfo>> resolved_;
 std::set<size_t> resolving_;
 std::map<std::string,std::chrono::steady_clock::time_point> retry_after_;
 std::chrono::steady_clock::time_point resolve_due_{};
 void refresh_bootstrap(){
  auto now=std::chrono::steady_clock::now();if(now<resolve_due_)return;resolve_due_=now+std::chrono::seconds(60);
  for(size_t index=0;index<bootstrap_.size();++index){
   if(!resolving_.insert(index).second)continue;
   std::make_shared<BootstrapLookup>(resolver_,bootstrap_[index],[this,index](std::vector<PeerInfo> peers){
    resolving_.erase(index);if(!peers.empty())resolved_.insert_or_assign(index,std::move(peers));
    seeds_.clear();std::set<std::string> seen;
    for(const auto& [source,entries]:resolved_)for(const auto& peer:entries){
     auto id=peer.id.toBase58();if(seen.insert(id).second){if(seeds_.size()>=256)break;seeds_.push_back(peer);}
     else{auto it=std::find_if(seeds_.begin(),seeds_.end(),[&](const auto& known){return known.id==peer.id;});if(it!=seeds_.end())for(const auto& addr:peer.addresses)if(it->addresses.size()<64&&std::find(it->addresses.begin(),it->addresses.end(),addr)==it->addresses.end())it->addresses.push_back(addr);}
    }
    for(auto it=retry_after_.begin();it!=retry_after_.end();)if(!seen.contains(it->first))it=retry_after_.erase(it);else ++it;
   })->start();
  }
 }

 bool valid(const Json& status)const{return valid_peer_status(status,status_);}
 void exchange(PeerInfo info,std::string protocol,Json request,Reply reply){
  const auto limit=(protocol==handshake||protocol==chainsync)?peer_status_frame_bytes:JsonFrame::maximum_size;
  auto pending=std::make_shared<TimedReply>(io_,std::move(reply));pending->start();
  host_->newStream(info,{protocol},[this,request=std::move(request),pending,limit](auto result){
   if(!result){pending->finish(nullptr);return;}auto stream=result.value().stream;
   if(!pending->on_cancel([stream]{stream->reset();}))return;
   write_json(io_,stream,request,[this,stream,pending,limit](bool ok){
    if(!pending->active())return;
    if(!ok){stream->reset();pending->finish(nullptr);return;}
    read_json(io_,stream,[stream,pending](Json value){close_stream(stream);pending->finish(std::move(value));},limit);
   },limit);
  });
 }

 void send_altair(PeerInfo peer,Json frame,Reply reply){
  // Diagnostics only: timestamps never affect admission, pacing or deadlines.
  const auto start=std::chrono::steady_clock::now();
  const auto phases=trace_network_?std::make_shared<std::array<int64_t,2>>(std::array<int64_t,2>{-1,-1}):nullptr;
  const auto elapsed=[start]{return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();};
  const auto kind=phases?frame.value("kind",std::string{}):std::string{};
  const auto peer_id=peer.id.toBase58();
  auto completion=[phases,elapsed,kind,peer_id,reply=std::move(reply)](Json value)mutable{
   if(phases)try{std::cout<<Json{{"event","test-consensus-send-completion"},{"timeMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()},{"peerId",peer_id},{"kind",kind},{"streamOpenMs",(*phases)[0]},{"writeCompletedMs",(*phases)[1]},{"elapsedMs",elapsed()},{"outcome",value.is_object()&&value.size()==1&&value.value("accepted",Json())==true?"accepted":value.is_object()&&value.size()==1&&value.value("accepted",Json())==false?"rejected":"unconfirmed"}}.dump()<<std::endl;}catch(...){/* Observability must not alter delivery. */}
   reply(std::move(value));
  };
  const auto maximum=altair_ingress_->frame_limit();
  auto found=peers_.find(peer_id);
  if(found!=peers_.end()&&session_peer(found->second.status)){
   auto& remote=found->second;
   if(!remote.capella_sender){
    const auto generation=remote.capella_generation;
    remote.capella_sender=std::make_shared<AltairStreamSender>(io_,[this,peer](auto done){host_->newStream(peer,{session_protocol},[done=std::move(done)](auto result){done(result?result.value().stream:Stream{});});},[this,peer_id,generation]{auto it=peers_.find(peer_id);return it!=peers_.end()&&it->second.capella_generation==generation&&session_peer(it->second.status)&&altair_dispatch_&&!altair_dispatch_->blocked();},maximum);
   }
   remote.capella_sender->send(std::move(frame),std::move(completion),[phases,elapsed](unsigned phase){if(phases)(*phases)[phase]=elapsed();});return;
  }
  auto pending=std::make_shared<TimedReply>(io_,std::move(completion));pending->start();
  host_->newStream(peer,{consensus_protocol()},[this,pending,maximum,phases,elapsed,frame=std::move(frame)](auto result){
   if(!result){pending->finish(nullptr);return;}if(phases)(*phases)[0]=elapsed();auto stream=result.value().stream;if(!pending->on_cancel([stream]{stream->reset();}))return;
   std::make_shared<JsonWrite>(io_,stream,frame,[this,stream,pending,phases,elapsed](bool ok){if(!pending->active())return;if(!ok){stream->reset();pending->finish(nullptr);return;}if(phases)(*phases)[1]=elapsed();
    read_json_frame(io_,stream,256,[stream,pending](auto raw){close_stream(stream);auto result=raw?Json::parse(*raw,nullptr,false):Json(nullptr);pending->finish(result.is_discarded()?Json(nullptr):std::move(result));});
   },maximum)->start();
  });
 }

 void poll_capella_archives(){
  if(!altair_relay_budget_.available()||!altair_ingress_||!altair_dispatch_||altair_dispatch_->blocked()||peers_.empty())return;
  auto it=peers_.upper_bound(altair_last_peer_);if(it==peers_.end())it=peers_.begin();const auto start=it;
  do{
   if(consensus_peer(it->second.status)&&!altair_relay_budget_.busy(it->first)){
    auto& peer=it->second;const auto id=it->first;const auto info=peer.info;const auto generation=peer.capella_generation;
    const bool preparing=peer.capella_preparation&&!peer.capella_preparation->target.empty();
    const auto step=peer.capella_schedule.next(preparing);const std::string kind=CapellaRelaySchedule::name(step);
    auto& cache=step==0?peer.capella_blocks:step==1?peer.capella_votes:step==2?peer.capella_slashings:step==3?peer.capella_bls_changes:step==4?peer.capella_sync_messages:step==5?peer.capella_exits:peer.capella_proposer_slashings;
    if(!peer.capella_selections[step])peer.capella_selections[step]=std::make_shared<CapellaCatalogSelection>();const auto selection=peer.capella_selections[step];
    if(!cache)cache=std::make_shared<AltairRelayProgress>(capella_relay_.at("acknowledgedRoots").get<size_t>());const auto knowledge=cache;
    if(!altair_relay_budget_.acquire(id))return;altair_last_peer_=id;
    auto alive=std::make_shared<bool>(true);auto current=[this,id,generation,alive]{const auto p=peers_.find(id);return *alive&&p!=peers_.end()&&p->second.capella_generation==generation&&altair_ingress_&&altair_dispatch_&&!altair_dispatch_->blocked()&&consensus_peer(p->second.status);};
    auto send=[this,id,info,current](Json frame,Reply reply){
     if(!current()){reply(nullptr);return;}
     auto timer=std::make_shared<asio::steady_timer>(io_);timer->expires_at(peers_.at(id).capella_pacer.reserve(std::chrono::steady_clock::now()));
     timer->async_wait([this,timer,info,current,frame=std::move(frame),reply=std::move(reply)](auto error)mutable{
      if(error||!current()){reply(nullptr);return;}send_altair(info,std::move(frame),[current,reply=std::move(reply)](Json result)mutable{reply(current()?std::move(result):Json(nullptr));});
     });
    };
    auto core=capella_peer_core([this](const std::string& method,Json params,Reply reply){core_.call(method,std::move(params),std::move(reply));},current);
    auto active=std::make_shared<std::shared_ptr<CapellaBlockDelivery>>();
    auto active_vote=std::make_shared<std::shared_ptr<CapellaAttestationDelivery>>();
    auto active_head=std::make_shared<std::shared_ptr<CapellaHeadCatalogRelay>>();
    if(kind=="attestation"&&!peer.capella_blocks)peer.capella_blocks=std::make_shared<AltairRelayProgress>(capella_relay_.at("acknowledgedRoots").get<size_t>());
    const auto block_knowledge=peer.capella_blocks;
    if(!peer.capella_preparation)peer.capella_preparation=std::make_shared<CapellaDependencyPreparation>();
    if(!peer.capella_exports)peer.capella_exports=std::make_shared<CapellaExportCache>(capella_relay_.at("dependencyBlocks").get<size_t>(),capella_relay_.at("dependencyBytes").get<size_t>());
    const auto export_cache=peer.capella_exports;
    const bool fair=capella_relay_.at("acknowledgedRoots").get<size_t>()>=capella_relay_.at("maximumEntries").get<size_t>();
    auto quantum=(kind=="block"||kind=="attestation")&&fair?std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{capella_relay_.at("pageEntries").get<size_t>()}):nullptr;
    auto deliver=[this,core,send,current,knowledge,kind,active,active_vote,block_knowledge,quantum,export_cache](std::string root,Json frame,Reply reply){
     if(!current()){reply(nullptr);return;}if(knowledge->acknowledged(root)){reply(Json{{"accepted",true}});return;}
     if(kind=="block"){
      *active=std::make_shared<CapellaBlockDelivery>(io_,core,send,report_token_,capella_relay_.at("anchorRoot").get<std::string>(),root,altair_ingress_,knowledge,capella_relay_.at("dependencyBlocks").get<size_t>(),capella_relay_.at("dependencyBytes").get<size_t>(),std::move(reply),quantum,export_cache);(*active)->start();
     }else if(kind=="attestation"){
      *active_vote=std::make_shared<CapellaAttestationDelivery>(io_,core,send,report_token_,capella_relay_.at("anchorRoot").get<std::string>(),std::move(frame),altair_ingress_,block_knowledge,capella_relay_.at("dependencyBlocks").get<size_t>(),capella_relay_.at("dependencyBytes").get<size_t>(),[knowledge,root,reply=std::move(reply)](Json result)mutable{if(result==Json{{"accepted",true}})knowledge->acknowledge(root);reply(std::move(result));},quantum,export_cache,trace_network_?CapellaAttestationDelivery::Trace{[this](const Json& row){if(attestation_trace_count_>=256)return;++attestation_trace_count_;auto record=row;record["event"]="test-capella-attestation-delivery";std::cerr<<record.dump()<<std::endl;}}:CapellaAttestationDelivery::Trace{});(*active_vote)->start();
     }else send(std::move(frame),[knowledge,root,reply=std::move(reply)](Json result)mutable{if(result.is_object()&&result.size()==1&&result.value("accepted",Json())==true)knowledge->acknowledge(root);reply(std::move(result));});
    };
    auto complete=[this,id,kind,preparing,active,active_vote,active_head,alive](Json result){*alive=false;if(*active)(*active)->cancel();if(*active_vote)(*active_vote)->cancel();if(*active_head)(*active_head)->cancel();altair_relay_budget_.release(id);if(trace_network_){result["event"]="test-capella-catalog-relay";result["preparationPriority"]=preparing;result["peerId"]=id;result["kind"]=kind;std::cout<<result.dump()<<std::endl;}};
    auto known=[current,knowledge](const std::string& root){return current()&&knowledge->acknowledged(root);};
    if(kind=="block"){
     *active_head=std::make_shared<CapellaHeadCatalogRelay>(io_,core,deliver,send,report_token_,capella_relay_.at("anchorRoot").get<std::string>(),altair_ingress_,knowledge,capella_relay_.at("dependencyBlocks").get<size_t>(),capella_relay_.at("dependencyBytes").get<size_t>(),capella_relay_.at("pageEntries").get<size_t>(),capella_relay_.at("maximumEntries").get<size_t>(),std::move(complete),std::move(known),quantum,fair,export_cache,peer.capella_schedule.next_block_history_first(),selection,peer.capella_preparation);(*active_head)->start(CapellaCatalogRelay::default_timeout,consensus_relay_interval_);
    }else std::make_shared<CapellaCatalogRelay>(io_,core,deliver,report_token_,kind,altair_ingress_,capella_relay_.at("pageEntries").get<size_t>(),capella_relay_.at("maximumEntries").get<size_t>(),std::move(complete),std::move(known),fair,selection)->start(CapellaCatalogRelay::default_timeout,consensus_relay_interval_);return;
   }
   if(++it==peers_.end())it=peers_.begin();
  }while(it!=start);
 }
 // Empty categories must not consume an entire relay interval: with four
 // kinds, one vote per four 100ms ticks cannot drain a two-epoch replay of
 // 64 votes/epoch in 48s. At most one sweep per attempt and one attempt per peer.
 // Slow peers do not monopolize all outbound work; the global budget is bounded.
 void poll_altair_archives(unsigned remaining=4){
  if(capella_mode_){poll_capella_archives();return;}
  if(capella_mode_||!altair_relay_budget_.available()||!altair_ingress_||!altair_dispatch_||altair_dispatch_->blocked()||peers_.empty())return;
  auto start=peers_.upper_bound(altair_last_peer_);if(start==peers_.end())start=peers_.begin();auto it=start;
  do{
   if(consensus_peer(it->second.status)&&!altair_relay_budget_.busy(it->first)){
    const auto peer=it->second.info;const auto step=bellatrix_mode_?it->second.next_bellatrix_kind++%4:unsigned(it->second.next_altair_vote);const bool vote=step==1,proof=step==2,exit=step==3;it->second.next_altair_vote=!vote;const auto kind=exit?AltairWireKind::voluntary_exit:proof?AltairWireKind::attester_slashing:vote?AltairWireKind::attestation:AltairWireKind::block;const auto progress=exit?it->second.exit_progress:proof?it->second.slashing_progress:vote?it->second.altair_vote_progress:it->second.altair_progress;altair_last_peer_=it->first;if(!altair_relay_budget_.acquire(it->first))return;
    std::make_shared<AltairArchiveRelay>(io_,[this](const auto& method,auto params,auto reply){consensus_call(method,std::move(params),std::move(reply));},[this,peer](auto frame,auto reply){send_altair(peer,std::move(frame),std::move(reply));},report_token_,altair_ingress_,progress,[this,progress,vote,proof,exit,remaining,before=progress->cursor,id=it->first](Json result){altair_relay_budget_.release(id);
     const auto completed_at_ms=trace_network_?std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count():0;
     if(trace_network_&&(!result.value("success",false)||result.value("elapsedMs",int64_t(0))>500))
      std::cout<<Json{{"event","test-consensus-relay-attempt"},{"timeMs",completed_at_ms},{"peerId",id},{"kind",exit?"exit":proof?"slashing":vote?"vote":"block"},{"success",result.value("success",false)},{"stage",result.value("stage","")},{"stageUs",result.value("stageUs",Json::object())},{"elapsedMs",result.value("elapsedMs",int64_t(0))},{"cursor",progress->cursor},{"dispatchBlocked",altair_dispatch_->blocked()}}.dump()<<std::endl;
     const bool empty=result.value("success",false)&&progress->cursor==before;if(bellatrix_mode_&&empty&&remaining>1)asio::post(io_,[this,remaining]{poll_altair_archives(remaining-1);});if(trace_network_&&result.value("success",false)&&progress->cursor!=before)std::cout<<Json{{"event",exit?"test-bellatrix-exit-relayed":proof?"test-bellatrix-slashing-relayed":vote?(result.value("stage","")=="known"?"test-altair-vote-known":"test-altair-vote-relayed"):"test-altair-block-relayed"},{"timeMs",completed_at_ms},{"peerId",id},{"root",progress->cursor}}.dump()<<std::endl;},kind)->start();return;
   }
   if(++it==peers_.end())it=peers_.begin();
  }while(it!=start);
 }
 void send(PeerInfo info,const std::string& protocol,Json value){host_->newStream(info,{protocol},[this,value=std::move(value)](auto result){if(!result)return;auto stream=result.value().stream;write_json(io_,stream,value,[stream](bool){close_stream(stream);});});}
 void add(PeerInfo info,const Json& status){if(!valid(status)||peers_.size()>=50)return;auto projected=project_peer_status(status);auto id=info.id.toBase58();bool fresh=!peers_.contains(id);observe_gossip_session(gossip_progress_[id],fresh?Json::object():peers_.at(id).status,status);if(fresh)peers_.emplace(id,Peer{std::move(info),std::move(projected)});else{if((peers_.at(id).status.value("gossip_epoch",Json())!=status.value("gossip_epoch",Json())||peers_.at(id).status.value("capellaStreamProtocol",Json())!=status.value("capellaStreamProtocol",Json())||peers_.at(id).status.value("capellaSession",Json())!=status.value("capellaSession",Json()))){if(peers_.at(id).capella_sender)peers_.at(id).capella_sender->stop();peers_.at(id).capella_sender.reset();peers_.at(id).capella_generation=std::make_shared<int>(0);peers_.at(id).capella_schedule=CapellaRelaySchedule{};peers_.at(id).capella_selections={};peers_.at(id).capella_preparation.reset();peers_.at(id).capella_exports.reset();peers_.at(id).capella_blocks.reset();peers_.at(id).capella_votes.reset();peers_.at(id).capella_slashings.reset();peers_.at(id).capella_bls_changes.reset();peers_.at(id).capella_sync_messages.reset();peers_.at(id).capella_exits.reset();peers_.at(id).capella_proposer_slashings.reset();peers_.at(id).bft_known=std::make_shared<BftVoteKnowledge>();peers_.at(id).altair_progress=std::make_shared<AltairRelayProgress>();peers_.at(id).altair_vote_progress=std::make_shared<AltairRelayProgress>();peers_.at(id).slashing_progress=std::make_shared<AltairRelayProgress>();peers_.at(id).exit_progress=std::make_shared<AltairRelayProgress>();}peers_.at(id).info=std::move(info);peers_.at(id).status=std::move(projected);}peers_.at(id).status_seen=std::chrono::steady_clock::now();if(fresh)std::cout<<Json{{"event","peer"},{"peerId",id},{"height",status["best_block"]}}.dump()<<std::endl;}
 void import_blocks(const std::string& id,std::shared_ptr<Json> blocks,size_t index){
  if(index>=blocks->size()){if(auto it=peers_.find(id);it!=peers_.end())it->second.syncing=false;return;}
  core_.call("validator_submitBlock",Json::array({(*blocks)[index]}),[this,id,blocks,index](Json result){if(result.is_object()&&result.value("success",false))import_blocks(id,blocks,index+1);else{if(auto it=peers_.find(id);it!=peers_.end())it->second.syncing=false;std::cerr<<"Rejected block received from peer\n";}});
 }
 void sync(const std::string& id){if(execution_consensus())return;auto it=peers_.find(id);if(it==peers_.end()||it->second.syncing||it->second.status["best_block"]<=status_["best_block"])return;
  if(!report_token_.empty()){
   if(download_busy_||std::chrono::steady_clock::now()<it->second.fragment_retry)return;
   download_busy_=true;it->second.syncing=true;auto info=it->second.info;auto height=status_.at("best_block").get<uint64_t>();
   if(height==UINT64_MAX){download_busy_=false;it->second.syncing=false;return;}
   std::make_shared<BlockDownload>(io_,core_,report_token_,height+1,[this,info](Json request,Reply reply){exchange(info,blockparts,std::move(request),std::move(reply));},[this,id](bool ok,Json result){
    download_busy_=false;if(auto found=peers_.find(id);found!=peers_.end()){found->second.syncing=false;found->second.fragment_retry=std::chrono::steady_clock::now()+std::chrono::seconds(1);}
    if(trace_network_){result["event"]=ok?"test-block-fragments-imported":"test-block-fragments-failed";std::cout<<result.dump()<<std::endl;}
   })->start();return;
  }
  it->second.syncing=true;
  exchange(it->second.info,blocksync,{{"start",status_["best_block"]}},[this,id](Json result){if(result.is_object()&&result.contains("blocks")&&result["blocks"].is_array()&&result["blocks"].size()<=50)import_blocks(id,std::make_shared<Json>(std::move(result["blocks"])),0);else if(auto it=peers_.find(id);it!=peers_.end())it->second.syncing=false;});
 }
 void poll_status(const std::string& id){
  auto found=peers_.find(id);if(found==peers_.end()||!polling_.insert(id).second)return;auto info=found->second.info;
  exchange(info,chainsync,Json::object(),[this,id](Json remote){polling_.erase(id);auto it=peers_.find(id);if(it==peers_.end())return;if(!valid(remote)){if(trace_network_)++failed_status_;if(it->second.capella_sender)it->second.capella_sender->stop();peers_.erase(it);gossip_progress_.erase(id);return;}remote=project_peer_status(remote);auto previous=gossip_progress_[id];observe_gossip_session(gossip_progress_[id],it->second.status,remote);if(trace_network_&&previous!=gossip_progress_[id]&&retry_trace_count_<8){++retry_trace_count_;std::cout<<Json{{"event","test-gossip-remote-session-reset"},{"peerId",id},{"oldEpoch",it->second.status.value("gossip_epoch",Json())},{"newEpoch",remote.value("gossip_epoch",Json())}}.dump()<<std::endl;}if((it->second.status.value("gossip_epoch",Json())!=remote.value("gossip_epoch",Json())||it->second.status.value("capellaStreamProtocol",Json())!=remote.value("capellaStreamProtocol",Json())||it->second.status.value("capellaSession",Json())!=remote.value("capellaSession",Json()))){if(it->second.capella_sender)it->second.capella_sender->stop();it->second.capella_sender.reset();it->second.capella_generation=std::make_shared<int>(0);it->second.capella_schedule=CapellaRelaySchedule{};peers_.at(id).capella_selections={};it->second.capella_preparation.reset();it->second.capella_exports.reset();it->second.capella_blocks.reset();it->second.capella_votes.reset();it->second.capella_slashings.reset();it->second.capella_bls_changes.reset();it->second.capella_sync_messages.reset();it->second.capella_exits.reset();it->second.capella_proposer_slashings.reset();it->second.bft_known=std::make_shared<BftVoteKnowledge>();it->second.altair_progress=std::make_shared<AltairRelayProgress>();it->second.altair_vote_progress=std::make_shared<AltairRelayProgress>();it->second.slashing_progress=std::make_shared<AltairRelayProgress>();it->second.exit_progress=std::make_shared<AltairRelayProgress>();}it->second.status=std::move(remote);it->second.status_seen=std::chrono::steady_clock::now();sync(id);});
 }
 Json vote_delta(Json local,const std::shared_ptr<BftVoteKnowledge>& knowledge){
  auto result=knowledge->filter(local);
  if(trace_network_&&vote_delta_trace_count_<20&&local.is_object()&&local.contains("votes")&&local["votes"].is_array()&&result["votes"].size()<local["votes"].size()){
   ++vote_delta_trace_count_;std::cout<<Json{{"event","test-bft-vote-delta"},{"height",local["height"]},{"round",local["round"]},{"votesBefore",local["votes"].size()},{"votesSent",result["votes"].size()}}.dump()<<std::endl;
  }
  return result;
 }
 void receive_bft(const PeerInfo& peer,Json remote,Reply reply){
  auto found=peers_.find(peer.id.toBase58());if(found==peers_.end()){reply(nullptr);return;}
  found->second.bft_known->observe(remote);
  auto params=Json::array({remote});
  core_.call("nano_bftExchangeManifest",std::move(params),[this,peer,remote=std::move(remote),reply=std::move(reply)](Json local)mutable{
   reply(local);
   try{
    if(report_token_.empty()||bft_download_busy_||!local.is_object()||!remote.is_object()||!remote.at("proposal").is_object())return;
    if(local.at("height")!=remote.at("height")||local.at("round")!=remote.at("round")||!local.at("proposal").is_null())return;
    auto height=std::stoull(remote.at("height").get<std::string>(),nullptr,16);bft_download_busy_=true;
    std::make_shared<BlockDownload>(io_,core_,report_token_,height,[this,peer](Json request,Reply response){exchange(peer,bft_parts,std::move(request),std::move(response));},[this](bool ok,Json result){
     bft_download_busy_=false;if(trace_network_){result["event"]=ok?"test-bft-fragments-received":"test-bft-fragments-failed";std::cout<<result.dump()<<std::endl;}
    },std::move(remote))->start();
   }catch(const std::exception&){bft_download_busy_=false;}
  });
 }
 void inbound(const std::string& protocol,Stream stream){
  auto remote=stream->remotePeerId();if(!remote||!initialized_){stream->reset();return;}auto id=remote.value().toBase58();
  if(protocol==consensus_protocol()||(capella_mode_&&protocol==session_protocol)){
   if(!altair_ingress_||!altair_dispatch_||altair_dispatch_->blocked()||!peers_.contains(id)||!consensus_peer(peers_.at(id).status)){stream->reset();return;}
   // AltairStreamRequest owns the shared budget lease through durable worker
   // completion. Do not acquire the generic lease a second time below.
   const bool reusable=protocol==session_protocol;
   if(reusable&&!session_peer(peers_.at(id).status)){stream->reset();return;}
   const auto generation=peers_.at(id).capella_generation;
   auto dispatch=altair_dispatch_->dispatcher();
   if(capella_mode_){auto& peer=peers_.at(id);const auto capacity=capella_relay_.at("acknowledgedRoots").get<size_t>();if(!peer.capella_votes)peer.capella_votes=std::make_shared<AltairRelayProgress>(capacity);if(!peer.capella_sync_messages)peer.capella_sync_messages=std::make_shared<AltairRelayProgress>(capacity);dispatch=remember_message_sender(std::move(dispatch),peer.capella_votes,AltairWireKind::attestation);dispatch=remember_message_sender(std::move(dispatch),peer.capella_sync_messages,AltairWireKind::sync_committee_message);}
   else dispatch=remember_vote_sender(std::move(dispatch),peers_.at(id).altair_vote_progress);
   auto delivery=[this,stream,dispatch,id,generation,reusable](auto message,auto cancel,auto done){auto it=peers_.find(id);if(it==peers_.end()||it->second.capella_generation!=generation||!consensus_peer(it->second.status)||(reusable&&!session_peer(it->second.status))||altair_dispatch_->blocked())return false;const auto kind=message.kind;return dispatch(std::move(message),std::move(cancel),[this,stream,kind,done=std::move(done)](auto decision){const bool drop_vote=drop_altair_vote_reply_&&kind==AltairWireKind::attestation;if((drop_altair_reply_||drop_vote)&&decision==AltairWork::Decision::accepted){if(drop_vote)drop_altair_vote_reply_=false;else drop_altair_reply_=false;stream->reset();std::cout<<Json{{"event",drop_vote?"test-altair-vote-reply-dropped":"test-altair-reply-dropped"}}.dump()<<std::endl;}done(decision);});};
   auto observe=[this,id](Json result){if(trace_network_){result["event"]="test-consensus-stream-completion";result["peerId"]=id;std::cout<<result.dump()<<std::endl;}};
   if(reusable)std::make_shared<AltairStreamSession>(io_,std::move(stream),altair_ingress_,std::move(delivery),std::move(observe))->start();
   else std::make_shared<AltairStreamRequest>(io_,std::move(stream),altair_ingress_,std::move(delivery),std::move(observe))->start();return;
  }
  // Beacon blocks travel with signed consensus envelopes, never through the
  // legacy height-only importer or legacy BFT round protocol.
  if(execution_consensus()&&(protocol==blockparts||protocol==blocksync||protocol==announce||protocol==announce_ref||protocol==bft||protocol==bft_manifest||protocol==bft_parts)){stream->reset();return;}
  auto admission=inbound_budget_.acquire(id,InboundBudget::Clock::now(),(protocol==bft||protocol==bft_manifest||protocol==chainsync||protocol==blocksync)?InboundBudget::Traffic::control:InboundBudget::Traffic::bulk);if(!admission){if(trace_network_)++budget_rejections_[protocol];stream->reset();return;}
  struct HeldStream {Stream stream;std::shared_ptr<InboundBudget::Lease> admission;};
  auto held=std::make_shared<HeldStream>(HeldStream{stream,std::move(admission)});
  stream=Stream(held,stream.get());
  if(protocol==handshake){read_json(io_,stream,[this,stream,id,peer=remote.value()](Json request){if(!valid(request)){stream->reset();return;}add(PeerInfo{peer,{}},request);write_json(io_,stream,status_,[stream](bool){close_stream(stream);},peer_status_frame_bytes);},peer_status_frame_bytes);return;}
  if(!peers_.contains(id)){stream->reset();return;}
  if(protocol==chainsync){write_json(io_,stream,status_,[stream](bool){close_stream(stream);},peer_status_frame_bytes);return;}
  read_json(io_,stream,[this,protocol,stream,id](Json request){try{
   if(!request.is_object()){stream->reset();return;}
   if(protocol==bft){core_.call("nano_bftExchangePage",Json::array({request}),[this,stream](Json result){write_json(io_,stream,result,[stream](bool){close_stream(stream);});});}
   else if(protocol==bft_manifest){auto peer=peers_.at(id).info;auto knowledge=peers_.at(id).bft_known;receive_bft(peer,std::move(request),[this,stream,knowledge](Json local){write_json(io_,stream,vote_delta(std::move(local),knowledge),[stream](bool){close_stream(stream);});});}
   else if(protocol==bft_parts){
    if(request.size()!=3||!request.contains("blockHash")||!request.contains("transactionHash")||!request.contains("offset")){stream->reset();return;}
    core_.call("nano_bftTransactionFragment",Json::array({request.at("blockHash"),request.at("transactionHash"),request.at("offset")}),[this,stream](Json result){if(!result.is_object()){stream->reset();return;}write_json(io_,stream,result,[stream](bool){close_stream(stream);});});
   }
   else if(protocol==blockparts){
    auto reply=[this,stream,drop=request.value("offset",Json())=="0x40000"](Json result){if(!result.is_object()){stream->reset();return;}
     if(drop&&drop_block_fragment_){drop_block_fragment_=false;std::cout<<Json{{"event","test-block-fragment-dropped"},{"blockHash",result.at("blockHash")},{"offset",result.at("offset")}}.dump()<<std::endl;stream->reset();return;}
     write_json(io_,stream,result,[stream](bool){close_stream(stream);});};
    if(request.size()==1&&request.contains("height")&&request.at("height").is_number_unsigned())core_.call("nano_networkBlockManifestAt",Json::array({request.at("height")}),std::move(reply));
    else if(request.size()==3&&request.contains("blockHash")&&request.contains("transactionHash")&&request.contains("offset"))core_.call("nano_networkTransactionFragment",Json::array({request.at("blockHash"),request.at("transactionHash"),request.at("offset")}),std::move(reply));
    else stream->reset();
   }
   else if(protocol==blocksync){auto start=request.value("start",Json(0));if(!start.is_number_unsigned()){stream->reset();return;}core_.call("nano_networkBlocks",Json::array({start}),[this,stream](Json result){if(result.is_null()){stream->reset();return;}write_json(io_,stream,result,[stream](bool){close_stream(stream);});});}
   else if(protocol==announce_ref){if(!valid_block_reference(request)){stream->reset();return;}if(trace_network_&&reference_trace_count_++<32)std::cout<<Json{{"event","test-block-reference-received"},{"reference",request},{"peerId",id}}.dump()<<std::endl;close_stream(stream);poll_status(id);}
   else if(protocol==announce&&request.contains("block")){core_.call("validator_submitBlock",Json::array({request["block"]}),[stream](Json){close_stream(stream);});}
   else if(protocol==reliable_gossip&&request.contains("transactions")&&request.contains("cursor")&&request["cursor"].is_number_unsigned()){
    if(!request["transactions"].is_array()||!inbound_budget_.admit_transactions(id,request["transactions"].size())){stream->reset();return;}
    auto cursor=request["cursor"];core_.call("nano_receiveTransactions",Json::array({request["transactions"]}),[this,stream,cursor,id](Json result){if(!result.is_object()){stream->reset();return;}if(drop_gossip_reply_){drop_gossip_reply_=false;std::cout<<Json{{"event","test-gossip-reply-dropped"},{"sender",id},{"cursor",cursor},{"processed",result.value("processed",Json())}}.dump()<<std::endl;stream->reset();return;}result["cursor"]=cursor;write_json(io_,stream,result,[stream](bool){close_stream(stream);});});
   }
   else if(protocol==gossip&&request.contains("raw")&&request["raw"].is_string()){if(!inbound_budget_.admit_transactions(id,1)){stream->reset();return;}core_.call("eth_sendRawTransaction",Json::array({request["raw"]}),[stream](Json){close_stream(stream);});}
   else stream->reset();
  }catch(const std::exception&){stream->reset();}});
 }
 // Consensus exchange has its own bounded cadence; discovery, chain sync and
 // transaction gossip keep their one-second cadence and existing inbound limits.
 void schedule_bft(){bft_timer_.expires_after(std::chrono::milliseconds(500));bft_timer_.async_wait([this](auto ec){if(!ec)poll_bft();});}
 void poll_bft(){
  if(!initialized_){schedule_bft();return;}
  // Alternate complete single-round pages. Retain the previous round for lagging
  // voters without combining two potentially large proposals in one frame.
  const bool previous=previous_bft_page_;previous_bft_page_=!previous_bft_page_;
  core_.call(report_token_.empty()?"nano_bftExchangePage":"nano_bftExchangeManifest",Json::array({nullptr,previous}),[this,previous](Json local){
   if(local.is_object())for(auto& [id,peer]:peers_){
    // Advance each peer independently: a slow exchange must not repeatedly
    // skip the same round merely because it overlaps alternate local polls.
    if(peer.previous_bft_page!=previous||!bft_polling_.insert(id).second)continue;
    peer.previous_bft_page=!previous;
    auto knowledge=peer.bft_known;auto outgoing=report_token_.empty()?local:vote_delta(local,knowledge);auto receipt=knowledge->receipt(outgoing);
    exchange(peer.info,report_token_.empty()?bft:bft_manifest,std::move(outgoing),[this,id,info=peer.info,knowledge,receipt=std::move(receipt)](Json remote){
     auto found=peers_.find(id);if(found==peers_.end()||found->second.bft_known!=knowledge){bft_polling_.erase(id);return;}
     if(!report_token_.empty())knowledge->acknowledge(receipt,remote);
     if(remote.is_object()&&!report_token_.empty())receive_bft(info,std::move(remote),[this,id](Json){bft_polling_.erase(id);});
     else if(remote.is_object())core_.call("nano_bftExchangePage",Json::array({remote}),[this,id](Json){bft_polling_.erase(id);});
     else bft_polling_.erase(id);
    });
   }
   // Schedule only after the local snapshot returns; never stack local polls.
   schedule_bft();
  });
 }
 // At most four event batches are retained globally and one per peer.
 // A lost reply keeps the event/inventory position unchanged for retry.
 void poll_gossip(){
  std::vector<std::string> ids;for(const auto& [id,peer]:peers_)ids.push_back(id);
  auto split=std::upper_bound(ids.begin(),ids.end(),gossip_last_);std::rotate(ids.begin(),split,ids.end());
  for(const auto& id:ids){if(gossip_active_>=4)break;auto progress=gossip_progress_.at(id);if(progress->busy)continue;progress->refresh_inventory();
   progress->busy=true;++gossip_active_;gossip_last_=id;auto peer=peers_.at(id).info;
   core_.call("nano_gossipPage",Json::array({progress->cursor,progress->epoch,progress->after,progress->snapshot}),[this,id,peer,progress](Json response){
    auto done=[this,progress]{progress->busy=false;--gossip_active_;};
    if(!response.is_object()||!response.contains("cursor")||!response["cursor"].is_number_unsigned()||!response.contains("transactions")||!response["transactions"].is_array()||response["transactions"].size()>50||!response.contains("epoch")||!response["epoch"].is_string()||response["epoch"].get_ref<const std::string&>().size()!=34||!response.contains("mode")||!response.contains("next")){done();return;}
    bool inventory=response["mode"]=="inventory";if(!inventory&&response["mode"]!="events"){done();return;}
    auto tail=response["next"];if(!tail.is_null()&&(!inventory||!tail.is_string()||tail.get_ref<const std::string&>().size()!=66)){done();return;}
    uint64_t next=response["cursor"].get<uint64_t>();if(!inventory&&(next<progress->cursor||response["epoch"]!=progress->epoch)){done();return;}
    auto epoch=response["epoch"].get<std::string>();auto raw=std::move(response["transactions"]);auto count=raw.size();
    auto acknowledge=[this,id,progress,next,count,inventory,epoch,tail,done](Json reply){
     auto failed=progress->failed_cursor;auto from=progress->cursor;
     if(progress->acknowledge(reply,next,count,inventory)){
      if(trace_network_&&retry_trace_count_<8&&std::any_of(reply["accepted"].begin(),reply["accepted"].end(),[](const Json& value){return value==false;})){++retry_trace_count_;std::cout<<Json{{"event","test-gossip-refused"},{"peerId",id},{"cursor",next},{"processed",count}}.dump()<<std::endl;}
      progress->epoch=epoch;progress->snapshot=inventory&&!tail.is_null();progress->after=progress->snapshot?tail.get<std::string>():"";if(inventory&&progress->periodic&&trace_network_&&periodic_trace_count_<16){++periodic_trace_count_;std::cout<<Json{{"event","test-gossip-periodic-batch"},{"peerId",id},{"processed",count},{"steadyMs",std::chrono::duration_cast<std::chrono::milliseconds>(GossipProgress::Clock::now().time_since_epoch()).count()}}.dump()<<std::endl;}
      if(inventory&&!progress->snapshot){progress->inventory_completed();progress->periodic=false;}
      if(failed&&std::getenv("NANO_TEST_DROP_GOSSIP_REPLY_ONCE")&&retry_trace_count_<8){++retry_trace_count_;std::cout<<Json{{"event","test-gossip-retry-acknowledged"},{"peerId",id},{"failedCursor",*failed},{"retryFrom",from},{"acknowledgedCursor",next}}.dump()<<std::endl;}
     }
     done();
    };
    if(raw.empty()){acknowledge(Json{{"cursor",next},{"processed",0},{"accepted",Json::array()}});return;}
    exchange(peer,reliable_gossip,{{"transactions",std::move(raw)},{"cursor",next}},std::move(acknowledge));
   });
  }
 }
 void poll_block_events(){
  if(event_polling_)return;event_polling_=true;const bool refs=!report_token_.empty();auto from=cursor_;auto epoch=event_epoch_;
  core_.call(refs?"nano_networkEventRefs":"nano_networkEvents",refs?Json::array({from,epoch}):Json::array({from}),[this,refs,from,epoch](Json response){
   event_polling_=false;
   if(!response.is_object()||!response.contains("cursor")||!response.at("cursor").is_number_unsigned()||!response.contains("events")||!response.at("events").is_array()||response.at("events").size()>50)return;
   auto next=response.at("cursor").get<uint64_t>();
   if(refs){
    if(response.value("version",Json())!=1||!response.contains("reset")||!response.at("reset").is_boolean()||!response.contains("epoch")||!response.at("epoch").is_string()||response.at("epoch").get_ref<const std::string&>().size()!=34)return;
    if(!response.at("reset").get<bool>()&&(next<from||response.at("epoch")!=epoch))return;
    for(const auto& event:response.at("events"))if(!event.is_object()||event.value("type",Json())!="block"||!event.contains("value")||!valid_block_reference(event.at("value")))return;
    event_epoch_=response.at("epoch").get<std::string>();
   }else if(next<from)return;
   // References are hints backed by periodic canonical status reconciliation.
   // A failed local RPC does not silently skip its cursor.
   cursor_=next;
   if(execution_consensus())return;
   for(const auto& event:response.at("events"))if(event.value("type",Json())=="block"){
    if(refs&&trace_network_&&reference_trace_count_++<32)std::cout<<Json{{"event","test-block-reference-sent"},{"reference",event.at("value")},{"cursor",cursor_}}.dump()<<std::endl;
    for(const auto& [id,peer]:peers_)send(peer.info,refs?announce_ref:announce,refs?event.at("value"):Json{{"block",event.at("value")}});
   }
  });
 }
 void schedule_consensus_relay(){consensus_relay_timer_.expires_after(consensus_relay_interval_);consensus_relay_timer_.async_wait([this](auto ec){if(!ec){poll_altair_archives();schedule_consensus_relay();}});}
 void schedule(){timer_.expires_after(std::chrono::seconds(1));timer_.async_wait([this](auto ec){if(!ec)tick();});}
 void tick(){core_.call("nano_networkStatus",Json::array(),[this](Json local){if(!valid_peer_status(local,local)){schedule();return;}status_=std::move(local);status_[consensus_field()]=altair_advertisement();if(capella_mode_&&!capella_session_.empty())status_["capellaSession"]=capella_session_;if(capella_mode_&&altair_ingress_)status_["capellaStreamProtocol"]=session_protocol;if(!initialized_){cursor_=status_.value("cursor",uint64_t(0));event_epoch_=status_.value("gossip_epoch",std::string{});initialized_=true;std::cout<<Json{{"event","ready"},{"chainId",status_["chain_id"]}}.dump()<<std::endl;}
  recover_altair();
  if(execution_consensus()){
   if(beacon_head_slot_&&std::chrono::steady_clock::now()-beacon_head_seen_<std::chrono::seconds(5))status_["beacon_head_slot"]=*beacon_head_slot_;
   if(!beacon_status_pending_){beacon_status_pending_=true;consensus_call("nano_altairStatus",Json::array({report_token_}),[this](Json result){beacon_status_pending_=false;if(result.is_object()&&result.contains("headSlot")&&result["headSlot"].is_number_unsigned()){beacon_head_slot_=result["headSlot"].get<uint64_t>();beacon_head_seen_=std::chrono::steady_clock::now();}else beacon_head_slot_.reset();});}
  }

  refresh_bootstrap();
  for(const auto& seed:seeds_){if(connecting_.size()>=4||peers_.size()+connecting_.size()>=50)break;auto id=seed.id.toBase58();if(id==host_->getPeerInfo().id.toBase58()||peers_.contains(id)||connecting_.contains(id))continue;auto now=std::chrono::steady_clock::now();if(retry_after_.contains(id)&&now<retry_after_[id])continue;retry_after_[id]=now+std::chrono::seconds(5);connecting_.insert(id);exchange(seed,handshake,status_,[this,seed,id](Json remote){connecting_.erase(id);add(seed,remote);});}
  std::vector<PeerInfo> known;for(const auto& [id,peer]:peers_)known.push_back(peer.info);
  for(const auto& info:known)poll_status(info.id.toBase58());

  poll_gossip();
  poll_block_events();
  if(!report_token_.empty()){
   Json addresses=Json::array(),peers=Json::array(),bootnodes=Json::array();for(const auto& addr:bootstrap_)bootnodes.push_back(std::string(addr.getStringAddress()));for(const auto& addr:host_->getAddressesInterfaces())addresses.push_back(std::string(addr.getStringAddress()));
   for(const auto& [id,peer]:peers_){Json report={{"peerId",id},{"bestBlock",peer.status["best_block"]},{"bestHash",peer.status.value("best_hash",Json())}};
    if(execution_consensus()&&std::chrono::steady_clock::now()-peer.status_seen<std::chrono::seconds(5)&&consensus_peer(peer.status)&&peer.status.contains("beacon_head_slot")&&peer.status["beacon_head_slot"].is_number_unsigned())report["beaconHeadSlot"]=peer.status["beacon_head_slot"];
    peers.push_back(std::move(report));
   }
   core_.call("nano_networkReport",Json::array({report_token_,Json{{"peerId",host_->getPeerInfo().id.toBase58()},{"addresses",addresses},{"peers",peers},{"bootnodes",bootnodes}}}),[](Json){});
  }
  if(trace_network_&&trace_ticks_<40&&++trace_ticks_%5==0){Json ids=Json::array();for(const auto& [id,peer]:peers_)ids.push_back(id);std::cout<<Json{{"event","test-network-budget"},{"height",status_["best_block"]},{"peers",ids},{"rejectedByProtocol",budget_rejections_},{"failedStatus",failed_status_}}.dump()<<std::endl;}
  schedule();
 });}
public:
 Network(asio::io_context& io,Host host,uint16_t port,std::vector<libp2p::multi::Multiaddress> bootstrap,std::shared_ptr<libp2p::network::DnsaddrResolver> resolver):io_(io),host_(std::move(host)),core_(io,port,4,std::getenv("NANO_TEST_NETWORK_TRACE")?bounded_core_rpc_trace([](const Json& row){auto record=row;record["event"]="test-core-rpc-failure";std::cerr<<record.dump()<<std::endl;},64):CoreRpcTrace{}),timer_(io),bft_timer_(io),consensus_relay_timer_(io),bootstrap_(std::move(bootstrap)),resolver_(std::move(resolver)){if(const char* token=std::getenv("NANO_NETWORK_TOKEN"))report_token_=token;
  if(const char* raw=std::getenv("NANO_CONSENSUS_RELAY_CONCURRENCY")){const std::string value(raw);if(value.empty()||value.size()>4||value.find_first_not_of("0123456789")!=std::string::npos)throw std::invalid_argument("Invalid consensus relay concurrency");altair_relay_budget_=PeerRelayBudget(std::stoul(value));}
  if(const char* raw=std::getenv("NANO_CONSENSUS_RELAY_INTERVAL_MS")){const std::string value(raw);if(value.empty()||value.size()>5||value.find_first_not_of("0123456789")!=std::string::npos)throw std::invalid_argument("Invalid consensus relay interval");const auto n=std::stoul(value);if(n<25||n>60000)throw std::invalid_argument("Consensus relay interval outside 25..60000 ms");consensus_relay_interval_=std::chrono::milliseconds(n);}
 }
 void start(){for(const auto& protocol:{handshake,blocksync,announce,announce_ref,gossip,reliable_gossip,chainsync,bft,bft_manifest,bft_parts,blockparts})host_->setProtocolHandler({protocol},[this,protocol](libp2p::StreamAndProtocol stream){inbound(protocol,std::move(stream.stream));});tick();schedule_bft();schedule_consensus_relay();}
};
int main(int argc,char** argv){try{
 // The direct parent is the lifetime owner only for managed relays. Pin its
 // identity before opening listeners, including death between spawn and exec.
 pid_t supervisor=0;
 if(const char* raw=std::getenv("NANO_SUPERVISOR_PID")){
  // Linux containers can run the legitimate core as namespace PID 1. The
  // kernel kills the namespace's remaining processes if that init exits.
  // Other managed parents still use the same strict identity/liveness check.
  supervisor=managed_supervisor(raw,getppid());
 }
 if(argc<4){std::cerr<<"Usage: nano-p2p <core-rpc-port> <identity-file> <listen-multiaddress> [peer-multiaddress ...]\n";return 1;}
 auto port=std::stoul(argv[1]);if(port<1||port>65535)throw std::invalid_argument("Invalid RPC port");
 auto logging=std::make_shared<soralog::LoggingSystem>(std::make_shared<libp2p::log::Configurator>());auto configured=logging->configure();if(configured.has_error)throw std::runtime_error(configured.message);libp2p::log::setLoggingSystem(logging);
 auto injector=libp2p::injector::makeHostInjector(libp2p::injector::useKeyPair(identity(argv[2])),libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>(),libp2p::injector::useTransportAdaptors<libp2p::transport::TcpTransport>());
 auto host=injector.create<Host>();auto io=injector.create<std::shared_ptr<asio::io_context>>();auto listen=libp2p::multi::Multiaddress::create(argv[3]).value();auto listening=host->listen(listen);if(!listening)throw std::runtime_error(listening.error().message());host->start();
 if(argc>54)throw std::invalid_argument("At most 50 bootstrap addresses are supported");
 std::vector<libp2p::multi::Multiaddress> seeds;for(int i=4;i<argc;++i){auto ma=libp2p::multi::Multiaddress::create(argv[i]).value();auto id=ma.getPeerId();if(!id&&!ma.hasProtocol(libp2p::multi::Protocol::Code::DNS_ADDR))throw std::invalid_argument("Peer multiaddress requires /p2p/peer-id");if(id)libp2p::peer::PeerId::fromBase58(*id).value();seeds.push_back(ma);}
 for(const auto& address:host->getAddressesInterfaces())std::cout<<Json{{"event","listen"},{"address",std::string(address.getStringAddress())+"/p2p/"+host->getPeerInfo().id.toBase58()}}.dump()<<std::endl;
 libp2p::network::c_ares::Ares cares;auto resolver=std::make_shared<libp2p::network::DnsaddrResolverImpl>(io,cares);
 Network network(*io,host,uint16_t(port),std::move(seeds),std::move(resolver));network.start();asio::signal_set signals(*io,SIGINT,SIGTERM);signals.async_wait([&](auto,int){host->stop();io->stop();});// SIGKILL bypasses the parent's destructor; stop the orphan without relying
 // on RPC failure (which may also occur temporarily during normal operation).
 asio::steady_timer parent_watch(*io);std::function<void()> check_parent;
 check_parent=[&]{if(getppid()!=supervisor){std::cerr<<"Nano supervisor exited; relay stopped\n";host->stop();io->stop();return;}parent_watch.expires_after(std::chrono::milliseconds(250));parent_watch.async_wait([&](auto error){if(!error)check_parent();});};
 if(supervisor)check_parent();io->run();return 0;
}catch(const std::exception& e){std::cerr<<"Nano P2P: "<<e.what()<<"\n";return 1;}}
