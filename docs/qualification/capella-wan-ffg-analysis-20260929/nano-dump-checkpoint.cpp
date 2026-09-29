#include "nano/storage.hpp"
#include "nano/chain.hpp"
#include <fstream>
#include <iostream>
using namespace nano;
int main(int argc,char**argv){try{if(argc!=3)return 2;std::ifstream f(argv[1]);Json j;f>>j;auto g=Genesis::from_json(j);ChainStore db(argv[2]);auto r=db.fork_checkpoint(g.chain_id,g.block.header.hash(),128*1024*1024);if(!r)return 3;std::cerr<<Json{{"sequence",r->token.sequence},{"digest",hex(r->token.digest)}}.dump()<<"\n";std::cout<<r->snapshot;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
