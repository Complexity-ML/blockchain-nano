#include "nano/engine.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace nano;
namespace fs=std::filesystem;
// Exact mappings only: an unrelated exception must never satisfy a rejection test.
std::string validation_exception(const std::string& message){
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
 if(argc!=2){std::cerr<<"Usage: nano-ethereum-state-runner <state-fixture.json or directory>\n";return 2;}
 uint64_t matched=0,mismatched=0,unsupported=0,errors=0,validation_matched=0;std::vector<fs::path> files;
 try{
  fs::path input=argv[1];if(fs::is_directory(input)){for(const auto& entry:fs::recursive_directory_iterator(input))if(entry.is_regular_file()&&entry.path().extension()==".json")files.push_back(entry.path());std::sort(files.begin(),files.end());}else files.push_back(input);
  for(const auto& file:files){
   try{
    Json fixture;std::ifstream in(file);if(!in)throw std::runtime_error("cannot open fixture");in>>fixture;
    if(!fixture.is_object())throw std::runtime_error("fixture root must be object");
    for(const auto& [name,test]:fixture.items()){
     if(!test.is_object()||!test.contains("pre")||!test.contains("env")||!test.contains("post")){++unsupported;std::cout<<Json{{"file",file.string()},{"case",name},{"status","unsupported-format"}}.dump()<<'\n';continue;}
     for(const auto& [fork,variants]:test.at("post").items())for(size_t i=0;i<variants.size();++i){
      Json report={{"file",file.string()},{"case",name},{"fork",fork},{"variant",i},{"engine","Nano current unversioned rules"}};
      try{
       const auto& expected=variants.at(i);const auto& env=test.at("env");
       if(!expected.contains("txbytes")){++unsupported;report["status"]="unsupported-validation-case";}
       else{
        State state=State::from_json(test.at("pre"));auto initial_state=state.to_json();
        Header header;header.coinbase=address(env.at("currentCoinbase"));header.gas_limit=quantity(env.at("currentGasLimit"));header.number=quantity(env.at("currentNumber"));header.timestamp=quantity(env.at("currentTimestamp"));
        header.base_fee=quantity(env.value("currentBaseFee",Json("0x0")));header.difficulty=quantity(env.value("currentRandom",env.value("currentDifficulty",Json("0x0"))));
        Int chain_id=quantity(test.value("config",Json::object()).value("chainid",Json(1)));
        EvmRevision revision=EvmRevision::NanoExperimental;
        if(fork=="Berlin")revision=EvmRevision::Berlin;else if(fork=="Paris")revision=EvmRevision::Paris;
        else{++unsupported;report["status"]="unsupported-fork";std::cout<<report.dump()<<'\n';continue;}
        report["engine"]="Nano explicit "+fork+" rules (partial implementation)";
        report["replayPolicy"]="Ethereum historical signatures allowed; Nano network policy unchanged";
        // State-test convention (Geth vmTestBlockHash); no real chain history is supplied.
        auto fixture_block_hash=[](const Int& number){auto decimal=number.get_str();return keccak(Bytes(decimal.begin(),decimal.end()));};
        std::optional<Receipt> execution;std::string actual_exception,native_error;
        try{auto tx=SignedTransaction::decode(unhex(expected.at("txbytes")));execution=ExecutionEngine(chain_id,fixture_block_hash,revision,ReplayPolicy::AllowLegacyUnprotected).apply_transaction(state,tx,header,0,0);}
        catch(const std::invalid_argument& error){native_error=error.what();actual_exception=validation_exception(native_error);if(actual_exception.empty())throw;}
        auto expected_exception=expected.value("expectException",std::string());
        if(!actual_exception.empty()||!expected_exception.empty()){
         auto root=hex(state.root());auto empty_logs=hex(keccak(Rlp(std::vector<Rlp>{}).encode()));
         bool unchanged=state.to_json()==initial_state;
         bool equal=!execution&&!actual_exception.empty()&&actual_exception==expected_exception&&unchanged&&root==expected.at("hash").get<std::string>()&&empty_logs==expected.at("logs").get<std::string>();
         report["validation"]={{"expected",expected_exception},{"actual",actual_exception},{"nativeError",native_error},{"stateUnchanged",unchanged},{"transactionExecuted",execution.has_value()}};
         report["stateRoot"]={{"actual",root},{"expected",expected.at("hash")}};
         report["status"]=equal?"matched":"mismatch";if(equal){++matched;++validation_matched;}else ++mismatched;
         std::cout<<report.dump()<<'\n';continue;
        }
        const auto& receipt=*execution;
        std::vector<Rlp> logs;for(const auto& log:receipt.logs){std::vector<Rlp> topics;for(const auto& topic:log.topics)topics.emplace_back(topic);logs.emplace_back(std::vector<Rlp>{Rlp(unhex(address(log.address))),Rlp(topics),Rlp(log.data)});}
        auto actual_root=hex(state.root());auto actual_logs=hex(keccak(Rlp(logs).encode()));
        bool equal=actual_root==expected.at("hash").get<std::string>()&&actual_logs==expected.at("logs").get<std::string>();
        report["stateRoot"]={{"actual",actual_root},{"expected",expected.at("hash")}};report["logsHash"]={{"actual",actual_logs},{"expected",expected.at("logs")}};
        if(expected.contains("receipt")&&expected.at("receipt").contains("rlp")){auto actual=hex(receipt.encode());report["receiptMatches"]=actual==expected.at("receipt").at("rlp").get<std::string>();equal=equal&&report.at("receiptMatches").get<bool>();report["gasUsed"]=quantity(receipt.gas_used);report["expectedGasUsed"]=expected.at("receipt").at("cumulativeGasUsed");}
        report["status"]=equal?"matched":"mismatch";if(equal)++matched;else ++mismatched;
       }
      }catch(const std::exception& e){++errors;report["status"]="execution-or-adapter-error";report["error"]=e.what();}
      std::cout<<report.dump()<<'\n';
     }
    }
   }catch(const std::exception& e){++errors;std::cout<<Json{{"file",file.string()},{"status","fixture-error"},{"error",e.what()}}.dump()<<'\n';}
  }
 }catch(const std::exception& e){++errors;std::cerr<<e.what()<<'\n';}
 std::cout<<Json{{"summary",{{"files",files.size()},{"matched",matched},{"mismatched",mismatched},{"unsupported",unsupported},{"errors",errors},{"validationMatched",validation_matched},{"forkConformanceProven",false}}}}.dump()<<'\n';
 return matched>0&&!mismatched&&!unsupported&&!errors?0:1;
}
