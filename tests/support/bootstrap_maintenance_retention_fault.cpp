// Linux-only TEST preload. Never linked/installed with a production program.
// Existing EIO modes remain separate from the bounded observed-WAL protocol.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <mutex>
#include <signal.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>
namespace {
std::atomic<bool> armed{false};
using Sync=int(*)(int);
Sync backend(){static const auto fn=reinterpret_cast<Sync>(::dlsym(RTLD_NEXT,"fsync"));return fn;}
thread_local bool in_hook=false;
struct Hook{bool old;Hook():old(in_hook){in_hook=true;}~Hook(){in_hook=old;}};
// This fixed binary pipe layout is duplicated verbatim by the public harness.
// No write buffer, token, snapshot or request bytes enter a packet.
struct WalStat{uint64_t dev,ino,size,uid,gid,mode,nlink;};
struct WalPacket{uint32_t magic,case_id,index,pid,stage,call,role;int32_t fd;int64_t position,result;int32_t error;uint32_t reserved;WalStat before,after;char leaf[256];};
struct WalControl{uint32_t magic,index,command,reserved;uint64_t dev,ino;};
static_assert(sizeof(WalPacket)<=512&&sizeof(WalControl)<=512);
constexpr uint32_t event_magic=0x4e574531,control_magic=0x4e574331;
std::mutex serial;uint32_t event_index=0;bool primary_armed=false,retention_done=false,primary_file=false,witness_file=false;
[[noreturn]] void refused(){::_exit(123);}
bool wal_mode(){return ::getenv("NANO_TEST_WAL_MODE")!=nullptr;}
int envnum(const char* name){const char* p=::getenv(name);if(!p||!*p)refused();char* end=nullptr;errno=0;long n=::strtol(p,&end,10);if(errno||!end||*end||n<1||n>INT_MAX)refused();return int(n);}
int envfd(const char* name){const auto n=envnum(name);if(n<3)refused();return n;}
WalStat stamp(const struct stat& s){if(s.st_size<0)refused();return {uint64_t(s.st_dev),uint64_t(s.st_ino),uint64_t(s.st_size),uint64_t(s.st_uid),uint64_t(s.st_gid),uint64_t(s.st_mode),uint64_t(s.st_nlink)};}
bool same_inode(const WalStat& a,const WalStat& b){return a.dev==b.dev&&a.ino==b.ino&&a.uid==b.uid&&a.gid==b.gid&&a.mode==b.mode&&a.nlink==b.nlink;}
struct Observed{uint32_t role=0;WalStat stat{};char leaf[256]{};};
Observed observed(int fd){
 struct stat s{},linked{};if(::fstat(fd,&s)!=0)return {};
 char descriptor[64]{},name[4097]{};int k=std::snprintf(descriptor,sizeof(descriptor),"/proc/self/fd/%d",fd);if(k<=0||size_t(k)>=sizeof(descriptor))return {};
 const auto n=::readlink(descriptor,name,sizeof(name)-1);if(n<=0||size_t(n)>=sizeof(name)-1)return {};name[n]=0;
 Observed out;
 const char* roots[]={"NANO_TEST_WAL_PRIMARY","NANO_TEST_WAL_WITNESS","NANO_TEST_WAL_OUTPUT"};
 for(unsigned i=0;i<3;++i){const char* root=::getenv(roots[i]);if(!root||root[0]!='/')refused();const size_t len=::strlen(root);if(!::strncmp(name,root,len)&&((name[len]=='/'&&name[len+1])||(i==2&&name[len]==0))){
   out.role=i+1;const char* leaf=name[len]=='/'?name+len+1:"";if(::strlen(leaf)>=sizeof(out.leaf)||::strstr(leaf,"..")||::strstr(name," (deleted)"))refused();std::strcpy(out.leaf,leaf);break;}}
 if(!out.role)return out;
 if(S_ISDIR(s.st_mode)&&(out.role!=3||out.leaf[0]))return {}; // only the exact OUTPUT root is a retention directory event
 if(::lstat(name,&linked)!=0||!same_inode(stamp(s),stamp(linked))||s.st_size!=linked.st_size||s.st_uid!=::geteuid()||s.st_gid!=::getegid())refused();
 if(!S_ISREG(s.st_mode)&&!(out.role==3&&S_ISDIR(s.st_mode)&&!out.leaf[0]))refused();
 if(S_ISREG(s.st_mode)&&s.st_nlink!=1)refused();out.stat=stamp(s);return out;
}
WalPacket event(uint32_t stage,uint32_t call,int fd,const Observed& before,const Observed& after,int64_t pos,int64_t result,int err){
 if(++event_index>64)refused();WalPacket p{};p.magic=event_magic;p.case_id=uint32_t(envnum("NANO_TEST_WAL_CASE"));if(p.case_id>6)refused();p.index=event_index;p.pid=uint32_t(::getpid());p.stage=stage;p.call=call;p.role=before.role;p.fd=fd;p.position=pos;p.result=result;p.error=err;p.before=before.stat;p.after=after.stat;std::strcpy(p.leaf,before.leaf);
 const int out=envfd("NANO_TEST_WAL_EVENT_FD");const long bound=::fpathconf(out,_PC_PIPE_BUF);if(bound<0||sizeof(p)>size_t(bound))refused();
 ssize_t n;do{n=::syscall(SYS_write,out,&p,sizeof(p));}while(n<0&&errno==EINTR);if(n!=ssize_t(sizeof(p)))refused();return p;
}
WalControl stopped(const WalPacket& p){
 if(::kill(::getpid(),SIGSTOP)!=0)refused();WalControl c{};size_t at=0;const int fd=envfd("NANO_TEST_WAL_CONTROL_FD");while(at<sizeof(c)){const auto n=::read(fd,reinterpret_cast<char*>(&c)+at,sizeof(c)-at);if(n<0&&errno==EINTR)continue;if(n<=0)refused();at+=size_t(n);}
 if(c.magic!=control_magic||c.index!=p.index||c.reserved)refused();return c;
}
int legacy_fsync(int fd,Sync real){
 const char* target=::getenv("NANO_TEST_RETENTION_FSYNC_TARGET");if(!target)return real(fd);
 const char* after=::getenv("NANO_TEST_RETENTION_ARM_AFTER");
 char descriptor[64]{};const int len=std::snprintf(descriptor,sizeof(descriptor),"/proc/self/fd/%d",fd);if(len<=0||size_t(len)>=sizeof(descriptor))return real(fd);
 char name[4097]{};const auto n=::readlink(descriptor,name,sizeof(name)-1);if(n<0)return real(fd);name[n]='\0';
 if(::strcmp(name,target)==0&&(!after||armed.load(std::memory_order_acquire))){constexpr char e[]="nano-test-retention-fsync-failed\n";const auto ignored=::syscall(SYS_write,STDERR_FILENO,e,sizeof(e)-1);(void)ignored;errno=EIO;return -1;}
 const int result=real(fd);if(result==0&&after&&::strcmp(name,after)==0)armed.store(true,std::memory_order_release);return result;
}
int sync_call(int fd,uint32_t call,Sync real){
 if(!wal_mode())return call==7?legacy_fsync(fd,real):real(fd);
 if(::getenv("NANO_TEST_RETENTION_FSYNC_TARGET")||::getenv("NANO_TEST_RETENTION_ARM_AFTER"))refused();
 std::lock_guard lock(serial);const auto b=observed(fd);
 if(!b.role)return real(fd);
 if(primary_armed&&b.role==1){const auto e=event(5,call,fd,b,b,-1,-1,0);const auto c=stopped(e);
  if(c.command!=3&&c.command!=4)refused();if(c.command==4&&(c.dev!=b.stat.dev||c.ino!=b.stat.ino))refused();
  const auto again=observed(fd);if(again.role!=1||std::strcmp(again.leaf,b.leaf)||!same_inode(b.stat,again.stat)||again.stat.size!=b.stat.size)refused();
  const int result=real(fd),error=errno;const auto a=observed(fd);if(!same_inode(b.stat,a.stat))refused();
  if(c.command==4){if(result!=0)refused();const auto done=event(6,call,fd,b,a,-1,result,error);(void)stopped(done);refused();}
  errno=error;return result;
 }
 const int result=real(fd),error=errno;const auto a=observed(fd);if(a.role!=b.role||std::strcmp(a.leaf,b.leaf)||!same_inode(b.stat,a.stat))refused();
 if(result==0&&b.role==3&&!retention_done){
  if(!std::strcmp(b.leaf,"primary-next.json")&&S_ISREG(b.stat.mode)&&(b.stat.mode&0777)==0600)primary_file=true;
  if(!std::strcmp(b.leaf,"witness-next.json")&&S_ISREG(b.stat.mode)&&(b.stat.mode&0777)==0600)witness_file=true;
  if(!b.leaf[0]&&S_ISDIR(b.stat.mode)&&primary_file&&witness_file){const auto e=event(1,call,fd,b,a,-1,result,error);const auto c=stopped(e);if(c.command!=1&&c.command!=5)refused();retention_done=true;if(c.command==5)primary_armed=false;}
 }
 if(result==0&&retention_done&&!primary_armed&&b.role==2&&std::strcmp(::getenv("NANO_TEST_WAL_MODE"),"retention-only")){
  const auto e=event(2,call,fd,b,a,-1,result,error);const auto c=stopped(e);if(c.command==2)primary_armed=true;else if(c.command!=1)refused();
 }
 errno=error;return result;
}
template<class Call>ssize_t append_call(int fd,uint32_t call,int64_t position,Call real){
 if(!wal_mode())return real();std::lock_guard lock(serial);const auto b=observed(fd);
 if(!primary_armed||b.role!=1)return real();
 (void)event(3,call,fd,b,b,position,-1,0);const auto result=real();const int error=errno;const auto a=observed(fd);
 if(a.role!=1||std::strcmp(a.leaf,b.leaf)||!same_inode(b.stat,a.stat))refused();(void)event(4,call,fd,b,a,position,result,error);errno=error;return result;
}
template<class F>F next(const char* name){auto fn=reinterpret_cast<F>(::dlsym(RTLD_NEXT,name));if(!fn){errno=ENOSYS;return nullptr;}return fn;}
}
extern "C" int fsync(int fd){if(in_hook)return int(::syscall(SYS_fsync,fd));Hook hook;const auto real=backend();if(!real){errno=ENOSYS;return -1;}return sync_call(fd,7,real);}
extern "C" int fdatasync(int fd){if(in_hook)return int(::syscall(SYS_fdatasync,fd));Hook hook;const auto real=next<Sync>("fdatasync");if(!real)return -1;return sync_call(fd,8,real);}
extern "C" ssize_t write(int fd,const void* p,size_t n){if(in_hook)return ::syscall(SYS_write,fd,p,n);Hook hook;const auto f=next<ssize_t(*)(int,const void*,size_t)>("write");if(!f)return -1;const auto at=::lseek(fd,0,SEEK_CUR);return append_call(fd,1,int64_t(at),[&]{return f(fd,p,n);});}
extern "C" ssize_t writev(int fd,const iovec* p,int n){if(in_hook)return ::syscall(SYS_writev,fd,p,n);Hook hook;const auto f=next<ssize_t(*)(int,const iovec*,int)>("writev");if(!f)return -1;const auto at=::lseek(fd,0,SEEK_CUR);return append_call(fd,2,int64_t(at),[&]{return f(fd,p,n);});}
extern "C" ssize_t pwrite(int fd,const void* p,size_t n,off_t at){if(in_hook)return ::syscall(SYS_pwrite64,fd,p,n,at);Hook hook;const auto f=next<ssize_t(*)(int,const void*,size_t,off_t)>("pwrite");if(!f)return -1;return append_call(fd,3,int64_t(at),[&]{return f(fd,p,n,at);});}
extern "C" ssize_t pwritev(int fd,const iovec* p,int n,off_t at){if(in_hook)return ::syscall(SYS_pwritev,fd,p,n,static_cast<unsigned long>(at),static_cast<unsigned long>(uint64_t(at)>>32));Hook hook;const auto f=next<ssize_t(*)(int,const iovec*,int,off_t)>("pwritev");if(!f)return -1;return append_call(fd,4,int64_t(at),[&]{return f(fd,p,n,at);});}
extern "C" ssize_t pwrite64(int fd,const void* p,size_t n,off64_t at){if(in_hook)return ::syscall(SYS_pwrite64,fd,p,n,at);Hook hook;const auto f=next<ssize_t(*)(int,const void*,size_t,off64_t)>("pwrite64");if(!f)return -1;return append_call(fd,5,int64_t(at),[&]{return f(fd,p,n,at);});}
extern "C" ssize_t pwritev64(int fd,const iovec* p,int n,off64_t at){if(in_hook)return ::syscall(SYS_pwritev,fd,p,n,static_cast<unsigned long>(at),static_cast<unsigned long>(uint64_t(at)>>32));Hook hook;const auto f=next<ssize_t(*)(int,const iovec*,int,off64_t)>("pwritev64");if(!f)return -1;return append_call(fd,6,int64_t(at),[&]{return f(fd,p,n,at);});}
