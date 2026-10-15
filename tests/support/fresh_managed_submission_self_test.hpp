#pragma once
// New protocol/real-file test only. Opaque operation bytes are not BLS proofs;
// the in-process callback is not a live core, HTTP server or network campaign.
#include "fresh_managed_submission.hpp"
namespace nano::test::managed_submission {
inline int self_test(){
 uint64_t refusals=0,groups=0;const auto began=std::chrono::steady_clock::now();const auto check=[&]{need(std::chrono::steady_clock::now()-began<std::chrono::seconds(25),"self-test deadline");};
 auto reject=[&](auto operation){bool failed=false;try{operation();}catch(const std::exception&){failed=true;}need(failed,"expected refusal");++refusals;};
 const std::string campaign(64,'a'),token="0x"+std::string(64,'b'),session="0x"+std::string(64,'c'),public_key="0x"+std::string(96,'d'),genesis="0x"+std::string(64,'e'),identity="0x"+std::string(64,'f');
 const Json artifact{{"path","/test-only/opaque"},{"sha256",std::string(64,'a')},{"bytes",uint64_t(1)}};
 const Json plan{{"format","nano-fresh-lifecycle-submission-plan-v1"},{"campaignId",campaign},{"owner","mac-native"},{"manifest",artifact},{"action",artifact},{"realP2p",artifact},{"resultFile","/test-only/new-receipt"},{"maximumWallSeconds",uint64_t(30)}};
 const Json manifest{{"format","nano-isolated-capella-fixture-domain-v1"},{"synthetic",true},{"productionQualified",false},{"fork","Capella/Shanghai"},{"campaignId",campaign},{"chainId","0x539"},{"executionGenesisHash",genesis},{"publicKeys",Json::array({Json{{"index",uint64_t(0)},{"publicKey",public_key}}})}};
 const Json envelope{{"chainId","0x539"},{"genesisHash",genesis},{"consensusIdentity",identity},{"forkDigest","0xffffffff"},{"kind","voluntary_exit"},{"ssz",hex(Bytes(112,0))}};
 auto payload=Bytes(113,0);payload[0]=3;const Json document{{"format","nano-fresh-lifecycle-action-v1"},{"campaignId",campaign},{"requestSha256",std::string(64,'a')},{"owner","mac-native"},{"kind","voluntary_exit"},{"publicKey",public_key},{"resolvedIndex","0x0"},{"preparedHead",{{"root",genesis},{"stateRoot",identity},{"slot","0x8"}}},{"payloadDigest",hex(sha256(payload))},{"envelope",envelope}};
 const auto action=policy(plan,manifest,document);need(action.owner=="mac-native"&&action.kind=="voluntary_exit","positive policy");
 for(unsigned fault=0;fault<8;++fault)reject([&]{auto p=plan,m=manifest,a=document;if(fault==0)p["owner"]="vps-docker";if(fault==1)p["format"]="other";if(fault==2)p["maximumWallSeconds"]=uint64_t(61);if(fault==3)m["synthetic"]=false;if(fault==4)a["kind"]="bls_to_execution_change";if(fault==5)a["payloadDigest"]=genesis;if(fault==6)a["publicKey"]="0x"+std::string(96,'a');if(fault==7)a["envelope"]["chainId"]="0x1";(void)policy(p,m,a);});
 reject([&]{(void)parse("{\"same\":1,\"same\":2}");});++groups;std::cout<<Json{{"scenario","fresh-lifecycle-managed-submit-policy"},{"passed",true},{"expectedRefusals",9},{"cryptographicWireVerificationClaimed",false},{"productionQualified",false}}.dump()<<'\n';
 auto protocol=[&](int fault){
  uint64_t submits=0,calls=0;Json result;
  const Call callback=[&](const std::string& method,Json params){check();++calls;need(params.is_array()&&!params.empty()&&params[0]==token,"in-process token stays callback-local");
   if(method=="nano_capellaStatus"){auto s=envelope;s.erase("kind");s.erase("ssz");s["executionBound"]=true;s["clockHealthy"]=true;s["validatorHealthy"]=true;s["networkAdmissionEnabled"]=true;s["admissionSession"]=session;if(fault==1)s["consensusIdentity"]=genesis;if(fault==2)s["validatorHealthy"]=false;return s;}
   if(method=="nano_capellaJobs"){need(params.size()==1,"inventory arity");Json jobs=Json::array();if(fault==3)jobs.push_back({{"state","pending"}});return Json{{"session",fault==7?token:session},{"highWater","0x7"},{"jobs",jobs}};}
   need(params.size()>2&&params[1]==session&&params[2]=="0x8","session and next high-water sequence");
   const Json complete{{"sequence","0x8"},{"payloadDigest",action.payload},{"cancelRequested",false},{"state","completed"},{"decision","accepted"}};
   if(method=="nano_capellaSubmit"){++submits;need(params.size()==4&&params[3].is_string()&&parse(params[3].get<std::string>())==envelope,"Submit carries exact envelope string");if(fault==4)throw std::runtime_error("synthetic lost submit");auto j=complete;j.erase("decision");j["state"]="pending";return j;}
   if(method=="nano_capellaJob"){auto j=complete;if(fault==5)j["payloadDigest"]=genesis;if(fault==6)j["decision"]="rejected";if(fault==8)j["cancelRequested"]=true;return j;}
   need(method=="nano_capellaForget","known callback method");auto j=complete;if(fault==9)j["payloadDigest"]=genesis;return j;
  };
  try{result=accepted(action,token,callback,check);}catch(...){need(submits==(fault>=4&&fault!=7?1u:0u),"no retry or Submit before authority");throw;}
  need(fault==0&&submits==1&&calls==5,"one Submit and exact protocol");shape(result,{"format","campaignId","owner","kind","actionSha256","session","sequence","job"});shape(result.at("job"),{"sequence","payloadDigest","cancelRequested","state","decision"});need(result.at("sequence")=="0x8"&&result.at("job").at("decision")=="accepted"&&result.at("actionSha256")==digest(document.dump())&&result.dump().find(token)==std::string::npos,"accepted receipt has no authentication token");return calls;
 };
 const auto calls=protocol(0);for(int fault=1;fault<=9;++fault)reject([&]{(void)protocol(fault);});uint64_t checks=0;reject([&]{(void)accepted(action,token,[](const auto&,auto){return Json::object();},[&]{if(++checks==1)throw std::runtime_error("synthetic cancellation");});});reject([&]{(void)accepted(action,"bad-token",[](const auto&,auto){return Json::object();},check);});++groups;std::cout<<Json{{"scenario","fresh-lifecycle-managed-submit-ipc"},{"passed",true},{"callbackCalls",calls},{"expectedRefusals",11},{"realHttpUsed",false},{"execExecuted",false},{"productionQualified",false}}.dump()<<'\n';
 struct Temp {fs::path p;Temp(){auto s=(fs::temp_directory_path()/"nano-managed-submit-XXXXXX").string();need(::mkdtemp(s.data())!=nullptr,"temporary fixture");p=fs::canonical(s);}~Temp(){std::error_code e;fs::remove_all(p,e);}} temporary;
 auto write=[&](const fs::path& p,std::string_view raw,mode_t mode=0600){Fd f(::open(p.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,mode));need(::write(f.n,raw.data(),raw.size())==ssize_t(raw.size())&&::fsync(f.n)==0,"opaque fixture write");};
 const auto good=temporary.p/"opaque";write(good,"opaque-known-bytes");Files files(check);need(files.read(good,64)=="opaque-known-bytes","positive actual FD input");files.fence();
 {Fd f(::open(good.c_str(),O_WRONLY|O_NOFOLLOW|O_CLOEXEC));need(::pwrite(f.n,"X",1,0)==1,"byte mutation");}reject([&]{files.fence();});
 const auto metadata=temporary.p/"metadata";write(metadata,"metadata-only");Files timed(check);(void)timed.read(metadata,64);fs::last_write_time(metadata,fs::last_write_time(metadata)+std::chrono::seconds(1));reject([&]{timed.fence();});
 const auto target=temporary.p/"target";write(target,"regular-private");const auto link=temporary.p/"symlink";fs::create_symlink(target,link);reject([&]{Files f(check);(void)f.read(link,64);});
 const auto alias=temporary.p/"hardlink";fs::create_hard_link(target,alias);reject([&]{Files f(check);(void)f.read(target,64);});
 const auto wrong=temporary.p/"wrong-sha";write(wrong,"wrong-hash");reject([&]{Files f(check);(void)f.artifact(Json{{"path",wrong.string()},{"sha256",std::string(64,'0')},{"bytes",uint64_t(10)}},64);});
 const auto noexec=temporary.p/"not-executable";write(noexec,"not-executable");reject([&]{Files f(check);(void)f.artifact(Json{{"path",noexec.string()},{"sha256",digest("not-executable")},{"bytes",uint64_t(14)}},64,true);});
 ++groups;std::cout<<Json{{"scenario","fresh-lifecycle-managed-submit-input-fences"},{"passed",true},{"expectedRefusals",6},{"actualOpaqueFilesystemFixtures",true},{"databaseAccessed",false},{"productionQualified",false}}.dump()<<'\n';
 need(groups==3&&refusals==26,"all new protocol/file cases observed");std::cout<<Json{{"scenario","fresh-lifecycle-managed-submit-summary"},{"passed",true},{"groups",groups},{"expectedRefusals",refusals},{"signaturesCreated",0},{"envRead",false},{"databaseAccessed",false},{"networkUsed",false},{"execExecuted",false},{"productionQualified",false}}.dump()<<'\n';return 0;
}
} // namespace nano::test::managed_submission
