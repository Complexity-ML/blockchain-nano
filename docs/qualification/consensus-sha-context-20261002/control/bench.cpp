#include "nano/consensus_signing.hpp"
#include <chrono>
#include <iostream>
int main(){nano::ConsensusRoot a{},b{},out{};unsigned check=0;constexpr unsigned count=1000000;
 for(unsigned warm=0;warm<1000;++warm)out=nano::consensus_signing_root(a,b);
 for(unsigned round=0;round<5;++round){auto start=std::chrono::steady_clock::now();
  for(unsigned n=0;n<count;++n){a[n%32]=static_cast<unsigned char>(n);b[(n+7)%32]=static_cast<unsigned char>(n>>8);out=nano::consensus_signing_root(a,b);check+=out[n%32];}
  std::cout<<round<<' '<<count<<' '<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<' '<<check<<'\n';
 }
}
