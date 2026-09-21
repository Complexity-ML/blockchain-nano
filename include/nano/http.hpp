#pragma once
#include "rpc.hpp"
#include <memory>
namespace nano {
class HttpServer {
 struct Impl;
 std::unique_ptr<Impl> impl_;
public:
 HttpServer(Rpc& rpc,const std::string& host="127.0.0.1",uint16_t port=8545);
 ~HttpServer();
 uint16_t port()const;
 void run();
 void stop();
};
}
