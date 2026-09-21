#include "nano/wallet.hpp"
#include "nano/block_auth.hpp"
#include "nano/hdwallet.hpp"
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <iostream>
namespace nano {
Wallet::Wallet(Bytes key):key_(std::move(key)){nano::public_key(key_);}
void Wallet::sign_block(Block& block,const Int& chain_id,const Bytes& genesis_hash)const{nano::sign_block(block,key_,chain_id,genesis_hash);}
Wallet Wallet::create(){return Wallet(generate_private_key());}
Wallet Wallet::from_keyfile(const std::filesystem::path& path,const std::string& password){std::ifstream in(path);if(!in)throw std::runtime_error("cannot open wallet");Json data;in>>data;return Wallet(decrypt_key(data,password));}
std::string Wallet::address()const{return public_address(public_key());}
Bytes Wallet::public_key()const{return nano::public_key(key_);}
Json Wallet::keyfile(const std::string& password)const{return encrypt_key(key_,password);}
SignedTransaction Wallet::sign_transaction(const Transaction& tx)const{return SignedTransaction::sign(tx,key_);}
Bytes Wallet::sign_typed_data(const Json& data)const{return nano::sign_typed_data(key_,data);}
Bytes Wallet::sign_message(const Bytes& message)const{return nano::sign_message(key_,message);}
static void write_private_file(const std::filesystem::path& path,const std::string& data){
 int fd=::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
 if(fd<0)throw std::runtime_error("cannot create wallet file (destination must not exist)");
 try{
  size_t offset=0;
  while(offset<data.size()){auto n=::write(fd,data.data()+offset,data.size()-offset);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("wallet write failed");offset+=size_t(n);}
  if(::fsync(fd)!=0)throw std::runtime_error("wallet synchronization failed");
 }catch(...){::close(fd);::unlink(path.c_str());throw;}
 if(::close(fd)!=0)throw std::runtime_error("wallet close failed");
}
void Wallet::save_keyfile(const std::filesystem::path& path,const std::string& password)const{write_private_file(path,keyfile(password).dump(2)+"\n");}
std::string KeyStore::add_wallet(Wallet wallet,const std::string& name){
 auto addr=wallet.address();
 if(!name.empty()){
  if(name.starts_with("0x"))throw std::invalid_argument("wallet name must not resemble an address");
  auto found=names_.find(name);if(found!=names_.end()&&found->second!=addr)throw std::invalid_argument("wallet name already used");
 }
 if(!wallets_.contains(addr))order_.push_back(addr);
 wallets_[addr]=std::make_shared<Wallet>(std::move(wallet));if(!name.empty())names_[name]=addr;if(default_.empty())default_=addr;return addr;
}
Wallet KeyStore::create_wallet(const std::string& name){auto wallet=Wallet::create();add_wallet(wallet,name);return wallet;}
std::shared_ptr<const Wallet> KeyStore::get_wallet(const std::string& input)const{
 auto name=names_.find(input);std::string key=name==names_.end()?input:name->second;
 if(key.starts_with("0x")){try{key=checksum_address(key);}catch(...){return {};}}
 auto it=wallets_.find(key);return it==wallets_.end()?nullptr:it->second;
}
std::shared_ptr<const Wallet> KeyStore::get_default()const{return get_wallet(default_);}
bool KeyStore::set_default(const std::string& input){auto wallet=get_wallet(input);if(!wallet)return false;default_=wallet->address();return true;}
std::vector<std::string> KeyStore::addresses()const{return order_;}
bool KeyStore::remove_wallet(const std::string& input){auto wallet=get_wallet(input);if(!wallet)return false;auto addr=wallet->address();wallets_.erase(addr);std::erase(order_,addr);for(auto it=names_.begin();it!=names_.end();)if(it->second==addr)it=names_.erase(it);else ++it;if(default_==addr)default_=order_.empty()?"":order_.front();return true;}
int wallet_command(int argc,char** argv){
 if(argc<4)throw std::invalid_argument("wallet <create|import|address|sign-message|sign-transaction|sign-typed-data> <keyfile> [data]");
 std::string action=argv[2];
 if(action=="create-many"){
  if(argc>5)throw std::invalid_argument("create-many <new-directory> [count]");
  auto count=argc==5?u64(quantity(Json(argv[4]))):10;
  if(count<1||count>100)throw std::invalid_argument("account count must be 1 to 100");
  const char* password=std::getenv("NANO_WALLET_PASSWORD");
  if(!password||!*password)throw std::invalid_argument("NANO_WALLET_PASSWORD is required");
  std::filesystem::path directory=argv[3];
  if(::mkdir(directory.c_str(),0700)!=0)throw std::runtime_error("cannot create account directory (destination must not exist)");
  std::vector<std::filesystem::path> written;Json accounts=Json::array();
  try{
   for(uint64_t i=0;i<count;++i){
    auto wallet=Wallet::create();auto path=directory/(std::to_string(i)+".json");
    wallet.save_keyfile(path,password);written.push_back(path);
    accounts.push_back({{"index",i},{"address",wallet.address()},{"keyfile",path.string()}});
   }
  }catch(...){for(const auto& path:written)::unlink(path.c_str());::rmdir(directory.c_str());throw;}
  std::cout<<accounts.dump(2)<<"\n";return 0;
 }
 if(action=="deployment-address"){
  if(argc>5)throw std::invalid_argument("deployment-address <keyfile> [nonce]");
  const char* password=std::getenv("NANO_WALLET_PASSWORD");if(!password||!*password)throw std::invalid_argument("NANO_WALLET_PASSWORD is required");
  auto wallet=Wallet::from_keyfile(argv[3],password);Int nonce=argc==5?quantity(Json(argv[4])):Int(0);bytes(nonce,32);
  std::cout<<Json{{"from",wallet.address()},{"nonce",quantity(nonce)},{"contractAddress",create_address(wallet.address(),nonce)}}.dump()<<"\n";return 0;
 }
 if(action=="hd-create"||action=="hd-derive"){
  if(argc>5)throw std::invalid_argument("too many HD wallet arguments");
  const char* passphrase=std::getenv("NANO_HD_PASSPHRASE");std::string pass=passphrase?passphrase:"";
  if(action=="hd-create"){
   auto count=argc==5?u64(quantity(Json(argv[4]))):12;if(count>24)throw std::invalid_argument("invalid mnemonic word count");auto wallet=HDWallet::create(unsigned(count),pass);
   write_private_file(argv[3],Json{{"mnemonic",wallet.mnemonic()}}.dump(2)+"\n");
   std::cout<<Json{{"mnemonicFile",argv[3]},{"address",wallet.derive_account().address()}}.dump()<<"\n";return 0;
  }
  const char* mnemonic=std::getenv("NANO_MNEMONIC");const char* password=std::getenv("NANO_WALLET_PASSWORD");
  if(!mnemonic||!password||!*password)throw std::invalid_argument("NANO_MNEMONIC and NANO_WALLET_PASSWORD are required");
  auto index=argc==5?u64(quantity(Json(argv[4]))):0;if(index>=0x80000000ULL)throw std::invalid_argument("invalid HD account index");
  auto wallet=HDWallet(mnemonic,pass).derive_account(uint32_t(index));wallet.save_keyfile(argv[3],password);
  std::cout<<Json{{"address",wallet.address()},{"keyfile",argv[3]},{"index",index}}.dump()<<"\n";return 0;
 }
 bool sign=action=="sign-message"||action=="sign-transaction"||action=="sign-typed-data";
 if((sign&&argc!=5)||(!sign&&argc!=4)||(action!="create"&&action!="import"&&action!="address"&&!sign))throw std::invalid_argument("invalid wallet arguments");
 const char* password=std::getenv("NANO_WALLET_PASSWORD");if(!password||!*password)throw std::invalid_argument("NANO_WALLET_PASSWORD is required");
 if(action=="create"||action=="import"){
  auto wallet=[&](){if(action=="create")return Wallet::create();const char* key=std::getenv("NANO_PRIVATE_KEY");if(!key)throw std::invalid_argument("NANO_PRIVATE_KEY is required for import");return Wallet(unhex(key));}();
  wallet.save_keyfile(argv[3],password);std::cout<<Json{{"address",wallet.address()},{"keyfile",argv[3]}}.dump()<<"\n";return 0;
 }
 auto wallet=Wallet::from_keyfile(argv[3],password);
 if(action=="address")std::cout<<Json{{"address",wallet.address()},{"publicKey",hex(wallet.public_key())}}.dump()<<"\n";
 else if(action=="sign-message")std::cout<<Json{{"address",wallet.address()},{"signature",hex(wallet.sign_message(unhex(argv[4])))}}.dump()<<"\n";
 else{
  std::ifstream in(argv[4]);if(!in)throw std::runtime_error("cannot open transaction file");Json data;in>>data;
  if(action=="sign-typed-data"){std::cout<<Json{{"address",wallet.address()},{"hash",hex(typed_data_hash(data))},{"signature",hex(wallet.sign_typed_data(data))}}.dump()<<"\n";return 0;}
  if(data.contains("from")&&nano::address(data["from"].get<std::string>())!=nano::address(wallet.address()))throw std::invalid_argument("transaction sender differs from wallet");
  auto tx=wallet.sign_transaction(Transaction::from_json(data));std::cout<<Json{{"address",wallet.address()},{"raw",hex(tx.raw())},{"hash",hex(tx.hash())}}.dump()<<"\n";
 }
 return 0;
}
}
