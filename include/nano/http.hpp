#pragma once
#include "rpc.hpp"
#include "beacon_api.hpp"
#include <memory>
namespace nano {
class HttpServer {
 struct Impl;
 std::unique_ptr<Impl> impl_;
public:
 HttpServer(Rpc& rpc,const std::string& host="127.0.0.1",uint16_t port=8545,std::chrono::milliseconds maximum_queue_wait=std::chrono::seconds(4),BeaconReadHandler beacon={},BeaconEventsHandler events={});
 ~HttpServer();
 uint16_t port()const;
 void run();
 void stop();
};
}
