#include "nano/consensus_capella_cold_records.hpp"
#include <atomic>
namespace nano {
struct CapellaCheckpointProvider::Shared {std::mutex mutex;std::atomic<bool> closing{false};Fetch fetch;explicit Shared(Fetch f):fetch(std::move(f)){};};
CapellaCheckpointProvider::CapellaCheckpointProvider(Fetch fetch){
 if(!fetch)throw std::invalid_argument("Missing Capella checkpoint provider");
 shared_=std::make_shared<Shared>(std::move(fetch));
}
CapellaCheckpointProvider::~CapellaCheckpointProvider(){close();}
CapellaCheckpointProvider::Fetch CapellaCheckpointProvider::reader()const{
 return [shared=shared_](const Bytes& digest,size_t bytes){
  if(shared->closing.load(std::memory_order_acquire))throw std::runtime_error("Capella checkpoint provider closed");
  std::lock_guard lock(shared->mutex);
  if(shared->closing.load(std::memory_order_acquire)||!shared->fetch)throw std::runtime_error("Capella checkpoint provider closed");
  return shared->fetch(digest,bytes);
 };
}
void CapellaCheckpointProvider::close(){
 shared_->closing.store(true,std::memory_order_release);
 Fetch retired;
 {std::lock_guard lock(shared_->mutex);retired=std::move(shared_->fetch);}
 // Backing ownership may run destructors; release it outside the gate mutex.
}
}
