#include "json_stream.hpp"
#include "block_download.hpp"
#include "block_reference.hpp"
#include "bootstrap.hpp"
#include "peer_status.hpp"
#include "inbound_budget.hpp"
#include "gossip_progress.hpp"
#include "bft_vote_knowledge.hpp"
#include "timed_reply.hpp"
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
 asio::io_context& io_;Host host_;Core core_;asio::steady_timer timer_,bft_timer_;
 Json status_;uint64_t cursor_=0;bool initialized_=false;bool previous_bft_page_=false;bool download_busy_=false,bft_download_busy_=false;
 std::string report_token_,event_epoch_;bool event_polling_=false;unsigned reference_trace_count_=0;InboundBudget inbound_budget_;
 bool drop_block_fragment_=std::getenv("NANO_TEST_DROP_BLOCK_FRAGMENT_ONCE")!=nullptr;
 bool trace_network_=std::getenv("NANO_TEST_NETWORK_TRACE")!=nullptr;
 std::map<std::string,uint64_t> budget_rejections_;uint64_t failed_status_=0;unsigned trace_ticks_=0;
 unsigned retry_trace_count_=0,periodic_trace_count_=0,vote_delta_trace_count_=0;
 std::map<std::string,std::shared_ptr<GossipProgress>> gossip_progress_;size_t gossip_active_=0;std::string gossip_last_;bool drop_gossip_reply_=std::getenv("NANO_TEST_DROP_GOSSIP_REPLY_ONCE")!=nullptr;
 struct Peer {PeerInfo info;Json status;std::shared_ptr<BftVoteKnowledge> bft_known=std::make_shared<BftVoteKnowledge>();bool syncing=false;bool previous_bft_page=false;std::chrono::steady_clock::time_point fragment_retry{};};
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
  auto pending=std::make_shared<TimedReply>(io_,std::move(reply));pending->start();
  host_->newStream(info,{protocol},[this,request=std::move(request),pending](auto result){
   if(!result){pending->finish(nullptr);return;}auto stream=result.value().stream;
   if(!pending->on_cancel([stream]{stream->reset();}))return;
   write_json(io_,stream,request,[this,stream,pending](bool ok){
    if(!pending->active())return;
    if(!ok){stream->reset();pending->finish(nullptr);return;}
    read_json(io_,stream,[stream,pending](Json value){close_stream(stream);pending->finish(std::move(value));});
   });
  });
 }

 void send(PeerInfo info,const std::string& protocol,Json value){host_->newStream(info,{protocol},[this,value=std::move(value)](auto result){if(!result)return;auto stream=result.value().stream;write_json(io_,stream,value,[stream](bool){close_stream(stream);});});}
 void add(PeerInfo info,const Json& status){if(!valid(status)||peers_.size()>=50)return;auto id=info.id.toBase58();bool fresh=!peers_.contains(id);observe_gossip_session(gossip_progress_[id],fresh?Json::object():peers_.at(id).status,status);if(fresh)peers_.emplace(id,Peer{std::move(info),status});else{if(peers_.at(id).status.value("gossip_epoch",Json())!=status.value("gossip_epoch",Json()))peers_.at(id).bft_known=std::make_shared<BftVoteKnowledge>();peers_.at(id).info=std::move(info);peers_.at(id).status=status;}if(fresh)std::cout<<Json{{"event","peer"},{"peerId",id},{"height",status["best_block"]}}.dump()<<std::endl;}
 void import_blocks(const std::string& id,std::shared_ptr<Json> blocks,size_t index){
  if(index>=blocks->size()){if(auto it=peers_.find(id);it!=peers_.end())it->second.syncing=false;return;}
  core_.call("validator_submitBlock",Json::array({(*blocks)[index]}),[this,id,blocks,index](Json result){if(result.is_object()&&result.value("success",false))import_blocks(id,blocks,index+1);else{if(auto it=peers_.find(id);it!=peers_.end())it->second.syncing=false;std::cerr<<"Rejected block received from peer\n";}});
 }
 void sync(const std::string& id){auto it=peers_.find(id);if(it==peers_.end()||it->second.syncing||it->second.status["best_block"]<=status_["best_block"])return;
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
  exchange(info,chainsync,Json::object(),[this,id](Json remote){polling_.erase(id);auto it=peers_.find(id);if(it==peers_.end())return;if(!valid(remote)){if(trace_network_)++failed_status_;peers_.erase(it);gossip_progress_.erase(id);return;}auto previous=gossip_progress_[id];observe_gossip_session(gossip_progress_[id],it->second.status,remote);if(trace_network_&&previous!=gossip_progress_[id]&&retry_trace_count_<8){++retry_trace_count_;std::cout<<Json{{"event","test-gossip-remote-session-reset"},{"peerId",id},{"oldEpoch",it->second.status.value("gossip_epoch",Json())},{"newEpoch",remote.value("gossip_epoch",Json())}}.dump()<<std::endl;}if(it->second.status.value("gossip_epoch",Json())!=remote.value("gossip_epoch",Json()))it->second.bft_known=std::make_shared<BftVoteKnowledge>();it->second.status=std::move(remote);sync(id);});
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
  auto admission=inbound_budget_.acquire(id,InboundBudget::Clock::now(),(protocol==bft||protocol==bft_manifest||protocol==chainsync||protocol==blocksync)?InboundBudget::Traffic::control:InboundBudget::Traffic::bulk);if(!admission){if(trace_network_)++budget_rejections_[protocol];stream->reset();return;}
  struct HeldStream {Stream stream;std::shared_ptr<InboundBudget::Lease> admission;};
  auto held=std::make_shared<HeldStream>(HeldStream{stream,std::move(admission)});
  stream=Stream(held,stream.get());
  if(protocol==handshake){read_json(io_,stream,[this,stream,id,peer=remote.value()](Json request){if(!valid(request)){stream->reset();return;}add(PeerInfo{peer,{}},request);write_json(io_,stream,status_,[stream](bool){close_stream(stream);});});return;}
  if(!peers_.contains(id)){stream->reset();return;}
  if(protocol==chainsync){write_json(io_,stream,status_,[stream](bool){close_stream(stream);});return;}
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
   for(const auto& event:response.at("events"))if(event.value("type",Json())=="block"){
    if(refs&&trace_network_&&reference_trace_count_++<32)std::cout<<Json{{"event","test-block-reference-sent"},{"reference",event.at("value")},{"cursor",cursor_}}.dump()<<std::endl;
    for(const auto& [id,peer]:peers_)send(peer.info,refs?announce_ref:announce,refs?event.at("value"):Json{{"block",event.at("value")}});
   }
  });
 }
 void schedule(){timer_.expires_after(std::chrono::seconds(1));timer_.async_wait([this](auto ec){if(!ec)tick();});}
 void tick(){core_.call("nano_networkStatus",Json::array(),[this](Json local){if(!valid_peer_status(local,local)){schedule();return;}status_=std::move(local);if(!initialized_){cursor_=status_.value("cursor",uint64_t(0));event_epoch_=status_.value("gossip_epoch",std::string{});initialized_=true;std::cout<<Json{{"event","ready"},{"chainId",status_["chain_id"]}}.dump()<<std::endl;}
  refresh_bootstrap();
  for(const auto& seed:seeds_){if(connecting_.size()>=4||peers_.size()+connecting_.size()>=50)break;auto id=seed.id.toBase58();if(id==host_->getPeerInfo().id.toBase58()||peers_.contains(id)||connecting_.contains(id))continue;auto now=std::chrono::steady_clock::now();if(retry_after_.contains(id)&&now<retry_after_[id])continue;retry_after_[id]=now+std::chrono::seconds(5);connecting_.insert(id);exchange(seed,handshake,status_,[this,seed,id](Json remote){connecting_.erase(id);add(seed,remote);});}
  std::vector<PeerInfo> known;for(const auto& [id,peer]:peers_)known.push_back(peer.info);
  for(const auto& info:known)poll_status(info.id.toBase58());

  poll_gossip();
  poll_block_events();
  if(!report_token_.empty()){
   Json addresses=Json::array(),peers=Json::array(),bootnodes=Json::array();for(const auto& addr:bootstrap_)bootnodes.push_back(std::string(addr.getStringAddress()));for(const auto& addr:host_->getAddressesInterfaces())addresses.push_back(std::string(addr.getStringAddress()));
   for(const auto& [id,peer]:peers_)peers.push_back({{"peerId",id},{"bestBlock",peer.status["best_block"]},{"bestHash",peer.status.value("best_hash",Json())}});
   core_.call("nano_networkReport",Json::array({report_token_,Json{{"peerId",host_->getPeerInfo().id.toBase58()},{"addresses",addresses},{"peers",peers},{"bootnodes",bootnodes}}}),[](Json){});
  }
  if(trace_network_&&trace_ticks_<40&&++trace_ticks_%5==0){Json ids=Json::array();for(const auto& [id,peer]:peers_)ids.push_back(id);std::cout<<Json{{"event","test-network-budget"},{"height",status_["best_block"]},{"peers",ids},{"rejectedByProtocol",budget_rejections_},{"failedStatus",failed_status_}}.dump()<<std::endl;}
  schedule();
 });}
