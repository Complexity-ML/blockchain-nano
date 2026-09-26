#include <rocksdb/db.h>
#include <iostream>
#include <memory>
int main(int argc,char** argv){if(argc!=2)return 2;std::unique_ptr<rocksdb::DB> db;rocksdb::Options options;auto s=rocksdb::DB::OpenForReadOnly(options,argv[1],&db);if(!s.ok()){std::cerr<<s.ToString();return 1;}std::string value;s=db->Get(rocksdb::ReadOptions(),"fork-checkpoint/v1/payload",&value);if(!s.ok()){std::cerr<<s.ToString();return 1;}std::cout<<value;}
