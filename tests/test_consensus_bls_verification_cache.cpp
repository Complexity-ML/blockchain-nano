// Offline synthetic crypto/cache qualification. No campaign keys, fixtures,
// DB, RPC, node, filesystem state restore or network. Parent/child processes
// deliberately prove that certificates are never persisted across exec.
#include "nano/consensus_bls.hpp"
#include "nano/consensus_bls_verification_cache.hpp"
#include "../src/consensus_bls_verification_cache_detail.hpp"
#include "blst.h"
#include "json.hpp"
#include <openssl/crypto.h>
#include <atomic>
#include <future>
#include <iostream>
#include <sys/resource.h>
#include <signal.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
using namespace nano;
using Json=nlohmann::json;
namespace {
const auto began=std::chrono::steady_clock::now();
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint64_t wall_ms(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();}
uint64_t cpu_us(){rusage r{};check(getrusage(RUSAGE_SELF,&r)==0,"CPU observation");return uint64_t(r.ru_utime.tv_sec+r.ru_stime.tv_sec)*1000000+r.ru_utime.tv_usec+r.ru_stime.tv_usec;}
uint64_t rss_kib(){rusage r{};check(getrusage(RUSAGE_SELF,&r)==0,"RSS observation");
#ifdef __APPLE__
 return uint64_t(r.ru_maxrss)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void budget(){check(wall_ms()<60000&&rss_kib()<=131072,"synthetic test resource bound");}
struct Sample {std::vector<uint8_t> key,message,signature;};
Sample sample(uint64_t index,size_t message_bytes=32){
 budget();blst_scalar scalar{};uint64_t words[4]{500701+index,0,0,0};blst_scalar_from_uint64(&scalar,words);blst_p1 public_point;blst_sk_to_pk_in_g1(&public_point,&scalar);
 Sample out{std::vector<uint8_t>(48),std::vector<uint8_t>(message_bytes),std::vector<uint8_t>(96)};blst_p1_compress(out.key.data(),&public_point);
 for(size_t i=0;i<message_bytes;++i)out.message[i]=uint8_t((index*17+i*31)%256);
 static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 hashed,sig;
 blst_hash_to_g2(&hashed,out.message.data(),out.message.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&sig,&hashed,&scalar);blst_p2_compress(out.signature.data(),&sig);OPENSSL_cleanse(&scalar,sizeof(scalar));return out;
}
bool verify(const Sample& s){budget();return verify_consensus_bls(s.key,s.message,s.signature);}
using Cache=bls_verification_cache_detail::PositiveVerificationCache;
auto genuine_backend=[](auto key,auto message,auto signature){return verify_consensus_bls(key,message,signature);};
void cold_process(){
 const auto before=consensus_bls_verification_cache_stats();check(before.entries==0&&before.calls==0&&before.in_flight==0,"fresh process inherited certificates");
 const auto s=sample(0);check(verify(s)&&verify(s),"fresh process genuine verification");const auto after=consensus_bls_verification_cache_stats();check(after.raw_calls==1&&after.hits==1&&after.entries==1,"fresh process must authenticate before hit");
 std::cout<<Json{{"scenario","bls-cache-fresh-exec-cold"},{"result","PASS"},{"rawCryptoCalls",after.raw_calls},{"hits",after.hits},{"persistenceUsed",false},{"productionQualified",false}}.dump()<<std::endl;
}
void byte_boundaries(const Sample& original){
 check(configure_consensus_bls_verification_cache({0,0}),"disable");check(verify(original)&&verify(original),"disabled genuine control");auto disabled=consensus_bls_verification_cache_stats();check(disabled.hits==0&&disabled.raw_calls==2&&disabled.entries==0,"disabled verification must remain uncached");
 check(configure_consensus_bls_verification_cache({}),"default capacity");check(verify(original)&&verify(original),"genuine cached control");auto warm=consensus_bls_verification_cache_stats();check(warm.raw_calls==1&&warm.hits==1&&warm.entries==1,"positive certificate not reused");
 size_t mutations=0;
 for(size_t part=0;part<3;++part)for(size_t at=0;at<(part==0?48:part==1?32:96);++at){
  auto changed=original;auto& bytes=part==0?changed.key:part==1?changed.message:changed.signature;bytes[at]^=1;
  const auto before=consensus_bls_verification_cache_stats();check(!verify(changed)&&!verify(changed),"changed byte accepted");const auto after=consensus_bls_verification_cache_stats();check(after.hits==before.hits&&after.raw_calls==before.raw_calls+2&&after.entries==1,"negative result or altered tuple reused");++mutations;
 }
 auto malformed=original;malformed.key.resize(47);check(!verify(malformed),"wrong public key width");malformed=original;malformed.signature.resize(95);check(!verify(malformed),"wrong signature width");
 auto infinity=original;std::fill(infinity.key.begin(),infinity.key.end(),0);infinity.key[0]=0xc0;check(!verify(infinity),"G1 infinity accepted");infinity=original;std::fill(infinity.signature.begin(),infinity.signature.end(),0);infinity.signature[0]=0xc0;check(!verify(infinity),"G2 infinity accepted");
 for(size_t width:{size_t(0),size_t(31),size_t(33)}){const auto arbitrary=sample(100+width,width);const auto before=consensus_bls_verification_cache_stats();check(verify(arbitrary)&&verify(arbitrary),"non32 genuine message altered");const auto after=consensus_bls_verification_cache_stats();check(after.raw_calls==before.raw_calls+2&&after.hits==before.hits&&after.entries==before.entries,"non32 message cached");}
 std::cout<<Json{{"scenario","bls-positive-cache-byte-boundaries"},{"result","PASS"},{"mutatedExactTupleBytes",mutations},{"negativeCertificatesStored",false},{"non32GenuineMessagesUncached",true},{"malformedAndInfinityRejected",true},{"productionQualified",false}}.dump()<<std::endl;
}
void instance_boundaries(const Sample& a,const Sample& b){
 check(configure_consensus_bls_verification_cache({0,0}),"raw genuine backend");Cache local({0,0});const auto fixed=local.stats().fixed_metadata_bytes;
 check(local.configure({fixed+176,8}),"one-entry byte capacity");check(local.verify(a.key,a.message,a.signature,genuine_backend)&&local.verify(b.key,b.message,b.signature,genuine_backend)&&local.verify(a.key,a.message,a.signature,genuine_backend),"bounded cache genuine controls");auto stats=local.stats();check(stats.capacity_entries==1&&stats.entries==1&&stats.evictions==2&&stats.resident_bytes<=fixed+176,"entry/vector/fixed-metadata budget incorrect");
 bool invalid=false;try{local.configure({fixed+175,1});}catch(const std::invalid_argument&){invalid=true;}check(invalid,"too-small budget silently accepted");
 local.configure({0,0});bool allocated=local.configure({65536,8},[](size_t)->std::vector<std::array<uint8_t,176>>{throw std::bad_alloc();});check(!allocated&&local.stats().capacity_entries==0&&local.verify(a.key,a.message,a.signature,genuine_backend),"optional cache allocation failure changed valid bool");check(local.stats().entries==0&&local.stats().allocation_failures==1,"failed allocation installed state");
 local.configure({65536,8});auto original=a;bool snapshot_called=false;
 check(local.verify(original.key,original.message,original.signature,[&](auto key,auto message,auto signature){snapshot_called=true;original.message[0]^=1;check(std::equal(a.key.begin(),a.key.end(),key.begin())&&std::equal(a.message.begin(),a.message.end(),message.begin())&&std::equal(a.signature.begin(),a.signature.end(),signature.begin()),"raw backend did not receive exact copied176");return genuine_backend(key,message,signature);}),"copied tuple verification changed");
 check(snapshot_called&&!local.verify(original.key,original.message,original.signature,genuine_backend),"split-read mutation inserted wrong certificate");
 local.clear();bool exception=false;try{local.verify(a.key,a.message,a.signature,[](auto,auto,auto)->bool{throw std::runtime_error("backend exception control");});}catch(const std::runtime_error&){exception=true;}check(exception&&local.stats().in_flight==0&&local.stats().entries==0,"backend exception cached or swallowed");
 std::cout<<Json{{"scenario","bls-cache-instance-budget-allocation-snapshot"},{"result","PASS"},{"byteBudgetIncludesFixedMetadata",true},{"optionalAllocationFailureFallsBackToGenuineCrypto",true},{"copied176VerifiedAndMemoized",true},{"productionQualified",false}}.dump()<<std::endl;
}
void clear_inflight(const Sample& s,bool disable){
 Cache local({65536,8});std::promise<void> entered,release;auto entered_future=entered.get_future(),release_future=release.get_future();bool result=false;std::exception_ptr failure;
 std::thread worker([&]{try{result=local.verify(s.key,s.message,s.signature,[&](auto key,auto message,auto signature){entered.set_value();release_future.wait();return genuine_backend(key,message,signature);});}catch(...){failure=std::current_exception();}});
 try{
  check(entered_future.wait_for(std::chrono::seconds(5))==std::future_status::ready,"inflight scheduling double deadline");const auto old=local.stats();check(old.in_flight==1,"actual instance miss did not enter backend");
  if(disable)check(local.configure({0,0}),"disable while inflight");else local.clear();const auto cleared=local.stats();check(cleared.generation==old.generation+1&&cleared.entries==0&&cleared.in_flight==1,"clear lied about outstanding crypto");
  release.set_value();worker.join();if(failure)std::rethrow_exception(failure);auto done=local.stats();check(result&&done.in_flight==0&&done.entries==0&&done.stale_completions==1,"old inflight completion reinserted after clear");
 }catch(...){try{release.set_value();}catch(const std::future_error&){}if(worker.joinable())worker.join();throw;}
}
void concurrent_clear(const std::vector<Sample>& samples){
 check(configure_consensus_bls_verification_cache({}),"concurrent default capacity");std::atomic<bool> failed=false;std::vector<std::thread> workers;
 for(size_t n=0;n<4;++n)workers.emplace_back([&,n]{try{for(size_t i=0;i<64;++i)if(!verify(samples[(i+n)%samples.size()]))failed=true;}catch(...){failed=true;}});
 for(size_t i=0;i<32;++i)clear_consensus_bls_verification_cache();for(auto& thread:workers)thread.join();check(!failed,"concurrent clear changed genuine result");auto done=consensus_bls_verification_cache_stats();check(done.in_flight==0&&done.entries<=done.capacity_entries&&done.resident_bytes<=done.maximum_bytes,"concurrent budget/accounting");clear_consensus_bls_verification_cache();check(consensus_bls_verification_cache_stats().entries==0,"terminal clear did not clear");
 std::cout<<Json{{"scenario","bls-cache-concurrent-clear-generation"},{"result","PASS"},{"actualGlobalGenuineVerificationCalls",256},{"controlledWaitUsesSeparateInstance",true},{"oldInflightCannotReinsert",true},{"globalVerifierInjectionUsed",false},{"productionQualified",false}}.dump()<<std::endl;
}
void benchmark(const std::vector<Sample>& samples){
 Json results=Json::array();for(bool cached:{false,true}){
  check(configure_consensus_bls_verification_cache(cached?ConsensusBlsVerificationCacheConfig{}:ConsensusBlsVerificationCacheConfig{0,0}),"benchmark capacity");const auto began_wall=wall_ms(),began_cpu=cpu_us();for(size_t i=0;i<512;++i)check(verify(samples[i%samples.size()]),"benchmark genuine bool differs");const auto stats=consensus_bls_verification_cache_stats();
  check(stats.raw_calls==(cached?64:512)&&stats.hits==(cached?448:0)&&stats.entries==(cached?64:0),"benchmark calls/hits not observed");results.push_back(Json{{"cacheEnabled",cached},{"genuineTuples",64},{"verificationCalls",512},{"rawCryptoCalls",stats.raw_calls},{"hits",stats.hits},{"rawCryptoNanoseconds",stats.raw_crypto_nanoseconds},{"wallMs",wall_ms()-began_wall},{"processCpuUs",cpu_us()-began_cpu},{"residentCacheBytes",stats.resident_bytes},{"fixedMetadataBytes",stats.fixed_metadata_bytes},{"productionQualified",false}});
 }
 std::cout<<Json{{"scenario","bls-cache-bounded-local-benchmark"},{"result","PASS"},{"samples",results},{"provesNaturalChainDeadline",false},{"productionQualified",false}}.dump()<<std::endl;
}
void fresh_exec(const char* executable){
 const auto pid=fork();check(pid>=0,"fresh process fork");if(!pid){execl(executable,executable,"--cold-check",nullptr);_exit(127);}int status=0;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);bool complete=false;
 while(std::chrono::steady_clock::now()<deadline){const auto done=waitpid(pid,&status,WNOHANG);check(done>=0,"fresh exec wait");if(done==pid){complete=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 if(!complete){kill(pid,SIGKILL);(void)waitpid(pid,&status,0);throw std::runtime_error("fresh exec five-second deadline");}check(WIFEXITED(status)&&WEXITSTATUS(status)==0,"fresh exec did not authenticate cold");
}
}
int main(int argc,char** argv){try{
 if(argc==2&&std::string(argv[1])=="--cold-check"){cold_process();return 0;}check(argc==2&&std::string(argv[1])=="--self-test","usage: --self-test or --cold-check");
 std::vector<Sample> samples;for(size_t i=0;i<64;++i)samples.push_back(sample(i));byte_boundaries(samples[0]);instance_boundaries(samples[0],samples[1]);clear_inflight(samples[0],false);clear_inflight(samples[0],true);concurrent_clear(samples);benchmark(samples);fresh_exec(argv[0]);budget();
 std::cout<<Json{{"scenario","bls-positive-cache-self-test"},{"result","PASS"},{"syntheticKeysOnly",true},{"maxRssKiB",rss_kib()},{"elapsedMs",wall_ms()},{"networkQualified",false},{"runtimeDutiesResumed",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<Json{{"result","FAIL"},{"message",e.what()},{"networkQualified",false},{"productionQualified",false}}.dump()<<std::endl;return 1;}}
