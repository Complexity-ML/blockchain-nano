// Test-only dyld interposition. This library is not linked into Nano binaries.
#include <atomic>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
static char target_path[PATH_MAX]{};
static std::atomic<bool> armed{false};
static std::atomic<unsigned> failures{0};
extern "C" void nano_test_arm_sync_fault(const char* path){
 if(!realpath(path,target_path))abort();failures=0;armed.store(true,std::memory_order_release);
}
extern "C" void nano_test_disarm_sync_fault(){armed.store(false,std::memory_order_release);}
extern "C" unsigned nano_test_sync_failures(){return failures.load();}
static int failing_fsync(int fd){
 if(armed.load(std::memory_order_acquire)){
  char path[PATH_MAX]{},resolved[PATH_MAX]{};
  if(fcntl(fd,F_GETPATH,path)==0&&realpath(path,resolved)&&std::strcmp(resolved,target_path)==0){++failures;errno=EIO;return -1;}
 }
 return fsync(fd);
}
__attribute__((used)) static struct {const void* replacement;const void* original;} interposer
__attribute__((section("__DATA,__interpose")))={reinterpret_cast<const void*>(&failing_fsync),reinterpret_cast<const void*>(&fsync)};

#include <cstdarg>
static int failing_fcntl(int fd,int command,...){
 auto original=&fcntl;
 if(command==F_FULLFSYNC){
  if(armed.load(std::memory_order_acquire)){
   char path[PATH_MAX]{},resolved[PATH_MAX]{};
   if(original(fd,F_GETPATH,path)==0&&realpath(path,resolved)&&std::strcmp(resolved,target_path)==0){++failures;errno=EIO;return -1;}
  }
  return original(fd,command);
 }
 switch(command){case F_GETFD:case F_GETFL:case F_GETOWN:return original(fd,command);}
 va_list args;va_start(args,command);int result;
 switch(command){
  case F_DUPFD:case F_DUPFD_CLOEXEC:case F_SETFD:case F_SETFL:case F_SETOWN:case F_NOCACHE:case F_RDAHEAD:case F_SETNOSIGPIPE:{int arg=va_arg(args,int);result=original(fd,command,arg);break;}
  default:{void* arg=va_arg(args,void*);result=original(fd,command,arg);break;}
 }
 va_end(args);return result;
}
__attribute__((used)) static struct {const void* replacement;const void* original;} fcntl_interposer
__attribute__((section("__DATA,__interpose")))={reinterpret_cast<const void*>(&failing_fcntl),reinterpret_cast<const void*>(&fcntl)};
