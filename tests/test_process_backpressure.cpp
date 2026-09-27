#include "support/process.hpp"

// A blocked trace writer cannot reach its progress marker. This reproduces the
// observer-induced stall independently of consensus or network timing.
int main(int argc,char** argv){try{
 using namespace test;
 constexpr size_t payload_size=1024*1024;
 if(argc==4&&std::string(argv[1])=="--writer"){
  const fs::path directory=argv[2];const bool stderr_writer=std::string(argv[3])=="stderr";
  const int fd=stderr_writer?STDERR_FILENO:STDOUT_FILENO;
  const int flags=fcntl(fd,F_GETFL);
  check(flags>=0&&fcntl(fd,F_SETFL,flags|O_NONBLOCK)==0,"set nonblocking trace writer");
  const std::string payload(payload_size,stderr_writer?'E':'O');
  size_t offset=0;
  while(offset<payload.size()){
   const auto n=::write(fd,payload.data()+offset,payload.size()-offset);
   if(n<0&&errno==EINTR)continue;
   if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){
    // Prove the pipe is actually full before asking the parent to drain it.
    save(directory/"blocked.json",true);
    check(fcntl(fd,F_SETFL,flags)==0,"restore blocking trace writer");
    continue;
   }
   check(n>0,"trace write failed");offset+=size_t(n);
  }
  save(directory/"progress.json",true);return 0;
 }
 Temp first,second;
 Process a({argv[0],"--writer",first.path.string(),"stdout"});
 Process b({argv[0],"--writer",second.path.string(),"stderr"});
 until([&]{return fs::exists(first/"blocked.json")&&fs::exists(second/"blocked.json");},"both trace pipes reached EAGAIN",5);
 check(a.running()&&b.running(),"negative control writers unexpectedly exited");
 check(!fs::exists(first/"progress.json")&&!fs::exists(second/"progress.json"),"undrained trace did not block progress");
 until([&]{
  a.pump(0);b.pump(0);
  return fs::exists(first/"progress.json")&&fs::exists(second/"progress.json");
 },"unconditional drain restores both writers",5);
 const auto ar=a.wait(5),br=b.wait(5);
 check(ar.code==0&&br.code==0,"writers failed");
 check(ar.out==std::string(payload_size,'O')&&ar.err.empty(),"stdout trace was lost");
 check(br.err==std::string(payload_size,'E')&&br.out.empty(),"stderr trace was lost");
 std::cout<<"process backpressure: negative control and complete trace recovery passed\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
