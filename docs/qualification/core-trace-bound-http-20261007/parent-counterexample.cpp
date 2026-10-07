#include <functional>
#include <iostream>
// Exact production value-capture/copy behavior; no RPC, clock or network involved.
int main(){
 unsigned emitted=0;
 std::function<void()> observer=[count=unsigned(0),&emitted]()mutable{if(count++>=64)return;++emitted;};
 for(unsigned connection=0;connection<1000;++connection){auto callback=observer;callback();}
 std::cout<<"{\"connections\":1000,\"diagnostics\":"<<emitted<<",\"intendedMaximum\":64,\"networkQualified\":false}\n";
 return emitted<=64?0:1;
}
