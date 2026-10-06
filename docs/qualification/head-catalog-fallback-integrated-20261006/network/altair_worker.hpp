#pragma once
#include "altair_work.hpp"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

// One ordered consensus worker. Limits cover queued AND running payloads, and
// are local resource policy, not consensus/validator limits. The handler must
// check cancellation before durable commit; after commit it must return the
// committed decision. Handler and completion must not destroy/join this queue.
class AltairWorker {
public:
 using Decision=AltairWork::Decision;
 using Cancellation=AltairWork::Cancellation;
 using Done=AltairWork::Done;
 using Handler=std::function<Decision(const AltairWireMessage&,const Cancellation&)>;
private:
 struct Job {AltairWireMessage message;Cancellation cancelled;Done done;size_t bytes;};
 const size_t max_jobs_,max_bytes_;
 Handler handler_;
 std::mutex mutex_;
 std::condition_variable ready_;
 std::deque<Job> pending_;
 Cancellation running_;
 size_t jobs_=0,bytes_=0;
 bool stopping_=false;
 std::thread worker_;
 void run(){
  for(;;){
   std::optional<Job> job;
   {
    std::unique_lock lock(mutex_);
    ready_.wait(lock,[&]{return stopping_||!pending_.empty();});
    if(pending_.empty())return;
    job.emplace(std::move(pending_.front()));pending_.pop_front();running_=job->cancelled;
   }
   auto decision=Decision::local_failure;
   if(!job->cancelled->load()){
    try{decision=handler_(job->message,job->cancelled);}catch(...){decision=Decision::local_failure;}
   }
   // Completion is invoked exactly once, after the handler (and commit) ends.
   // Completion callbacks must not throw, as with the stream adapter's Done.
   job->done(decision);
   {
    std::lock_guard lock(mutex_);
    --jobs_;bytes_-=job->bytes;running_.reset();
   }
  }
 }
public:
 AltairWorker(size_t maximum_jobs,size_t maximum_bytes,Handler handler):
 max_jobs_(maximum_jobs),max_bytes_(maximum_bytes),handler_(std::move(handler)){
  if(!max_jobs_||!max_bytes_||!handler_)throw std::invalid_argument("invalid Altair worker limits");
  worker_=std::thread([this]{run();});
 }
 AltairWorker(const AltairWorker&)=delete;
 AltairWorker& operator=(const AltairWorker&)=delete;
 ~AltairWorker(){stop();join();}
 bool submit(AltairWireMessage message,Cancellation cancelled,Done done){
  if(!cancelled||!done)return false;
  // Account allocation capacity rather than logical length (including spare bytes).
  const auto bytes=message.ssz.capacity();
  std::lock_guard lock(mutex_);
  if(stopping_||cancelled->load()||jobs_>=max_jobs_||bytes>max_bytes_-bytes_)return false;
  pending_.push_back(Job{std::move(message),std::move(cancelled),std::move(done),bytes});
  ++jobs_;bytes_+=bytes;ready_.notify_one();return true;
 }
 AltairWork::Dispatch dispatcher(){
  return [this](AltairWireMessage message,Cancellation cancelled,Done done){return submit(std::move(message),std::move(cancelled),std::move(done));};
 }
 // Safe from I/O thread. No new work admitted; pending tasks complete as local
 // failures without entering consensus. Running work is cancelled cooperatively.
 void stop(){
  std::lock_guard lock(mutex_);stopping_=true;
  if(running_)running_->store(true);
  for(auto& job:pending_)job.cancelled->store(true);
  ready_.notify_all();
 }
 // Owner thread only, after stop; do not block the network I/O loop on join.
 // Keep I/O executor alive until posted stream completions have been drained.
 void join(){if(worker_.joinable())worker_.join();}
};
