#include "nano/http.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <iostream>
#include <unistd.h>
using namespace nano;
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;
int main(){try{
 const auto directory=std::filesystem::temp_directory_path()/("nano-beacon-http-errors-"+std::to_string(getpid()));
 {
 Blockchain chain(directory,Genesis::from_json(Json::object()));Rpc rpc(chain);
 BeaconReadHandler handler=[](std::string_view path,const std::function<void()>& check)->BeaconReadResponse{
  if(path=="/eth/deadline"){std::this_thread::sleep_for(std::chrono::milliseconds(2100));try{check();}catch(const ExecutionCancelled&){throw BeaconReadCancelled(BeaconReadPhase::state_commitment,2100);}}
  if(path=="/eth/phase")throw BeaconReadCancelled(BeaconReadPhase::state_commitment,2100);
  if(path=="/eth/unavailable")throw std::runtime_error("private failure detail");
  return {200,{{"data",{{"version","test"}}}}};
 };
 HttpServer server(rpc,"127.0.0.1",0,std::chrono::seconds(4),handler);
 std::jthread thread([&]{server.run();});
 struct Stop{HttpServer& server;~Stop(){server.stop();}} stop{server};
 auto request=[&](const char* path){asio::io_context context;beast::tcp_stream stream(context);stream.expires_after(std::chrono::seconds(5));stream.connect({asio::ip::make_address("127.0.0.1"),server.port()});http::request<http::empty_body> req(http::verb::get,path,11);req.set(http::field::host,"localhost");req.set(http::field::accept,"application/json");http::write(stream,req);beast::flat_buffer buffer;http::response<http::string_body> response;http::read(stream,buffer,response);return response;};
 const auto expired=request("/eth/deadline");
 if(expired.result_int()!=503||Json::parse(expired.body()).at("message")!="Beacon read cancelled or execution budget expired"||Json::parse(expired.body()).at("phase")!="state-commitment")throw std::runtime_error("Beacon execution deadline diagnostic lost");
 const auto phased=request("/eth/phase");const auto detail=Json::parse(phased.body());
 if(phased.result_int()!=503||detail.at("phase")!="state-commitment"||detail.at("elapsedMs")!=2100||phased.body().find("private")!=std::string::npos)throw std::runtime_error("Beacon phase diagnostic lost");
 // Exceed the shared queue's reservation count across completed failures: a
 // leaked reservation would eventually report capacity instead of this error.
 for(unsigned i=0;i<12;++i){const auto unavailable=request("/eth/unavailable");if(unavailable.result_int()!=503||Json::parse(unavailable.body()).at("message")!="Beacon state unavailable"||unavailable.body().find("private failure detail")!=std::string::npos)throw std::runtime_error("Beacon exception diagnostic leaked or capacity was not released");}
 const auto reused=request("/eth/ok");
 if(reused.result_int()!=200)throw std::runtime_error("Beacon failed reads leaked queue capacity");
 std::cout<<Json{{"scenario","beacon-http-error-diagnostics"},{"executionDeadlinePreserved",true},{"phaseDiagnosticQualified",true},{"privateExceptionRedacted",true},{"queueCapacityReused",true}}.dump()<<std::endl;
 }
 std::filesystem::remove_all(directory);return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
