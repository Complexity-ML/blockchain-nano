#include "nano/consensus_capella_cold_records.hpp"
#include <atomic>
#include <condition_variable>

namespace nano {
struct CapellaCheckpointProvider::Shared {
 using Locator=CanonicalDepositContext::FetchLocator;
 std::mutex mutex;
 std::condition_variable retired;
 std::atomic<bool> closing{false};
 Fetch fetch;
 Locator locator;
 const bool paired;
 bool retiring=false,closed=false;
 // A stack, rather than one active pointer, also rejects A -> B -> A reentry.
 // It is thread-local stack storage: entering an operation allocates nothing.
 struct Frame {
  const Shared* owner;Frame* previous;
  static thread_local Frame* active;
  explicit Frame(const Shared* value):owner(value),previous(active){active=this;}
  ~Frame(){active=previous;}
 };
 static void refuse_reentry(const Shared* owner){
  for(auto* frame=Frame::active;frame;frame=frame->previous)
   if(frame->owner==owner)throw std::logic_error("Reentrant Capella checkpoint provider operation");
 }
 explicit Shared(Fetch f,Locator l={}):fetch(std::move(f)),locator(std::move(l)),paired(bool(locator)){}
 void before_read()const{
  refuse_reentry(this);
  if(closing.load(std::memory_order_acquire))throw std::runtime_error("Capella checkpoint provider closed");
 }
 void require_open()const{
  if(closing.load(std::memory_order_acquire)||closed||!fetch)
   throw std::runtime_error("Capella checkpoint provider closed");
 }
};
thread_local CapellaCheckpointProvider::Shared::Frame* CapellaCheckpointProvider::Shared::Frame::active=nullptr;

CapellaCheckpointProvider::CapellaCheckpointProvider(Fetch fetch){
 if(!fetch)throw std::invalid_argument("Missing Capella checkpoint provider");
 shared_=std::make_shared<Shared>(std::move(fetch));
}
CapellaCheckpointProvider::CapellaCheckpointProvider(Fetch fetch,CanonicalDepositContext::FetchLocator locator){
 if(!fetch||!locator)throw std::invalid_argument("Missing paired Capella checkpoint provider");
 shared_=std::make_shared<Shared>(std::move(fetch),std::move(locator));
}
CapellaCheckpointProvider::~CapellaCheckpointProvider(){close();}
CapellaCheckpointProvider::Fetch CapellaCheckpointProvider::reader()const{
 return [shared=shared_](const Bytes& digest,size_t bytes){
  if(digest.size()!=32||!bytes)throw std::invalid_argument("Invalid Capella checkpoint CAS request");
  shared->before_read();
  std::lock_guard lock(shared->mutex);shared->require_open();
  Shared::Frame active(shared.get());return shared->fetch(digest,bytes);
 };
}
CanonicalDepositContext::FetchLocator CapellaCheckpointProvider::locator_reader()const{
 if(!shared_->paired)throw std::invalid_argument("Capella checkpoint provider has no locator reader");
 return [shared=shared_](CanonicalDepositLookup kind,const Bytes& key){
  if((kind!=CanonicalDepositLookup::BeaconRoot&&kind!=CanonicalDepositLookup::ExecutionHash)||key.size()!=32)
   throw std::invalid_argument("Invalid Capella checkpoint locator request");
  shared->before_read();
  std::lock_guard lock(shared->mutex);shared->require_open();
  if(!shared->locator)throw std::runtime_error("Capella checkpoint provider closed");
  Shared::Frame active(shared.get());return shared->locator(kind,key);
 };
}
void CapellaCheckpointProvider::close(){
 Shared::refuse_reentry(shared_.get());
 // Publish closure before waiting for the active read, so queued readers can
 // never slip through between the final read and callback retirement.
 shared_->closing.store(true,std::memory_order_release);
 Fetch fetch;CanonicalDepositContext::FetchLocator locator;
 std::unique_lock lock(shared_->mutex);
 if(shared_->closed)return;
 if(shared_->retiring){shared_->retired.wait(lock,[&]{return shared_->closed;});return;}
 shared_->retiring=true;fetch=std::move(shared_->fetch);locator=std::move(shared_->locator);
 lock.unlock();
 {
  Shared::Frame retirement(shared_.get());
  // Captured owners may run destructors: no provider mutex is held here.
  fetch={};locator={};
 }
 lock.lock();shared_->closed=true;lock.unlock();shared_->retired.notify_all();
}
}
