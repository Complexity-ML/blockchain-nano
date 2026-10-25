#include "nano/consensus_capella_cold_records.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>

namespace {
using namespace nano;
using namespace std::chrono_literals;
using Provider=CapellaCheckpointProvider;
using Locator=CanonicalDepositContext::FetchLocator;
const Bytes key(32,0x36);
const CanonicalDepositLocator answer{7,ConsensusRoot{0x81}};
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class Error,class Call>void rejects(Call call,const char* why){bool refused=false;try{call();}catch(const Error&){refused=true;}check(refused,why);}
template<class T>void ready(std::future<T>& future,const char* why){check(future.wait_for(10s)==std::future_status::ready,why);}
struct Release {
 std::promise<void>& promise;bool released=false;
 void now(){if(!released){released=true;promise.set_value();}}
 ~Release(){try{now();}catch(...){}}
};

void legacy_and_arguments(){
 size_t cas=0,locators=0;
 rejects<std::invalid_argument>([]{Provider p(Provider::Fetch{});},"legacy empty constructor refused");
 rejects<std::invalid_argument>([]{Provider p([](const Bytes&,size_t)->std::optional<std::string>{return {};},Locator{});},"paired empty locator constructor refused");
 Provider legacy([&](const Bytes&,size_t maximum)->std::optional<std::string>{++cas;check(maximum==9,"legacy bound preserved");return "legacy";});
 const auto old=legacy.reader();check(old(key,9)=="legacy","legacy constructor/reader symbol and payload preserved");rejects<std::invalid_argument>([&]{legacy.locator_reader();},"unary provider cannot manufacture locator capability");legacy.close();legacy.close();rejects<std::runtime_error>([&]{old(key,9);},"legacy retained reader refuses after close");check(cas==1,"legacy close reaches no backing callback");
 Provider paired([&](const Bytes&,size_t)->std::optional<std::string>{++cas;return {};},[&](CanonicalDepositLookup kind,const Bytes&)->std::optional<CanonicalDepositLocator>{++locators;check(kind==CanonicalDepositLookup::BeaconRoot||kind==CanonicalDepositLookup::ExecutionHash,"valid enum reaches callback");return answer;});
 const auto read=paired.reader();const auto locate=paired.locator_reader();
 rejects<std::invalid_argument>([&]{read(Bytes(31),1);},"short CAS key refused before backing callback");rejects<std::invalid_argument>([&]{read(Bytes(33),1);},"long CAS key refused before backing callback");rejects<std::invalid_argument>([&]{read(key,0);},"zero CAS bound refused before backing callback");
 for(const auto kind:{CanonicalDepositLookup::BeaconRoot,CanonicalDepositLookup::ExecutionHash}){rejects<std::invalid_argument>([&]{locate(kind,Bytes(31));},"short locator key refused before callback");rejects<std::invalid_argument>([&]{locate(kind,Bytes(33));},"long locator key refused before callback");}
 rejects<std::invalid_argument>([&]{locate(static_cast<CanonicalDepositLookup>(-1),key);},"negative locator kind refused");rejects<std::invalid_argument>([&]{locate(static_cast<CanonicalDepositLookup>(2),key);},"unknown locator kind refused");check(cas==1&&locators==0,"malformed requests never reach either backing callback");
 check(!read(key,1)&&locate(CanonicalDepositLookup::BeaconRoot,key)==answer&&locate(CanonicalDepositLookup::ExecutionHash,key)==answer,"paired optional results and both typed kinds preserved");paired.close();rejects<std::runtime_error>([&]{read(key,1);},"paired CAS retained reader closed");rejects<std::runtime_error>([&]{locate(CanonicalDepositLookup::BeaconRoot,key);},"paired locator retained reader closed");check(cas==2&&locators==2,"neither callback runs after paired closure");
 const auto escaped=[&]{Provider scoped([&](const Bytes&,size_t)->std::optional<std::string>{++cas;return "unreachable";},[&](CanonicalDepositLookup,const Bytes&)->std::optional<CanonicalDepositLocator>{++locators;return answer;});return std::pair{scoped.reader(),scoped.locator_reader()};}();
 rejects<std::runtime_error>([&]{escaped.first(key,1);},"provider destructor closes retained CAS reader");rejects<std::runtime_error>([&]{escaped.second(CanonicalDepositLookup::BeaconRoot,key);},"provider destructor closes retained locator reader");check(cas==2&&locators==2,"escaped reader pair cannot resurrect destroyed provider callbacks");
}

void foreign_contention(bool closing_case){
 std::promise<void> entered_a,entered_b,release_promise;auto a_entered=entered_a.get_future(),b_entered=entered_b.get_future();auto released=release_promise.get_future().share();std::atomic<unsigned> calls_a=0,calls_b=0;
 Provider::Fetch read_a,read_b;Provider* owner_a=nullptr;Provider* owner_b=nullptr;
 Provider a([&](const Bytes&,size_t)->std::optional<std::string>{if(calls_a.fetch_add(1)==0){entered_a.set_value();released.wait();if(closing_case)owner_b->close();else read_b(key,1);}return "ok";});
 Provider b([&](const Bytes&,size_t)->std::optional<std::string>{if(calls_b.fetch_add(1)==0){entered_b.set_value();released.wait();if(closing_case)owner_a->close();else read_a(key,1);}return "ok";});
 owner_a=&a;owner_b=&b;read_a=a.reader();read_b=b.reader();
 std::future<bool> first,second;Release release{release_promise};
 const auto call=[](const Provider::Fetch& read){try{check(read(key,1)=="ok","foreign callback result");return false;}catch(const std::logic_error&){return true;}};
 first=std::async(std::launch::async,[&]{return call(read_a);});second=std::async(std::launch::async,[&]{return call(read_b);});
 ready(a_entered,"first foreign callback reached held gate");a_entered.get();ready(b_entered,"second foreign callback reached held gate");b_entered.get();release.now();
 ready(first,"first nested foreign operation completed");ready(second,"second nested foreign operation completed");const bool refused_a=first.get(),refused_b=second.get();check(refused_a||refused_b,"at least one contended nested operation refused rather than a two-gate wait cycle");
 if(!closing_case)check(read_a(key,1)=="ok"&&read_b(key,1)=="ok","foreign read contention does not poison either open gate");a.close();b.close();
}

void reentry_and_exceptions(){
 Provider::Fetch read;Locator locate;Provider* self=nullptr;unsigned mode=0,cas=0,locators=0;
 Provider provider([&](const Bytes&,size_t)->std::optional<std::string>{++cas;if(mode==1)locate(CanonicalDepositLookup::BeaconRoot,key);if(mode==2)read(key,1);if(mode==3)self->close();if(mode==4)throw std::runtime_error("CAS callback marker");return "cas";},[&](CanonicalDepositLookup,const Bytes&)->std::optional<CanonicalDepositLocator>{++locators;if(mode==5)read(key,1);if(mode==6)locate(CanonicalDepositLookup::ExecutionHash,key);if(mode==7)self->close();if(mode==8)throw std::runtime_error("locator callback marker");return answer;});self=&provider;read=provider.reader();locate=provider.locator_reader();
 for(mode=1;mode<=3;++mode)rejects<std::logic_error>([&]{read(key,1);},"CAS crossed/self/close reentry refused instead of deadlock");
 for(mode=5;mode<=7;++mode)rejects<std::logic_error>([&]{locate(CanonicalDepositLookup::BeaconRoot,key);},"locator crossed/self/close reentry refused instead of deadlock");
 mode=4;bool original=false;try{read(key,1);}catch(const std::runtime_error& error){original=std::string(error.what())=="CAS callback marker";}check(original,"CAS original callback exception retained");
 mode=8;original=false;try{locate(CanonicalDepositLookup::BeaconRoot,key);}catch(const std::runtime_error& error){original=std::string(error.what())=="locator callback marker";}check(original,"locator original callback exception retained");
 mode=0;check(read(key,1)=="cas"&&locate(CanonicalDepositLookup::BeaconRoot,key)==answer&&cas==5&&locators==5,"failed reentry/callbacks unwind the gate and do not close the provider");provider.close();
 // A -> B -> A must not lose A when another provider becomes the top frame.
 Provider::Fetch first,second;
 Provider a([&](const Bytes&,size_t)->std::optional<std::string>{return second(key,1);});
 Provider b([&](const Bytes&,size_t)->std::optional<std::string>{return first(key,1);});first=a.reader();second=b.reader();rejects<std::logic_error>([&]{first(key,1);},"multi-provider cycle reentry refused");a.close();b.close();
 foreign_contention(false);foreign_contention(true);
}

void inflight_close(bool locator_case){
 std::promise<void> entered,release_promise,close_attempted;auto entry=entered.get_future();auto released=release_promise.get_future().share();auto attempted=close_attempted.get_future();
 std::atomic<bool> reading=false,finished=false,unsafe_retirement=false;std::atomic<unsigned> deaths=0,cas=0,locators=0;
 struct Owner {std::atomic<bool>& reading;std::atomic<bool>& unsafe;std::atomic<unsigned>& deaths;~Owner(){if(reading.load())unsafe.store(true);++deaths;}};
 auto owner=std::make_shared<Owner>(reading,unsafe_retirement,deaths);std::weak_ptr<Owner> weak=owner;
 const auto block=[&]{struct Reading {std::atomic<bool>& value;explicit Reading(std::atomic<bool>& v):value(v){value.store(true);}~Reading(){value.store(false);}} active(reading);entered.set_value();released.wait();finished.store(true);};
 Provider provider([&,keep=owner](const Bytes&,size_t)->std::optional<std::string>{++cas;if(!locator_case)block();return "cas";},[&,keep=owner](CanonicalDepositLookup,const Bytes&)->std::optional<CanonicalDepositLocator>{++locators;if(locator_case)block();return answer;});owner.reset();
 const auto read=provider.reader();const auto locate=provider.locator_reader();
 // Declare the release guard AFTER both future owners, so every failure
 // releases the callback before either async future destructor can join it.
 std::future<void> operation,closing;Release release{release_promise};
 operation=std::async(std::launch::async,[&]{if(locator_case)check(locate(CanonicalDepositLookup::BeaconRoot,key)==answer,"inflight locator result unchanged");else check(read(key,1)=="cas","inflight CAS result unchanged");});
 ready(entry,"backing operation reached its controlled latch");entry.get();check(reading.load()&&!finished.load()&&deaths.load()==0,"backing owner alive during known active callback");
 closing=std::async(std::launch::async,[&]{close_attempted.set_value();provider.close();check(finished.load()&&!reading.load(),"close cannot complete before active callback exits");});
 ready(attempted,"close thread reached invocation marker");attempted.get();check(closing.wait_for(0s)==std::future_status::timeout,"close remains pending while callback release latch is held");check(!weak.expired()&&!unsafe_retirement.load(),"neither captured owner retired during active callback");
 release.now();ready(operation,"inflight operation completed after release");operation.get();ready(closing,"close joined active operation");closing.get();check(finished.load()&&!reading.load()&&weak.expired()&&deaths.load()==1&&!unsafe_retirement.load(),"both closures retired their shared backing owner once after operation join");
 rejects<std::runtime_error>([&]{read(key,1);},"CAS refused after joined closure");rejects<std::runtime_error>([&]{locate(CanonicalDepositLookup::ExecutionHash,key);},"locator refused after joined closure");check(cas.load()==unsigned(!locator_case)&&locators.load()==unsigned(locator_case),"postclose requests do not reach either backing callback");
}
}

int main(){try{legacy_and_arguments();reentry_and_exceptions();inflight_close(false);inflight_close(true);std::cout<<nano::Json{{"scenario","checkpoint-provider-paired-lifetime"},{"passed",true},{"groups",4},{"inflightKinds",2},{"arbitrarySleeps",0},{"pairedCallbackGate",true},{"reentryRejected",true},{"legacyValidCallsPreserved",true},{"runtimePublicationQualified",false},{"migrationQualified",false},{"physicalPowerLossQualified",false},{"productionQualified",false}}.dump()<<'\n';return 0;}catch(const std::exception& error){std::cerr<<nano::Json{{"scenario","checkpoint-provider-paired-lifetime"},{"passed",false},{"reason",error.what()},{"productionQualified",false}}.dump()<<'\n';return 1;}}
