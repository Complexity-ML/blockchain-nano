#include "nano/consensus.hpp"
#include <future>
#include <iostream>
using namespace nano;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 Block block;block.header.difficulty=1;
 Miner miner;std::promise<Block> submitted;auto result=submitted.get_future();bool offered=false;
 miner.start([&]()->std::optional<Block>{if(offered)return std::nullopt;offered=true;return block;},[&](const Block& mined){submitted.set_value(mined);});
 require(result.wait_for(std::chrono::seconds(3))==std::future_status::ready,"background proof timed out");
 auto mined=result.get();miner.stop();
 require(!miner.is_mining()&&miner.error().empty(),"worker stops cleanly");
 require(ProofOfWork::verify(mined.header)&&mined.header.mix_hash==ProofOfWork::hash(mined.header),"submitted block contains valid proof");
 require(miner.hashrate()>0,"hashrate retained after successful proof");
 std::promise<void> idle;auto idle_ready=idle.get_future();bool notified=false;
 miner.start([&]()->std::optional<Block>{if(!notified){notified=true;idle.set_value();}return std::nullopt;},[](const Block&){});
 require(idle_ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"idle worker starts");
 bool rejected=false;try{miner.start([]{return std::optional<Block>();},[](const Block&){});}catch(const std::logic_error&){rejected=true;}
 require(rejected,"double start rejected");miner.stop();require(!miner.is_mining(),"idle wait is interruptible");
 block.header.difficulty=(Int(1)<<256)-1;std::promise<void> hard;auto hard_ready=hard.get_future();
 miner.start([&]()->std::optional<Block>{hard.set_value();return block;},[](const Block&){throw std::runtime_error("unexpected hard proof");});
 require(hard_ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"hard search starts");
 auto stop_time=std::chrono::steady_clock::now();miner.stop();
 require(std::chrono::steady_clock::now()-stop_time<std::chrono::seconds(2)&&miner.error().empty(),"hard search cancellation survives startup race");
 miner.start([]()->std::optional<Block>{throw std::runtime_error("template failed");},[](const Block&){});
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 while(miner.is_mining()&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 miner.stop();require(miner.error()=="template failed","callback failure captured without terminating process");
 block.header.difficulty=1;std::promise<void> restarted;auto restart_ready=restarted.get_future();
 miner.start([&]()->std::optional<Block>{restarted.set_value();return std::nullopt;},[](const Block&){});
 require(restart_ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"restart after failure");miner.stop();require(miner.error().empty(),"restart clears previous failure");
 ProofOfWork pow;std::stop_source cancelled;cancelled.request_stop();
 require(!pow.mine(block.header,std::chrono::seconds(1),0,cancelled.get_token()),"pre-cancelled search never submits proof");
 std::cout<<"Background mining, proof, restart, failure and cancellation passed\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
