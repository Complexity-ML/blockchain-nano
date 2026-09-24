#include "nano/block_import.hpp"
#include "support/process.hpp"
using namespace test;
int main(){try{
 auto now=BlockImports::Clock::time_point{};Bytes raw(300000,49);const auto block_hash=hex(Bytes(32,7)),tx_hash=hex(keccak(raw));
 Json manifest={{"version",1},{"block",{{"hash",block_hash},{"transactionEncoding","signed-rlp-hashes-v1"},{"transactions",Json::array({tx_hash})}}}};
 auto part=[&](size_t offset){auto end=std::min(raw.size(),offset+256*1024);return Json{{"version",1},{"blockHash",block_hash},{"transactionHash",tx_hash},{"offset",quantity(Int(offset))},{"total",quantity(Int(raw.size()))},{"next",end==raw.size()?Json(nullptr):Json(quantity(Int(end)))},{"data",hex(Bytes(raw.begin()+offset,raw.begin()+end))}};};
 BlockImports queue({1024*1024,1,5s});unsigned refused=0;
 auto reject=[&](const auto& fn){bool failed=false;try{fn();}catch(const std::exception&){failed=true;}check(failed,"invalid import operation accepted");++refused;};
 auto id=queue.begin(manifest,now).at("id").get<std::string>();auto initial=queue.used_bytes();reject([&]{queue.begin(manifest,now);});reject([&]{queue.finish(id,now);});check(queue.sessions()==1&&queue.used_bytes()==initial,"early finish or session quota changed retained payload");
 auto first=part(0),last=part(256*1024);
 reject([&]{queue.append(id,0,last,now);});check(queue.used_bytes()==initial,"gap allocates payload");
 auto bad=first;bad["blockHash"]=hex(Bytes(32,8));reject([&]{queue.append(id,0,bad,now);});
 bad=first;bad["total"]="0x200000";bad["next"]="0x40000";reject([&]{queue.append(id,0,bad,now);});
 queue.append(id,0,first,now);const auto charged=queue.used_bytes();queue.append(id,0,first,now+4s);check(queue.used_bytes()==charged,"duplicate fragment charged twice");
 bad=first;bad["data"].get_ref<std::string&>()[2]='0';reject([&]{queue.append(id,0,bad,now);});check(queue.used_bytes()==charged,"conflicting duplicate changed retained bytes");
 queue.expire(now+5s);check(queue.used_bytes()==0&&queue.sessions()==0,"expiry frees abandoned payload despite duplicates");reject([&]{queue.append(id,0,last,now+5s);});
 id=queue.begin(manifest,now+5s).at("id").get<std::string>();queue.append(id,0,first,now+5s);queue.append(id,0,last,now+5s);const auto completed_charge=queue.used_bytes();queue.append(id,0,last,now+5s);check(queue.used_bytes()==completed_charge,"completed transaction duplicate charged twice");auto complete=queue.finish(id,now+5s);check(complete.at("transactions").at(0)==hex(raw)&&complete.at("transactionEncoding")=="signed-rlp-v1"&&queue.used_bytes()==0&&queue.sessions()==0,"finish reconstructs exact bytes and releases quota");reject([&]{queue.finish(id,now+5s);});
 id=queue.begin(manifest,now+5s).at("id").get<std::string>();queue.append(id,0,first,now+5s);bad=last;auto changed=unhex(bad.at("data"));changed.back()^=1;bad["data"]=hex(changed);reject([&]{queue.append(id,0,bad,now+5s);});check(queue.used_bytes()==0&&queue.sessions()==0,"bad final digest discards entire assembly");
 id=queue.begin(manifest,now+5s).at("id").get<std::string>();check(queue.abort(id,now+5s)&&!queue.abort(id,now+5s)&&queue.used_bytes()==0,"abort idempotently releases quota");
 auto duplicate=manifest;duplicate["block"]["transactions"].push_back(tx_hash);reject([&]{queue.begin(duplicate,now);});
 // Aggregate exhaustion is independent of the per-transaction declared length.
 BlockImports shared({400000,2,5s});auto one=shared.begin(manifest,now).at("id").get<std::string>(),two=shared.begin(manifest,now).at("id").get<std::string>();shared.append(one,0,first,now);auto before=shared.used_bytes();reject([&]{shared.append(two,0,first,now);});check(shared.used_bytes()==before,"aggregate rejection does not consume bytes");shared.abort(one,now);shared.append(two,0,first,now);shared.abort(two,now);check(shared.used_bytes()==0,"released shared capacity is reusable");
 // Block and proposal assemblies share capacity but cannot use each other's finish operation.
 Json envelope={{"version",1},{"height","0x1"},{"round","0x0"},{"proposal",manifest.at("block")},{"votes",Json::array()},{"previous",nullptr}};
 id=queue.begin_proposal(envelope,now).at("id").get<std::string>();auto metadata=envelope;metadata["proposal"]=nullptr;
 check(queue.used_bytes()==manifest.dump().size()+metadata.dump().size(),"proposal metadata charged to shared payload budget");
 reject([&]{queue.begin(manifest,now);});reject([&]{queue.finish(id,now);});queue.append(id,0,first,now);queue.append(id,0,last,now);reject([&]{queue.finish(id,now);});auto rebuilt=queue.finish_proposal(id,now);check(rebuilt.at("proposal").at("transactions").at(0)==hex(raw)&&rebuilt.at("round")=="0x0"&&queue.used_bytes()==0,"proposal reconstruction preserves metadata and releases shared budget");
 id=queue.begin(manifest,now).at("id").get<std::string>();reject([&]{queue.finish_proposal(id,now);});check(queue.sessions()==1,"wrong purpose must not consume assembly");queue.abort(id,now);
 id=queue.begin_proposal(envelope,now).at("id").get<std::string>();queue.append(id,0,first,now);queue.expire(now+5s);check(queue.used_bytes()==0&&queue.sessions()==0,"proposal metadata and bytes released on expiry");
 std::cout<<Json{{"scenario","block-import-budget"},{"rejectedCases",refused},{"duplicateIdempotence",true},{"expiryReleases",true},{"hashFailureReleases",true},{"aggregateBudgetReused",true},{"completeBytes",raw.size()}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
