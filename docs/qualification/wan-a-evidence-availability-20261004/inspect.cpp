#include "nano/storage.hpp"
#include <iostream>
using namespace nano;
int main(int argc,char** argv){try{if(argc!=4)throw std::invalid_argument("db chain genesis");ChainStore disk(argv[1],ChainStore::OpenMode::ReadOnly);const auto record=disk.fork_checkpoint(Int(argv[2]),unhex(argv[3]),128*1024*1024);const auto tip=disk.latest();if(!record||!tip)throw std::runtime_error("missing forensic checkpoint/tip");std::cout<<Json{{"expectedCheckpoint",Json{{"sequence",quantity(Int(std::to_string(record->token.sequence)))},{"digest",hex(record->token.digest)}}},{"tip",tip->header.to_json()},{"pinProvenance","read from evidence database, not independently retained before failure"}}.dump()<<std::endl;}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
