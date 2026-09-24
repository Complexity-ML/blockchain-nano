#pragma once
#include "rpc.hpp"
#include "simulation_queue.hpp"
#include "websocket_output.hpp"
#include <boost/beast.hpp>
#include <memory>
namespace nano {
void start_websocket(boost::beast::tcp_stream stream,boost::beast::http::request<boost::beast::http::string_body> request,Rpc& rpc,std::shared_ptr<RpcConnections> connections,SimulationQueue& simulations);
}
