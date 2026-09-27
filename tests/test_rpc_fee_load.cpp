#include "nano/rpc.hpp"
#include "support/process.hpp"
#include <future>
using namespace nano;
int main(int argc,char** argv){try{
 using Clock=std::chrono::steady_clock;
 auto micros=[](auto start){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count();};
 test::check(argc==1||(argc==2&&std::string(argv[1])=="1428"),"usage: nano-rpc-fee-load-tests [1428]");
 test::Temp directory;constexpr unsigned blocks=20,per_sender=64;
 const unsigned per_block=argc==2?1428:128,accounts=(per_block+per_sender-1)/per_sender;
 std::vector<Bytes> keys;std::vector<unsigned> nonces(accounts,0);Json allocations=Json::object();
 for(unsigned i=0;i<accounts;++i){keys.push_back(bytes(Int(2+i),32));allocations[public_address(public_key(keys.back()))]={{"balance",quantity(Int(1)<<200)}};}
 const auto producer=public_address(public_key(keys.front()));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"gasLimit",quantity(Int(per_block*21000))},{"alloc",allocations}});
 Blockchain chain(directory/"chain",genesis);Rpc rpc(chain);
 const auto setup_start=Clock::now();
 for(unsigned height=0;height<blocks;++height){
  const auto base=next_base_fee(chain.tip_header());
  for(unsigned i=0;i<per_block;++i){
    const auto account=i/per_sender;
    Transaction tx;tx.to="0x0000000000000000000000000000000000000099";tx.nonce=nonces[account]++;tx.gas=21000;
    const Int reward=account==0?Int(1):Int(100+i-per_sender);
    if(i%2){tx.type=2;tx.priority_fee=reward+1000;tx.max_fee=base+reward;}else tx.gas_price=base+reward;
    test::check(chain.submit(SignedTransaction::sign(tx,keys[account])),"load fixture admission");
  }
  auto mined=chain.mine(producer,12*(height+1));
  test::check(mined.transactions.size()==per_block&&mined.header.gas_used==mined.header.gas_limit,"fixture must fill every configured block");
 }
 const auto setup_us=micros(setup_start);const auto original=chain.tip_header().hash();
 auto request=[&](const char* method){auto r=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array()}});test::check(!r.contains("error"),r.dump());return r.at("result");};
 auto quote=[&]{const auto actual=quantity(request("eth_maxPriorityFeePerGas"));test::check(actual==101,"loaded history effective percentile: expected 101, got "+actual.get_str());};
 Json cold=Json::array();
 for(unsigned i=0;i<3;++i){rpc.load_wallets(Json::object(),"127.0.0.1");auto start=Clock::now();quote();cold.push_back(micros(start));}
 auto cached_start=Clock::now();for(unsigned i=0;i<1000;++i)quote();auto cached_us=micros(cached_start);
 // Start all workers after invalidating the cache: they contend on one genuine
 // cold request followed by cached reads, sharing the production Rpc mutex.
 rpc.load_wallets(Json::object(),"127.0.0.1");std::promise<void> release;auto ready=release.get_future().share();
 std::vector<std::future<void>> workers;
 for(unsigned thread=0;thread<8;++thread)workers.push_back(std::async(std::launch::async,[&]{ready.wait();for(unsigned i=0;i<128;++i){quote();test::check(quantity(request("eth_blockNumber"))==blocks,"ordinary reads survive concurrent fee requests");}}));
 const auto concurrent_start=Clock::now();release.set_value();for(auto& worker:workers)worker.get();auto concurrent_us=micros(concurrent_start);
 test::check(chain.tip_header().hash()==original&&chain.pending_count()==0,"read load must not mutate chain or pool");
 std::cout<<Json{{"blocks",blocks},{"transactions_per_block",per_block},{"total_transactions",blocks*per_block},{"gas_limit",per_block*21000},{"setup_us",setup_us},{"cold_calculation_us",cold},{"cached_1000_requests_us",cached_us},{"concurrent_workers",8},{"concurrent_total_requests",2048},{"concurrent_cold_and_cached_us",concurrent_us},{"expected_tip_wei",101},{"head_unchanged",true},{"transport","in-process RPC; no HTTP/P2P"}}.dump(2)<<'\n';
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
