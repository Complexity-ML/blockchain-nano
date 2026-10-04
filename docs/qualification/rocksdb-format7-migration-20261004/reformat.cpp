#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <openssl/evp.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <stdexcept>
void ok(const rocksdb::Status&s){if(!s.ok())throw std::runtime_error(s.ToString());}
template<class D=rocksdb::DB>std::unique_ptr<D> open(const rocksdb::Options&o,const std::string&p,bool read){std::unique_ptr<D>d;if constexpr(requires{D::Open(o,p,&d);D::OpenForReadOnly(o,p,&d);}){ok(read?D::OpenForReadOnly(o,p,&d):D::Open(o,p,&d));}else{D*raw=nullptr;ok(read?D::OpenForReadOnly(o,p,&raw):D::Open(o,p,&raw));d.reset(raw);}return d;}
std::pair<size_t,std::string> digest(rocksdb::DB& db){auto*raw=EVP_MD_CTX_new();if(!raw)throw std::runtime_error("digest allocation");std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)>ctx(raw,EVP_MD_CTX_free);if(EVP_DigestInit_ex(raw,EVP_sha256(),nullptr)!=1)throw std::runtime_error("digest init");
 size_t count=0;std::unique_ptr<rocksdb::Iterator>it(db.NewIterator(rocksdb::ReadOptions()));
 auto add=[&](rocksdb::Slice s){if(s.size()>128*1024*1024)throw std::runtime_error("value bound");unsigned char length[8];for(size_t i=0;i<8;++i)length[i]=(uint64_t(s.size())>>(8*i))&255;if(EVP_DigestUpdate(raw,length,8)!=1||EVP_DigestUpdate(raw,s.data(),s.size())!=1)throw std::runtime_error("digest update");};
 for(it->SeekToFirst();it->Valid();it->Next()){if(++count>1000000)throw std::runtime_error("entry bound");add(it->key());add(it->value());}ok(it->status());unsigned char hash[32];unsigned n=0;if(EVP_DigestFinal_ex(raw,hash,&n)!=1||n!=32)throw std::runtime_error("digest finish");std::ostringstream text;for(auto c:hash)text<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c);return {count,text.str()};}
int main(int argc,char**argv){try{if(argc!=3)throw std::runtime_error("usage: reformat-copy|read database");const std::string mode=argv[1];if(mode!="read"&&mode!="reformat-copy")throw std::runtime_error("mode");rocksdb::Options o;o.create_if_missing=false;o.paranoid_checks=true;o.allow_fallocate=false;rocksdb::BlockBasedTableOptions table;table.format_version=6;o.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table));auto db=open(o,argv[2],mode=="read");auto before=digest(*db);
 if(mode=="reformat-copy"){ok(db->SyncWAL());rocksdb::FlushOptions flush;flush.wait=true;ok(db->Flush(flush));rocksdb::CompactRangeOptions compact;compact.bottommost_level_compaction=rocksdb::BottommostLevelCompaction::kForce;ok(db->CompactRange(compact,nullptr,nullptr));if(before!=digest(*db))throw std::runtime_error("logical contents changed");}
 std::cout<<before.first<<" "<<before.second<<"\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
