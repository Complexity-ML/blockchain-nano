#include "nano/migration.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace nano;
static int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void save(const std::filesystem::path& path,const Json& value){std::ofstream file(path);file<<value.dump(2);if(!file)throw std::runtime_error("fixture write failed");}
std::map<std::string,std::string> contents(const std::filesystem::path& root){std::map<std::string,std::string> result;for(const auto& file:std::filesystem::recursive_directory_iterator(root))if(file.is_regular_file()){std::ifstream input(file.path());result[file.path().lexically_relative(root).string()]=std::string((std::istreambuf_iterator<char>(input)),{});}return result;}
int main(){try{
 auto root=std::filesystem::temp_directory_path()/("nano-migration-"+std::to_string(getpid()));auto source=root/"python",destination=root/"migrated";
 std::filesystem::create_directories(source/"blocks");std::filesystem::create_directories(source/"transactions");
 auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));auto receiver="0x00000000000000000000000000000000000000ab";
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}}}}});
 save(source/"blocks/0.json",genesis.block.to_json(true));Block first,second;SignedTransaction transaction;
 Json metadata={{"chain_id",1337},{"latest_block",2},{"genesis_hash",hex(genesis.block.header.hash())}};
 {Blockchain chain(root/"producer",genesis);Transaction tx;tx.to=receiver;tx.value=123;tx.gas_price=2000000000;transaction=SignedTransaction::sign(tx,key);chain.submit(transaction);first=chain.mine(sender,24);second=chain.mine(sender,48);metadata["total_difficulty"]=quantity(chain.store().total_difficulty());}
 save(source/"meta.json",metadata);save(source/"blocks/1.json",first.to_json(false));save(source/"blocks/2.json",second.to_json(true));save(source/"transactions"/(hex(transaction.hash())+".json"),transaction.to_json());
 auto original=contents(source);auto summary=migrate_chaindb(genesis,source,destination);
 check(summary["blocks"]==3&&summary["blockHash"]==hex(second.header.hash()),"migration replays complete history including hash-only transactions");
 {Blockchain restored(destination,genesis);check(restored.state().get(receiver).balance==123&&restored.store().receipt(transaction.hash())->success,"migration rebuilds state and receipts");check(restored.store().state_at(0)->root()==genesis.state.root()&&restored.store().state_at(1)->root()==first.header.state_root,"migration retains historical state roots");}
 check(contents(source)==original,"migration never writes source files");
 auto fail=[&](const std::filesystem::path& target,const char* message){bool rejected=false;try{migrate_chaindb(genesis,source,target);}catch(const std::exception&){rejected=true;}check(rejected,message);};
 auto saved_destination=contents(destination);fail(destination,"existing destination cannot be replaced");check(contents(destination)==saved_destination,"existing database remains unchanged");
 auto failed=root/"failed";auto clean_failure=[&](const char* message){auto before=contents(source);fail(failed,message);check(!std::filesystem::exists(failed),"failed migration publishes no database");for(const auto& file:std::filesystem::directory_iterator(root))check(!file.path().filename().string().starts_with(".failed.import-"),"failed migration removes staging directory");check(contents(source)==before,"failed migration preserves source files");};
 auto corrupt=first.to_json(false);corrupt["stateRoot"]=hex(Bytes(32));save(source/"blocks/1.json",corrupt);clean_failure("corrupt block hash rejected");save(source/"blocks/1.json",first.to_json(false));
 corrupt.erase("hash");save(source/"blocks/1.json",corrupt);clean_failure("valid header with incorrect replayed state root rejected");save(source/"blocks/1.json",first.to_json(false));
 auto wrong_transaction=transaction.to_json();wrong_transaction["value"]="0x456";save(source/"transactions"/(hex(transaction.hash())+".json"),wrong_transaction);clean_failure("transaction file hash mismatch rejected");save(source/"transactions"/(hex(transaction.hash())+".json"),transaction.to_json());
 std::filesystem::remove(source/"blocks/2.json");clean_failure("missing historical block rejected");save(source/"blocks/2.json",second.to_json(true));
 auto bad_metadata=metadata;bad_metadata["total_difficulty"]="0x999";save(source/"meta.json",bad_metadata);clean_failure("incorrect total difficulty rejected");save(source/"meta.json",metadata);
 fail(source/"nested","destination inside source rejected");check(!std::filesystem::exists(source/"nested"),"source remains untouched after nested destination rejection");
 std::filesystem::create_directory(failed);fail(failed,"existing empty directory is preserved");check(std::filesystem::is_empty(failed),"empty destination not reused");std::filesystem::remove(failed);
 auto link=root/"source-link";std::filesystem::create_directory_symlink(source,link);fail(link/"nested","source symlink cannot bypass destination isolation");
 auto resumed=migrate_chaindb(genesis,source,failed);check(resumed["blockHash"]==summary["blockHash"],"corrected source imports successfully after previous failures");
 std::filesystem::remove_all(root);std::cout<<checks<<" migration checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"After "<<checks<<" checks: "<<e.what()<<"\n";return 1;}}
