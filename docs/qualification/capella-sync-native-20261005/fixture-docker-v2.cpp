#include "/work/tests/support/rocksdb_fixture.hpp"
#include <filesystem>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 2;rocksdb::Options options;options.create_if_missing=false;
 bool refused=false;try{auto missing=test::open_owned_fixture(options,argv[1]);}catch(const std::runtime_error&){refused=true;}
 if(!refused||std::filesystem::exists(std::filesystem::path(argv[1])/"CURRENT"))return 3;
 options.create_if_missing=true;auto db=test::open_owned_fixture(options,argv[1]);rocksdb::WriteOptions write;write.sync=true;
 if(!db->Put(write,"meta/schema","nano/1").ok())return 4;db.reset();
 options.create_if_missing=false;db=test::open_owned_fixture(options,argv[1]);std::string schema;
 if(!db->Get(rocksdb::ReadOptions{},"meta/schema",&schema).ok()||schema!="nano/1")return 5;
 std::cout<<"owned fixture: missing refused, created, synced, closed, reopened\n";
}
