#include <memory>
#include <atomic>
int main(){std::shared_ptr<const int> publication; auto next=std::make_shared<const int>(7);std::atomic_store_explicit(&publication,next,std::memory_order_release);auto snapshot=std::atomic_load_explicit(&publication,std::memory_order_acquire);if(!snapshot||*snapshot!=7)return 1;std::atomic_store_explicit(&publication,std::shared_ptr<const int>{},std::memory_order_release);return std::atomic_load_explicit(&publication,std::memory_order_acquire)?2:0;}
