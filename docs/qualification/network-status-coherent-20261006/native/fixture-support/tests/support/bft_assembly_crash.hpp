#pragma once
// Owned subprocess fixture: no public listener, no production fault-injection hook.
namespace test {
inline Json assembly_request(Rpc& rpc,const char* method,Json params=Json::array()) {
 return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});
}
inline void assembly_wallet(Rpc& rpc,const fs::path& journal) {
 setenv("NANO_ASSEMBLY_CRASH_KEY",key(4).c_str(),1);
 rpc.load_wallets({{"bft",{{"key_env","NANO_ASSEMBLY_CRASH_KEY"},{"journal",journal.string()}}}},"127.0.0.1");
}
inline int assembly_crash_child(const fs::path& root,unsigned count) {
 auto fixture=load(root/"fixture.json");auto genesis=Genesis::from_json(fixture.at("genesis"));
 Blockchain chain(root/"chain",genesis);Rpc receiver(chain,"assembly-test-token");assembly_wallet(receiver,root/"journal");
 auto initial=chain.state().root();auto result=assembly_request(receiver,"nano_beginBftImport",Json::array({"assembly-test-token",fixture.at("manifest")}));
 auto id=result.at("result").at("id");auto parts=fixture.at("parts");check(count>0&&count<=parts.size(),"valid crash fragment boundary");
 for(unsigned i=0;i<count;++i){const auto& part=parts.at(i);check(!assembly_request(receiver,"nano_appendBlockImport",Json::array({"assembly-test-token",id,part.at("index"),part.at("fragment")})).contains("error"),"child fragment acknowledged");}
 auto manifest=assembly_request(receiver,"nano_bftManifest").at("result");
 check(manifest.at("proposal").is_null()&&manifest.at("votes").empty()&&chain.state().root()==initial&&chain.tip().header.number==0,"assembly must not vote or mutate canonical state");
 std::cout<<Json{{"event","assembly-ready-for-kill"},{"fragments",count},{"session",id},{"height",0},{"stateRoot",hex(initial)},{"votes",0},{"pid",getpid()}}.dump()<<std::endl;
 // A stop prevents any post-acknowledgement work; parent must terminate by SIGKILL.
 raise(SIGSTOP);throw std::runtime_error("crash fixture unexpectedly resumed");
}
inline void verify_assembly_crashes(const char* executable,const fs::path& parent,const Json& genesis_json,const Json& manifest,const Json& parts,const Block& block,const Json& prevotes,const Json& precommits,const std::string& fork) {
 check(parts.size()==40&&block.to_transport_json().dump().size()>16*1024*1024,"large assembly boundary fixture");
 auto genesis=Genesis::from_json(genesis_json);
 for(unsigned count:{1u,20u,40u}) {
  auto root=parent/("assembly-crash-"+std::to_string(count));fs::create_directories(root);
  save(root/"fixture.json",{{"genesis",genesis_json},{"manifest",manifest},{"parts",parts}});
  Process child({executable,"--assembly-crash-child",root.string(),std::to_string(count)});
  auto acknowledged=child.event("assembly-ready-for-kill");check(acknowledged.at("fragments")==count,"correct acknowledged crash boundary");
  check(child.running(),"owned assembly process alive before kill");child.signal(SIGKILL);check(child.wait().code==-SIGKILL,"observed SIGKILL termination");
  unsigned stale_rejections=0;
  {
   Blockchain chain(root/"chain",genesis);check(chain.tip().header.number==0&&chain.state().root()==genesis.state.root()&&chain.audit().at("verified")==true,"reopened pre-commit state unchanged and audited");
   Rpc receiver(chain,"assembly-test-token");assembly_wallet(receiver,root/"journal");
   auto request=[&](const char* method,Json params=Json::array()){return assembly_request(receiver,method,std::move(params));};
   auto recovered=request("nano_bftManifest").at("result");check(recovered.is_null()||(recovered.at("proposal").is_null()&&recovered.at("votes").empty()),"no phantom proposal or vote after recovery");
   auto stale=acknowledged.at("session");
   check(request("nano_finishBftImport",Json::array({"assembly-test-token",stale})).contains("error"),"old finish session rejected after process death");++stale_rejections;
   check(request("nano_appendBlockImport",Json::array({"assembly-test-token",stale,0,parts.at(0).at("fragment")})).contains("error"),"old append session rejected after process death");++stale_rejections;
   auto id=request("nano_beginBftImport",Json::array({"assembly-test-token",manifest})).at("result").at("id");
   for(const auto& part:parts)check(!request("nano_appendBlockImport",Json::array({"assembly-test-token",id,part.at("index"),part.at("fragment")})).contains("error"),"redownload fragment accepted");
   auto staged=request("nano_bftManifest").at("result");check(staged.at("proposal").is_null()&&staged.at("votes").empty()&&chain.tip().header.number==0,"complete redownload not automatically accepted");
   auto accepted=request("nano_finishBftImport",Json::array({"assembly-test-token",id})).at("result");check(accepted.at("received")==true&&accepted.at("blockHash")==hex(block.header.hash()),"fresh complete assembly authenticated");
   check(chain.tip().header.number==0&&chain.state().root()==genesis.state.root(),"accepted proposal still needs quorum");
   auto page=manifest;page["proposal"]=nullptr;page["votes"]=prevotes;check(!request("nano_bftExchangePage",Json::array({page})).contains("error"),"signed prevote quorum accepted");page["votes"]=precommits;check(!request("nano_bftExchangePage",Json::array({page})).contains("error"),"signed precommit quorum accepted");
   check(chain.tip().header.hash()==block.header.hash()&&chain.audit().at("verified")==true,"fresh assembly quorum commit audited");
   check(request("nano_finishBftImport",Json::array({"assembly-test-token",id})).contains("error"),"finish replay rejected");++stale_rejections;
  }
  {
   Blockchain reopened(root/"chain",genesis);check(reopened.tip().header.hash()==block.header.hash()&&reopened.audit().at("verified")==true,"second reopen verifies exact certified block");
   check(reopened.store().certificate_at(1).has_value(),"recovered block has certificate");
   check(reopened.state().get("0x00000000000000000000000000000000000000b0").balance==10,"ten transfers applied exactly once");
   for(const auto& tx:block.transactions){auto receipt=reopened.store().receipt(tx.hash());check(receipt&&receipt->success,"all recovered transaction receipts successful");}
  }
  std::cout<<Json{{"scenario","bft-assembly-crash"},{"fork",fork},{"crashAfterFragments",count},{"totalFragments",parts.size()},{"signal","SIGKILL"},{"preCommitAudit",true},{"noPhantomVotes",true},{"redownloadCommitted",true},{"reopenedAudit",true},{"receipts",block.transactions.size()},{"rejectedStaleOrReplay",stale_rejections},{"blockHash",hex(block.header.hash())},{"libp2pIntegrated",false}}.dump()<<std::endl;
 }
}
}
