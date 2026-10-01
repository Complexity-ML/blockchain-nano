#include "nano/engine.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iostream>
using namespace nano;
namespace fs=std::filesystem;
// Exact mappings only: an unrelated exception must never satisfy a rejection test.
std::string validation_exception(const std::string& message){
 if(message=="transaction nonce too low")return "TransactionException.NONCE_MISMATCH_TOO_LOW";
 if(message=="transaction nonce too high")return "TransactionException.NONCE_MISMATCH_TOO_HIGH";
 if(message=="blob transaction exceeds maximum blob gas allowance")return "TransactionException.TYPE_3_TX_MAX_BLOB_GAS_ALLOWANCE_EXCEEDED";
 if(message=="blob transaction exceeds Osaka transaction blob limit")return "TransactionException.TYPE_3_TX_BLOB_COUNT_EXCEEDED";
 if(message=="transaction gas limit exceeds Osaka maximum")return "TransactionException.GAS_LIMIT_EXCEEDS_MAXIMUM";
 if(message=="transaction type 4 unavailable before Prague")return "TransactionException.TYPE_4_TX_PRE_FORK";
 if(message=="set-code transaction cannot create contract")return "TransactionException.TYPE_4_TX_CONTRACT_CREATION";
 if(message=="set-code transaction has no authorizations")return "TransactionException.TYPE_4_EMPTY_AUTHORIZATION_LIST";
 if(message=="transaction gas below calldata floor")return "TransactionException.INTRINSIC_GAS_BELOW_FLOOR_GAS_COST";
 if(message=="transaction nonce at or above maximum")return "TransactionException.NONCE_IS_MAX";
 if(message=="transaction type 3 unavailable before Cancun")return "TransactionException.TYPE_3_TX_PRE_FORK";
 if(message=="blob transaction cannot create contract")return "TransactionException.TYPE_3_TX_CONTRACT_CREATION";
 if(message=="blob transaction has no blobs")return "TransactionException.TYPE_3_TX_ZERO_BLOBS";
 if(message=="blob transaction exceeds block blob limit")return "TransactionException.TYPE_3_TX_BLOB_COUNT_EXCEEDED";
 if(message=="invalid blob versioned hash")return "TransactionException.TYPE_3_TX_INVALID_BLOB_VERSIONED_HASH";
 if(message=="blob fee cap below base fee")return "TransactionException.INSUFFICIENT_MAX_FEE_PER_BLOB_GAS";
 if(message=="invalid transaction fee cap")return "TransactionException.INSUFFICIENT_MAX_FEE_PER_GAS";
 if(message=="priority fee exceeds fee cap")return "TransactionException.PRIORITY_GREATER_THAN_MAX_FEE_PER_GAS";
 if(message=="block gas limit exceeded")return "TransactionException.GAS_ALLOWANCE_EXCEEDED";
 if(message=="transaction sender has deployed code")return "TransactionException.SENDER_NOT_EOA";
 if(message=="initcode size exceeds limit")return "TransactionException.INITCODE_SIZE_EXCEEDED";
 if(message=="intrinsic gas exceeds limit")return "TransactionException.INTRINSIC_GAS_TOO_LOW";
 if(message=="transaction chain id mismatch"||message=="chain id mismatch")return "TransactionException.INVALID_CHAINID";
 if(message=="transaction type unavailable in Berlin")return "TransactionException.TYPE_2_TX_PRE_FORK";
 if(message=="insufficient transaction funds")return "TransactionException.INSUFFICIENT_ACCOUNT_FUNDS";
 if(message=="noncanonical high-s signature"||message=="invalid typed v"||message=="invalid legacy v"||message=="invalid signature"||message=="invalid signature point"||message=="signature recovery failed")return "TransactionException.INVALID_SIGNATURE_VRS";
 return {};
}

