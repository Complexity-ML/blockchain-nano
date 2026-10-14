// Linux-only TEST shared library, never linked to or installed with the tool.
// It fails a real fsync of an exact synthetic path observed through its FD.
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstddef>
#include <dlfcn.h>
#include <unistd.h>
namespace {
std::atomic<bool> armed{false};
using Sync=int(*)(int);
Sync backend(){static const auto fn=reinterpret_cast<Sync>(::dlsym(RTLD_NEXT,"fsync"));return fn;}
}
extern "C" int fsync(int fd){
 const auto real=backend();if(!real){errno=ENOSYS;return -1;}
 const char* target=::getenv("NANO_TEST_RETENTION_FSYNC_TARGET");if(!target)return real(fd);
 const char* after=::getenv("NANO_TEST_RETENTION_ARM_AFTER");
 char descriptor[64]{};const int len=std::snprintf(descriptor,sizeof(descriptor),"/proc/self/fd/%d",fd);if(len<=0||size_t(len)>=sizeof(descriptor))return real(fd);
 char name[4097]{};const auto n=::readlink(descriptor,name,sizeof(name)-1);if(n<0)return real(fd);name[n]='\0';
 if(::strcmp(name,target)==0&&(!after||armed.load(std::memory_order_acquire))){constexpr char event[]="nano-test-retention-fsync-failed\n";const auto ignored=::write(STDERR_FILENO,event,sizeof(event)-1);(void)ignored;errno=EIO;return -1;}
 const int result=real(fd);if(result==0&&after&&::strcmp(name,after)==0)armed.store(true,std::memory_order_release);return result;
}
