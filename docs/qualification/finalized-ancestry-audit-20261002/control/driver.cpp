#include "nano/consensus_capella_audit.hpp"
#include "nano/chain.hpp"
#include <fstream>
#include <iostream>
using namespace nano;
Json load(const char* path){std::ifstream f(path);if(!f)throw std::runtime_error("input missing");Json j;f>>j;return j;}
int main(int argc,char** argv){try{if(argc!=4)throw std::runtime_error("usage: driver genesis stopped-db request");std::cout<<audit_capella_database(Genesis::from_json(load(argv[1])),argv[2],load(argv[3])).dump()<<'\n';return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
