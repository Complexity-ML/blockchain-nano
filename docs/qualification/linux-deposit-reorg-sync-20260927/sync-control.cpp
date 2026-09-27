#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" void nano_test_disarm_sync_fault();
extern "C" unsigned nano_test_sync_failures();
int main(){const char* target="/tmp/nano-sync-target",*other="/tmp/nano-sync-other";int a=open(target,O_CREAT|O_RDWR,0600),b=open(other,O_CREAT|O_RDWR,0600);if(a<0||b<0)return 1;write(a,"a",1);write(b,"b",1);if(fsync(a)||fdatasync(b))return 2;nano_test_arm_sync_fault(target);if(fsync(a)!=-1||errno!=EIO)return 3;if(fdatasync(a)!=-1||errno!=EIO)return 4;if(fsync(b)||fdatasync(b)||nano_test_sync_failures()!=2)return 5;nano_test_disarm_sync_fault();if(fsync(a)||fdatasync(a))return 6;close(a);close(b);unlink(target);unlink(other);puts("target fsync/fdatasync refused; unrelated file and disarmed writes passed");}
