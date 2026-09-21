#include "nano/chain.hpp"
#include <iostream>
#include <unistd.h>
using namespace nano;
static int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void seal(Header& header){ProofOfWork pow;auto found=pow.mine(header,std::chrono::seconds(2));if(!found)throw std::runtime_error("fixture mining timed out");header.nonce=found->nonce;header.mix_hash=found->mix_hash;}
int main(){try{
 auto root=std::filesystem::temp_directory_path()/("nano-uncles-"+std::to_string(getpid()));
 std::string miner="0x00000000000000000000000000000000000000aa",other="0x00000000000000000000000000000000000000bb";
 auto genesis=Genesis::from_json(Json::object());Header uncle;Bytes accepted_hash;
 {Blockchain chain(root/"producer",genesis),peer(root/"peer",genesis);
  auto first=chain.mine(miner,24);peer.import_block(first,24);
  uncle=first.header;uncle.coinbase=other;uncle.timestamp=12;seal(uncle);
  auto candidate=[&](std::vector<Header> uncles){auto block=chain.block_template(miner,Int(chain.tip().header.timestamp+24));block.uncles=std::move(uncles);auto state=chain.state();ExecutionEngine engine(genesis.chain_id);engine.execute_block(state,block);seal(block.header);return block;};
  auto reject=[&](Block block,const std::string& expected){auto before=chain.state().root();auto tip=chain.tip().header.hash();auto count=chain.store().count();auto cursor=chain.event_cursor();bool rejected=false;try{chain.import_block(block,block.header.timestamp);}catch(const std::invalid_argument& e){rejected=std::string(e.what()).find(expected)!=std::string::npos;}check(rejected,expected.c_str());check(chain.state().root()==before&&chain.tip().header.hash()==tip&&chain.store().count()==count&&chain.event_cursor()==cursor,"rejected uncle leaves state, storage and events unchanged");};
  reject(candidate({uncle,uncle}),"uncle already included");
  reject(candidate({uncle,uncle,uncle}),"too many uncles");
  reject(candidate({first.header}),"uncle is canonical ancestor");
  auto bad=uncle;bad.parent_hash=Bytes(32,0xab);seal(bad);reject(candidate({bad}),"uncle parent is not eligible ancestor");
  bad=uncle;bad.number=0;seal(bad);reject(candidate({bad}),"Invalid block number");
  bad=uncle;bad.timestamp=0;seal(bad);reject(candidate({bad}),"Timestamp must be greater");
  bad=uncle;bad.timestamp=999;seal(bad);reject(candidate({bad}),"Block is from the future");
  bad=uncle;bad.gas_used=bad.gas_limit+1;seal(bad);reject(candidate({bad}),"Gas used exceeds");
  bad=uncle;bad.base_fee=123;seal(bad);reject(candidate({bad}),"invalid uncle base fee");
  bad=uncle;bad.timestamp=6;bad.difficulty=2;for(unsigned n=0;;++n){bad.nonce=bytes(Int(n),8);if(!ProofOfWork::verify(bad))break;}reject(candidate({bad}),"Invalid proof of work");
  auto valid=candidate({uncle});chain.import_block(valid,48);peer.import_block(valid,48);accepted_hash=valid.header.hash();
  Int reward("2000000000000000000");check(chain.state().get(other).balance==reward*7/8,"valid uncle receives depth-based reward");check(chain.state().get(miner).balance==reward*2+reward/32,"producer receives inclusion reward once");
  check(chain.state().root()==peer.state().root()&&peer.store().block_by_number(2)->uncles[0].hash()==uncle.hash(),"peers replay and persist full uncle header identically");
  reject(candidate({uncle}),"uncle already included");
 }
 {Blockchain reopened(root/"producer",genesis);
  check(reopened.tip().header.hash()==accepted_hash,"uncle-bearing chain survives reopening");
  auto block=reopened.block_template(miner,72);block.uncles={uncle};auto state=reopened.state();ExecutionEngine(genesis.chain_id).execute_block(state,block);seal(block.header);
  bool rejected=false;try{reopened.import_block(block,72);}catch(const std::invalid_argument& e){rejected=std::string(e.what())=="uncle already included";}check(rejected&&reopened.tip().header.number==2,"reopening preserves uncle reuse rejection");
 }
 // Boundary: a height-1 sibling may be included at height 7, but not 8.
 {Blockchain chain(root/"depth",genesis);for(unsigned height=1;height<=6;++height)chain.mine(miner,Int(height*24));
  auto block=chain.block_template(miner,168);block.uncles={uncle};auto state=chain.state();ExecutionEngine(genesis.chain_id).execute_block(state,block);seal(block.header);chain.import_block(block,168);check(chain.state().get(other).balance==Int("500000000000000000"),"maximum uncle depth six is accepted");
  auto too_old=chain.block_template(miner,192);too_old.uncles={uncle};bool rejected=false;try{chain.import_block(too_old,192);}catch(const std::invalid_argument& e){rejected=std::string(e.what())=="invalid uncle depth";}check(rejected&&chain.tip().header.number==7,"uncle depth seven is rejected before reward execution");
 }
 {Blockchain chain(root/"two",genesis);chain.mine(miner,24);
  auto second_uncle=uncle;second_uncle.timestamp=18;seal(second_uncle);
  auto block=chain.block_template(miner,48);block.uncles={uncle,second_uncle};auto state=chain.state();ExecutionEngine(genesis.chain_id).execute_block(state,block);seal(block.header);chain.import_block(block,48);
  check(chain.state().get(other).balance==Int("3500000000000000000")&&chain.store().block_by_number(2)->uncles.size()==2,"two distinct eligible uncles receive their rewards");
 }
 std::filesystem::remove_all(root);std::cout<<checks<<" uncle validation checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<"After "<<checks<<" checks: "<<e.what()<<"\n";return 1;}}
