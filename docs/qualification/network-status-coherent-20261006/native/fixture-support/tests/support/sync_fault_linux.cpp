// Test-only ELF interposition. Never link this library into Nano node binaries.
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
namespace {
char target_path[PATH_MAX]{};
std::atomic<bool> armed{false};
std::atomic<unsigned> failures{0};
bool should_fail(int fd){
 if(!armed.load(std::memory_order_acquire))return false;
 char descriptor[64],path[PATH_MAX]{},resolved[PATH_MAX]{};
 const auto width=std::snprintf(descriptor,sizeof(descriptor),"/proc/self/fd/%d",fd);
 if(width<0||size_t(width)>=sizeof(descriptor))return false;
 const auto length=readlink(descriptor,path,sizeof(path)-1);
 if(length<0||size_t(length)>=sizeof(path)-1)return false;
 path[length]=0;
 if(!realpath(path,resolved)||std::strcmp(resolved,target_path)!=0)return false;
 ++failures;errno=EIO;return true;
}
using Sync=int(*)(int);
Sync next(const char* symbol){auto fn=reinterpret_cast<Sync>(dlsym(RTLD_NEXT,symbol));if(!fn)std::abort();return fn;}
}
extern "C" void nano_test_arm_sync_fault(const char* path){
 if(!realpath(path,target_path))std::abort();failures=0;armed.store(true,std::memory_order_release);
}
extern "C" void nano_test_arm_future_sync_fault(const char* path){
 if(!path||path[0]!='/'||std::strlen(path)>=sizeof(target_path))std::abort();
 std::strcpy(target_path,path);failures=0;armed.store(true,std::memory_order_release);
}
extern "C" void nano_test_disarm_sync_fault(){armed.store(false,std::memory_order_release);}
extern "C" unsigned nano_test_sync_failures(){return failures.load();}
extern "C" int fsync(int fd){if(should_fail(fd))return -1;static const auto original=next("fsync");return original(fd);}
extern "C" int fdatasync(int fd){if(should_fail(fd))return -1;static const auto original=next("fdatasync");return original(fd);}