// Differential adapter for the current Nano engine. A matching case does not
// establish fork compliance: the engine's fork selection is still incomplete.
int main(int argc,char** argv){
 if(argc==2&&std::string(argv[1])=="--version"){std::cout<<"nano-ethereum-state-runner 1\n";return 0;}
 const bool selected=argc==4;const std::string case_name=selected?argv[3]:"";
 if((argc!=2&&argc!=4)||(selected&&std::string(argv[2])!="--case")){std::cerr<<"Usage: nano-ethereum-state-runner <state-fixture.json or directory> [--case exact-id]\n";return 2;}
 uint64_t matched=0,mismatched=0,unsupported=0,errors=0,validation_matched=0;std::vector<fs::path> files;
 try{
  fs::path input=argv[1];if(fs::is_directory(input)){for(const auto& entry:fs::recursive_directory_iterator(input))if(entry.is_regular_file()&&entry.path().extension()==".json")files.push_back(entry.path());std::sort(files.begin(),files.end());}else files.push_back(input);
  for(const auto& file:files){
   if(std::getenv("NANO_STATE_TRACE"))std::cerr<<file.string()<<std::endl;
   try{
    Json fixture;std::ifstream in(file);if(!in)throw std::runtime_error("cannot open fixture");in>>fixture;
    if(!fixture.is_object())throw std::runtime_error("fixture root must be object");
    for(const auto& [name,test]:fixture.items()){
     if(selected&&name!=case_name)continue;
     if(!test.is_object()||!test.contains("pre")||!test.contains("env")||!test.contains("post")){++unsupported;std::cout<<Json{{"file",file.string()},{"case",name},{"status","unsupported-format"}}.dump()<<'\n';continue;}
     for(const auto& [fork,variants]:test.at("post").items())for(size_t i=0;i<variants.size();++i){
      Json report={{"file",file.string()},{"case",name},{"fork",fork},{"variant",i},{"engine","Nano current unversioned rules"}};
      try{
       const auto& expected=variants.at(i);const auto& env=test.at("env");
       if(!expected.contains("txbytes")){++unsupported;report["status"]="unsupported-validation-case";}
       else{
        State state=State::from_json(test.at("pre"));auto initial_state=state.to_json();
        Header header;header.coinbase=address(env.at("currentCoinbase"));header.gas_limit=quantity(env.at("currentGasLimit"));header.number=quantity(env.at("currentNumber"));header.timestamp=quantity(env.at("currentTimestamp"));
        header.base_fee=quantity(env.value("currentBaseFee",Json("0x0")));header.difficulty=quantity(env.value("currentDifficulty",Json("0x0")));header.mix_hash=bytes(quantity(env.value("currentRandom",Json("0x0"))),32);
        Int chain_id=quantity(test.value("config",Json::object()).value("chainid",Json(1)));
        EvmRevision revision=EvmRevision::NanoExperimental;
        if(fork=="Berlin")revision=EvmRevision::Berlin;else if(fork=="Paris")revision=EvmRevision::Paris;else if(fork=="Shanghai")revision=EvmRevision::Shanghai;else if(fork=="Cancun")revision=EvmRevision::Cancun;else if(fork=="Prague")revision=EvmRevision::Prague;else if(fork=="Osaka")revision=EvmRevision::Osaka;
        else{++unsupported;report["status"]="unsupported-fork";std::cout<<report.dump()<<'\n';continue;}
        if(revision>=EvmRevision::Cancun){header.excess_blob_gas=quantity(env.value("currentExcessBlobGas",Json("0x0")));header.blob_gas_used=Int(0);header.withdrawals_root=keccak(Rlp(Bytes{}).encode());}
        report["engine"]="Nano explicit "+fork+" rules (partial implementation)";
        report["replayPolicy"]="Ethereum historical signatures allowed; Nano network policy unchanged";
        // State-test convention (Geth vmTestBlockHash); no real chain history is supplied.
        auto fixture_block_hash=[](const Int& number){auto decimal=number.get_str();return keccak(Bytes(decimal.begin(),decimal.end()));};
        std::optional<Receipt> execution;std::string actual_exception,native_error;
        try{auto tx=SignedTransaction::decode(unhex(expected.at("txbytes")));execution=ExecutionEngine(chain_id,fixture_block_hash,revision,ReplayPolicy::AllowLegacyUnprotected).apply_transaction(state,tx,header,0,0);}
        catch(const std::invalid_argument& error){native_error=error.what();actual_exception=validation_exception(native_error);if(actual_exception.empty()&&native_error=="unsupported transaction type"&&revision==EvmRevision::Shanghai){
          auto raw=unhex(expected.at("txbytes"));
          if(!raw.empty()&&raw.front()==3)actual_exception="TransactionException.TYPE_3_TX_PRE_FORK";
         }
         if(actual_exception.empty())throw;}
        auto expected_exception=expected.value("expectException",std::string());
        if(!actual_exception.empty()||!expected_exception.empty()){
         auto root=hex(state.root());auto empty_logs=hex(keccak(Rlp(std::vector<Rlp>{}).encode()));
         bool unchanged=state.to_json()==initial_state;
         // Official fixtures can permit multiple exact validation reasons separated by '|'.
         bool exception_matches=false;size_t start=0;
         while(start<=expected_exception.size()){
          auto end=expected_exception.find('|',start);
          auto candidate=expected_exception.substr(start,end==std::string::npos?end:end-start);
          if(!actual_exception.empty()&&candidate==actual_exception)exception_matches=true;
          if(end==std::string::npos)break;start=end+1;
         }
         bool equal=!execution&&!actual_exception.empty()&&exception_matches&&unchanged&&root==expected.at("hash").get<std::string>()&&empty_logs==expected.at("logs").get<std::string>();
         report["validation"]={{"expected",expected_exception},{"actual",actual_exception},{"nativeError",native_error},{"stateUnchanged",unchanged},{"transactionExecuted",execution.has_value()}};
         report["stateRoot"]={{"actual",root},{"expected",expected.at("hash")}};
         report["status"]=equal?"matched":"mismatch";if(equal){++matched;++validation_matched;}else ++mismatched;
         std::cout<<report.dump()<<'\n';continue;
        }
        const auto& receipt=*execution;
        const auto raw_hash=keccak(unhex(expected.at("txbytes")));
        bool transaction_hash_matches=receipt.transaction_hash==raw_hash;
        if(expected.contains("receipt")&&expected.at("receipt").contains("transactionHash"))transaction_hash_matches=transaction_hash_matches&&receipt.transaction_hash==unhex(expected.at("receipt").at("transactionHash"));
        report["transactionHashMatches"]=transaction_hash_matches;report["transactionHash"]=hex(receipt.transaction_hash);
        std::vector<Rlp> logs;for(const auto& log:receipt.logs){std::vector<Rlp> topics;for(const auto& topic:log.topics)topics.emplace_back(topic);logs.emplace_back(std::vector<Rlp>{Rlp(unhex(address(log.address))),Rlp(topics),Rlp(log.data)});}
        auto actual_root=hex(state.root());auto actual_logs=hex(keccak(Rlp(logs).encode()));
        bool equal=transaction_hash_matches&&actual_root==expected.at("hash").get<std::string>()&&actual_logs==expected.at("logs").get<std::string>();
        report["stateRoot"]={{"actual",actual_root},{"expected",expected.at("hash")}};report["logsHash"]={{"actual",actual_logs},{"expected",expected.at("logs")}};
        if(expected.contains("receipt")&&expected.at("receipt").contains("rlp")){auto actual=hex(receipt.encode());report["receiptMatches"]=actual==expected.at("receipt").at("rlp").get<std::string>();equal=equal&&report.at("receiptMatches").get<bool>();report["gasUsed"]=quantity(receipt.gas_used);report["expectedGasUsed"]=expected.at("receipt").at("cumulativeGasUsed");}
        if(expected.contains("receipt")&&expected.at("receipt").contains("cumulativeGasUsed")){report["gasUsed"]=quantity(receipt.gas_used);report["expectedGasUsed"]=expected.at("receipt").at("cumulativeGasUsed");bool gas_matches=receipt.gas_used==quantity(expected.at("receipt").at("cumulativeGasUsed"));report["gasUsedMatches"]=gas_matches;equal=equal&&gas_matches;}
        report["status"]=equal?"matched":"mismatch";if(equal)++matched;else ++mismatched;
       }
      }catch(const std::exception& e){++errors;report["status"]="execution-or-adapter-error";report["error"]=e.what();}
      std::cout<<report.dump()<<'\n';
     }
    }
   }catch(const std::exception& e){++errors;std::cout<<Json{{"file",file.string()},{"status","fixture-error"},{"error",e.what()}}.dump()<<'\n';}
  }
 }catch(const std::exception& e){++errors;std::cerr<<e.what()<<'\n';}
 std::cout<<Json{{"summary",{{"files",files.size()},{"matched",matched},{"executionMatched",matched-validation_matched},{"mismatched",mismatched},{"unsupported",unsupported},{"errors",errors},{"validationMatched",validation_matched},{"forkConformanceProven",false}}}}.dump()<<'\n';
 return matched>0&&!mismatched&&!unsupported&&!errors?0:1;
}
