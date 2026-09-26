#include "nano/engine.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace nano;
namespace fs=std::filesystem;
namespace {
Header header(const Json& input){
 auto j=input;
 for(const auto& [from,to]:std::vector<std::pair<std::string,std::string>>{{"uncleHash","sha3Uncles"},{"coinbase","miner"},{"transactionsTrie","transactionsRoot"},{"receiptTrie","receiptsRoot"},{"bloom","logsBloom"}})if(j.contains(from)){j[to]=j.at(from);j.erase(from);}
 return Header::from_json(j);
}
// Raw-only invalid fixtures still go through the real transaction decoder.
Header raw_header(const Rlp& raw){
 const std::vector<std::string> fields={"parentHash","sha3Uncles","miner","stateRoot","transactionsRoot","receiptsRoot","logsBloom","difficulty","number","gasLimit","gasUsed","timestamp","extraData","mixHash","nonce","baseFeePerGas","withdrawalsRoot","blobGasUsed","excessBlobGas","parentBeaconBlockRoot","requestsHash"};
 if(!raw.list||raw.items.size()<15||raw.items.size()>fields.size())throw std::runtime_error("unsupported raw header shape");
 Json j;for(size_t i=0;i<raw.items.size();++i)j[fields[i]]=(i>=7&&i<=11)||i==15||i==17||i==18?quantity(raw[i].number()):hex(raw[i].blob());
 auto h=Header::from_json(j);if(h.encode()!=raw.encode())throw std::runtime_error("noncanonical raw header");return h;
}
std::string rejection(const std::string& error){
 const std::map<std::string,std::string> reasons={
  {"incorrect execution block format","BlockException.INCORRECT_BLOCK_FORMAT"},
  {"transaction type 3 unavailable before Cancun","TransactionException.TYPE_3_TX_PRE_FORK"},
  {"transaction type 4 unavailable before Prague","TransactionException.TYPE_4_TX_PRE_FORK"},
  {"execution block RLP size limit exceeded","BlockException.RLP_BLOCK_LIMIT_EXCEEDED"},
  {"blob transaction exceeds maximum blob gas allowance","TransactionException.TYPE_3_TX_MAX_BLOB_GAS_ALLOWANCE_EXCEEDED"},
  {"blob transaction exceeds Osaka transaction blob limit","TransactionException.TYPE_3_TX_BLOB_COUNT_EXCEEDED"},
  {"transaction gas limit exceeds Osaka maximum","TransactionException.GAS_LIMIT_EXCEEDS_MAXIMUM"},
  {"required system contract is empty","BlockException.SYSTEM_CONTRACT_EMPTY"},
  {"required system contract call failed","BlockException.SYSTEM_CONTRACT_CALL_FAILED"},
  {"execution requests hash mismatch","BlockException.INVALID_REQUESTS"},
  {"invalid deposit event layout","BlockException.INVALID_DEPOSIT_EVENT_LAYOUT"},
  {"transaction gas below calldata floor","TransactionException.INTRINSIC_GAS_BELOW_FLOOR_GAS_COST"},
  {"set-code transaction cannot create contract","TransactionException.TYPE_4_TX_CONTRACT_CREATION"},
  {"set-code transaction has no authorizations","TransactionException.TYPE_4_EMPTY_AUTHORIZATION_LIST"},
  {"invalid transaction fields","BlockException.RLP_STRUCTURES_ENCODING"},
  {"incorrect excess blob gas","BlockException.INCORRECT_EXCESS_BLOB_GAS"},
  {"incorrect blob gas used","BlockException.INCORRECT_BLOB_GAS_USED"},
  {"block blob gas limit exceeded","TransactionException.TYPE_3_TX_MAX_BLOB_GAS_ALLOWANCE_EXCEEDED"},
  {"blob transaction exceeds block blob limit","TransactionException.TYPE_3_TX_MAX_BLOB_GAS_ALLOWANCE_EXCEEDED"},
  {"invalid blob versioned hash","TransactionException.TYPE_3_TX_INVALID_BLOB_VERSIONED_HASH"},
  {"blob transaction has no blobs","TransactionException.TYPE_3_TX_ZERO_BLOBS"},
  {"blob fee cap below base fee","TransactionException.INSUFFICIENT_MAX_FEE_PER_BLOB_GAS"},
  {"blob transaction cannot create contract","TransactionException.TYPE_3_TX_CONTRACT_CREATION"},
  {"transaction sender has deployed code","TransactionException.SENDER_NOT_EOA"},
  {"invalid transaction fee cap","TransactionException.INSUFFICIENT_MAX_FEE_PER_GAS"},
  {"transaction nonce at or above maximum","TransactionException.NONCE_IS_MAX"},
  {"priority fee exceeds fee cap","TransactionException.PRIORITY_GREATER_THAN_MAX_FEE_PER_GAS"},
  {"invalid block gas limits","BlockException.INVALID_GASLIMIT"},
  {"withdrawals root mismatch","BlockException.INVALID_WITHDRAWALS_ROOT"},
  {"invalid base fee","BlockException.INVALID_BASEFEE_PER_GAS"},
  {"intrinsic gas exceeds limit","TransactionException.INTRINSIC_GAS_TOO_LOW"},
  {"transaction chain id mismatch","TransactionException.INVALID_CHAINID"},
  {"chain id mismatch","TransactionException.INVALID_CHAINID"},
  {"insufficient transaction funds","TransactionException.INSUFFICIENT_ACCOUNT_FUNDS"},
  {"block gas limit exceeded","TransactionException.GAS_ALLOWANCE_EXCEEDED"},
  {"transaction nonce too low","TransactionException.NONCE_MISMATCH_TOO_LOW"},
  {"transaction nonce too high","TransactionException.NONCE_MISMATCH_TOO_HIGH"},
  {"initcode size exceeds limit","TransactionException.INITCODE_SIZE_EXCEEDED"},
  {"transaction type unavailable in Berlin","TransactionException.TYPE_2_TX_PRE_FORK"}
 };
 auto it=reasons.find(error);if(it!=reasons.end())return it->second;
 for(const auto* signature:{"noncanonical high-s signature","invalid typed v","invalid legacy v","invalid signature","invalid signature point","signature recovery failed"})if(error==signature)return "TransactionException.INVALID_SIGNATURE_VRS";
 return {};
}
const std::map<std::string,EvmRevision> fixed_forks{{"Berlin",EvmRevision::Berlin},{"Paris",EvmRevision::Paris},{"Shanghai",EvmRevision::Shanghai},{"Cancun",EvmRevision::Cancun},{"Prague",EvmRevision::Prague},{"Osaka",EvmRevision::Osaka}};
const std::map<std::string,std::pair<EvmRevision,EvmRevision>> transitions{
 {"ParisToShanghaiAtTime15k",{EvmRevision::Paris,EvmRevision::Shanghai}},
 {"ShanghaiToCancunAtTime15k",{EvmRevision::Shanghai,EvmRevision::Cancun}},
 {"CancunToPragueAtTime15k",{EvmRevision::Cancun,EvmRevision::Prague}},
 {"PragueToOsakaAtTime15k",{EvmRevision::Prague,EvmRevision::Osaka}}};
EvmRevision revision_at(const std::string& fork,const Int& timestamp){
 if(auto it=fixed_forks.find(fork);it!=fixed_forks.end())return it->second;
 const auto& transition=transitions.at(fork);return timestamp<15000?transition.first:transition.second;
}
std::string unsupported(const Json& test){
 auto fork=test.value("network",std::string{});
 if(!fixed_forks.contains(fork)&&!transitions.contains(fork))return "fork/block fields not supported by adapter";
 for(const auto& b:test.at("blocks")){
  if(b.contains("expectException")){auto e=b.at("expectException").get<std::string>();if(!e.starts_with("TransactionException.")&&e!="BlockException.INVALID_GASLIMIT"&&e!="BlockException.INCORRECT_BLOCK_FORMAT"&&e!="BlockException.SYSTEM_CONTRACT_EMPTY"&&e!="BlockException.RLP_BLOCK_LIMIT_EXCEEDED"&&e!="BlockException.SYSTEM_CONTRACT_CALL_FAILED"&&e!="BlockException.INVALID_REQUESTS"&&e!="BlockException.INVALID_DEPOSIT_EVENT_LAYOUT"&&e!="BlockException.RLP_STRUCTURES_ENCODING|TransactionException.TYPE_3_TX_WITH_FULL_BLOBS"&&e!="BlockException.RLP_STRUCTURES_ENCODING|TransactionException.TYPE_3_TX_CONTRACT_CREATION"&&e!="BlockException.INCORRECT_EXCESS_BLOB_GAS"&&e!="BlockException.INCORRECT_BLOB_GAS_USED"&&e!="BlockException.BLOB_GAS_USED_ABOVE_LIMIT|BlockException.INCORRECT_BLOB_GAS_USED"&&e!="BlockException.INVALID_BASEFEE_PER_GAS"&&e!="BlockException.INVALID_WITHDRAWALS_ROOT|BlockException.INVALID_BLOCK_HASH")return "invalid-header validation not yet supported";}
  const auto& decoded=b.contains("rlp_decoded")?b.at("rlp_decoded"):b;
  if(!decoded.contains("blockHeader")&&!b.contains("expectException"))return "block header metadata missing";

 }
 return {};
}
}
int main(int argc,char** argv){
 if(argc==2&&std::string(argv[1])=="--version"){std::cout<<"nano-ethereum-block-runner 1\n";return 0;}
 const bool selected=argc==4;const std::string case_name=selected?argv[3]:"";
 if((argc!=2&&argc!=4)||(selected&&std::string(argv[2])!="--case")){std::cerr<<"Usage: nano-ethereum-block-runner <fixture file or directory> [--case exact-id]\n";return 2;}
 size_t matched=0,mismatched=0,skipped=0,errors=0;std::vector<fs::path> files;
 try{
  if(fs::is_directory(argv[1])){for(const auto& f:fs::recursive_directory_iterator(argv[1]))if(f.is_regular_file()&&f.path().extension()==".json")files.push_back(f.path());std::sort(files.begin(),files.end());}else files.emplace_back(argv[1]);
  for(const auto& file:files){Json fixture;std::ifstream in(file);if(!in)throw std::runtime_error("cannot open fixture");in>>fixture;
   for(const auto& [name,test]:fixture.items()){
     if(selected&&name!=case_name)continue;
    Json report={{"case",name},{"file",file.string()},{"scope","Block execution commitments; not full header/consensus validation"}};
    try{
     if(!test.contains("blocks")||!test.contains("pre")||!test.contains("genesisBlockHeader")){++skipped;report["status"]="unsupported-format";std::cout<<report.dump()<<'\n';continue;}
     auto why=unsupported(test);if(!why.empty()){++skipped;report["status"]="unsupported";report["reason"]=why;std::cout<<report.dump()<<'\n';continue;}
     auto fork=test.at("network").get<std::string>();report["fork"]=fork;

     State state=State::from_json(test.at("pre"));auto parent=header(test.at("genesisBlockHeader"));
     if(state.root()!=parent.state_root)throw std::runtime_error("genesis state root mismatch");
     auto genesis=Rlp::decode(unhex(test.at("genesisRLP")));if(genesis[0].encode()!=parent.encode())throw std::runtime_error("genesis RLP header mismatch");
     std::map<Int,Bytes> ancestors{{parent.number,parent.hash()}};

     size_t count=0,rejected=0;bool rejections_match=true;report["rejections"]=Json::array();
     for(const auto& b:test.at("blocks")){
      const bool expect_rejection=b.contains("expectException");
      const auto& decoded=b.contains("rlp_decoded")?b.at("rlp_decoded"):b;
      const auto before=state.to_json();std::string native_error;bool accepted=false;
      try{
      Block block;auto encoded=unhex(b.at("rlp"));
      // Fixed Osaka can reject oversized input before decoding; transition activation needs its timestamp.
      if(fixed_forks.contains(fork))validate_execution_block_size(encoded.size(),fixed_forks.at(fork));
      auto raw=Rlp::decode(encoded);
      if(!raw.list||raw.items.empty()||!raw[0].list||raw[0].items.size()<12)throw std::runtime_error("invalid block RLP shape");
      auto revision=revision_at(fork,raw[0][11].number());validate_execution_block_size(encoded.size(),revision);
      const size_t fields=revision>=EvmRevision::Prague?21:revision>=EvmRevision::Cancun?20:revision>=EvmRevision::Shanghai?17:revision==EvmRevision::Berlin?15:16;
      if(raw[0].items.size()!=fields||raw.items.size()!=(revision>=EvmRevision::Shanghai?4:3))throw std::invalid_argument("incorrect execution block format");
      ExecutionEngine engine(quantity(test.value("config",Json::object()).value("chainid",Json(1))),[&](const Int& n){auto it=ancestors.find(n);return it==ancestors.end()?Bytes(32):it->second;},revision,ReplayPolicy::AllowLegacyUnprotected,ConsensusInputPolicy::EthereumValidated);
      block.header=decoded.contains("blockHeader")?header(decoded.at("blockHeader")):raw_header(raw[0]);
      if(!raw.list||(raw.items.size()!=3&&raw.items.size()!=4)||raw[0].encode()!=block.header.encode())throw std::runtime_error("block RLP/header mismatch");
      if(!raw[1].list||!raw[2].list)throw std::runtime_error("block transactions/uncles are not lists");
      if((revision>=EvmRevision::Shanghai)!=(raw.items.size()==4))throw std::runtime_error("withdrawal body presence differs from fork");
      if(raw.items.size()==4){if(!raw[3].list)throw std::runtime_error("withdrawals are not a list");for(const auto& w:raw[3].items){if(!w.list||w.items.size()!=4)throw std::runtime_error("invalid withdrawal RLP");Json item={{"index",quantity(w[0].number())},{"validatorIndex",quantity(w[1].number())},{"address",hex(w[2].blob())},{"amount",quantity(w[3].number())}};if(withdrawal_payload(item).encode()!=w.encode())throw std::runtime_error("withdrawal encoding differs");block.withdrawals.push_back(item);}}
      for(const auto& tx:raw[1].items)block.transactions.push_back(SignedTransaction::decode(tx.list?tx.encode():tx.blob()));
      if(decoded.contains("uncleHeaders")){for(const auto& u:decoded.at("uncleHeaders"))block.uncles.push_back(header(u));}
      else for(const auto& u:raw[2].items)block.uncles.push_back(raw_header(u));
      std::vector<Rlp> uncles;for(const auto& u:block.uncles)uncles.push_back(Rlp::decode(u.encode()));if(Rlp(uncles).encode()!=raw[2].encode())throw std::runtime_error("uncle RLP mismatch");
      if(block.header.parent_hash!=parent.hash()||block.header.number!=parent.number+1)throw std::runtime_error("nonlinear ancestry unsupported");
      validate_execution_header(block.header,parent);
      auto receipts=engine.verify_block(state,block,parent,revision==EvmRevision::Berlin);accepted=true;
      if(b.contains("receipts")){if(receipts.size()!=b.at("receipts").size())throw std::runtime_error("receipt count mismatch");for(size_t i=0;i<receipts.size();++i)if(b.at("receipts")[i].contains("rlp")&&receipts[i].encode()!=unhex(b.at("receipts")[i].at("rlp")))throw std::runtime_error("receipt RLP mismatch");}
      if(!expect_rejection){parent=block.header;ancestors[parent.number]=parent.hash();++count;}
      }catch(const std::invalid_argument& e){if(!expect_rejection)throw;native_error=e.what();}
      if(expect_rejection){
       auto actual=rejection(native_error);auto expected=b.at("expectException").get<std::string>();bool unchanged=state.to_json()==before;
       bool reason_matches=false;size_t start=0;while(start<=expected.size()){auto end=expected.find('|',start);if(!actual.empty()&&expected.substr(start,end==std::string::npos?end:end-start)==actual)reason_matches=true;if(end==std::string::npos)break;start=end+1;}
       bool equal=!accepted&&unchanged&&reason_matches;
       report["rejections"].push_back({{"expected",expected},{"actual",actual},{"nativeError",native_error},{"stateUnchanged",unchanged},{"blockAccepted",accepted},{"matched",equal}});
       if(!equal){rejections_match=false;break;}++rejected;
      }
     }
     bool state_matches=test.contains("postState")&&State::from_json(test.at("postState")).root()==state.root();bool hash_matches=hex(parent.hash())==test.at("lastblockhash").get<std::string>();
     bool equal=state_matches&&hash_matches&&rejections_match;report["blocksRejected"]=rejected;report["status"]=equal?"matched":"mismatch";report["blocksReplayed"]=count;report["postStateMatches"]=state_matches;report["lastBlockHashMatches"]=hash_matches;if(equal)++matched;else ++mismatched;
    }catch(const std::exception& e){++errors;report["status"]="execution-or-adapter-error";report["error"]=e.what();}
    std::cout<<report.dump()<<'\n';
   }
  }
 }catch(const std::exception& e){++errors;std::cerr<<e.what()<<'\n';}
 std::cout<<Json{{"summary",{{"files",files.size()},{"matched",matched},{"mismatched",mismatched},{"unsupported",skipped},{"errors",errors},{"forkConformanceProven",false}}}}.dump()<<'\n';return matched&&!mismatched&&!skipped&&!errors?0:1;
}
