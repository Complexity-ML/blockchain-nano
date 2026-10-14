// Linux test-only interposition. Exact caller-owned path selectors, one event,
// no WAL name/record offset, signing/provider hook or production linkage.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <atomic>
#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
namespace {
std::atomic<bool> fired=false;
std::string fd_path(int fd){char name[64],out[4096];std::snprintf(name,sizeof(name),"/proc/self/fd/%d",fd);auto n=::readlink(name,out,sizeof(out));return n>0&&size_t(n)<sizeof(out)?std::string(out,size_t(n)):std::string{};}
const char* mode(){auto p=::getenv("NANO_TEST_JOURNAL_IO_MODE");return p?p:"";}
std::string target(){auto p=::getenv("NANO_TEST_JOURNAL_IO_TARGET");return p?p:"";}
void event(char c){auto p=::getenv("NANO_TEST_JOURNAL_IO_EVENT");if(!p)return;char* end=nullptr;long fd=::strtol(p,&end,10);if(end&&!*end&&fd>=3&&fd<=4096)(void)::write(int(fd),&c,1);}
bool matches(const std::string& path,const std::string& expected){return !expected.empty()&&path==expected;}
std::string trigger(){auto p=::getenv("NANO_TEST_JOURNAL_IO_TRIGGER");return p?p:"";}
void read_fault(int fd){auto m=std::string(mode());if(m!="delay"&&m!="stop"&&m!="rename-parent")return;auto path=fd_path(fd);if((m=="rename-parent"?!matches(path,trigger()):!matches(path,target()))||fired.exchange(true))return;
 if(m=="delay"){event('D');::usleep(30000);return;}if(m=="stop"){event('S');::raise(SIGSTOP);return;}
 auto p=target(),held=p+".test-held";struct stat s{};if(p.empty()||::lstat(p.c_str(),&s)!=0||!S_ISDIR(s.st_mode)||s.st_uid!=::geteuid()||(s.st_mode&0777)!=0700||::rename(p.c_str(),held.c_str())!=0)return;if(::mkdir(p.c_str(),0700)!=0)return;int out=::open((p+"/substituted-sentinel").c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(out>=0){const char text[]="test sentinel\n";(void)::write(out,text,sizeof(text)-1);::close(out);}event('R');
}
}
extern "C" ssize_t pread(int fd,void* b,size_t n,off_t o){static auto real=reinterpret_cast<ssize_t(*)(int,void*,size_t,off_t)>(::dlsym(RTLD_NEXT,"pread"));if(!real){errno=EIO;return -1;}read_fault(fd);return real(fd,b,n,o);}
extern "C" int fsync(int fd){static auto real=reinterpret_cast<int(*)(int)>(::dlsym(RTLD_NEXT,"fsync"));if(!real){errno=EIO;return -1;}auto m=std::string(mode()),p=fd_path(fd),t=target();struct stat st{};bool regular_private=::fstat(fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_uid==::geteuid()&&(st.st_mode&0777)==0600;auto parent=t.substr(0,t.find_last_of('/'));bool hit=(m=="file-fsync"&&regular_private&&matches(p.substr(0,p.find_last_of('/')),parent))||(m=="parent-fsync"&&matches(p,parent));if(hit&&!fired.exchange(true)){event(m=="file-fsync"?'F':'P');errno=EIO;return -1;}return real(fd);}