public:
 Network(asio::io_context& io,Host host,uint16_t port,std::vector<libp2p::multi::Multiaddress> bootstrap,std::shared_ptr<libp2p::network::DnsaddrResolver> resolver):io_(io),host_(std::move(host)),core_(io,port),timer_(io),bft_timer_(io),bootstrap_(std::move(bootstrap)),resolver_(std::move(resolver)){if(const char* token=std::getenv("NANO_NETWORK_TOKEN"))report_token_=token;}
 void start(){for(const auto& protocol:{handshake,blocksync,announce,announce_ref,gossip,reliable_gossip,chainsync,bft,bft_manifest,bft_parts,blockparts})host_->setProtocolHandler({protocol},[this,protocol](libp2p::StreamAndProtocol stream){inbound(protocol,std::move(stream.stream));});tick();schedule_bft();}
};
int main(int argc,char** argv){try{
 // The direct parent is the lifetime owner only for managed relays. Pin its
 // identity before opening listeners, including death between spawn and exec.
 pid_t supervisor=0;
 if(const char* raw=std::getenv("NANO_SUPERVISOR_PID")){
  std::string value(raw);if(value.empty()||value.find_first_not_of("0123456789")!=std::string::npos)throw std::invalid_argument("Invalid supervisor PID");
  auto parsed=std::stoll(value);if(parsed<=1||parsed>std::numeric_limits<pid_t>::max())throw std::invalid_argument("Invalid supervisor PID");supervisor=static_cast<pid_t>(parsed);
  if(getppid()!=supervisor)throw std::runtime_error("Supervisor exited before relay startup");
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
