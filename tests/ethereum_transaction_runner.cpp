#include "nano/engine.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
using namespace nano;
namespace fs=std::filesystem;
namespace {
std::string reason(const std::string& error){
 if(error=="set-code transaction has no authorizations")return "TransactionException.TYPE_4_EMPTY_AUTHORIZATION_LIST";
 if(error=="invalid authorization signature encoding")return "TransactionException.TYPE_4_INVALID_AUTHORITY_SIGNATURE";
 const std::set<std::string> format={"invalid authorization fields","invalid authorization address","authorization field out of bounds","authorization list must be a list","noncanonical RLP integer","RLP list where bytes expected","RLP bytes where list expected","noncanonical RLP","noncanonical RLP byte","bad RLP length","truncated RLP","trailing RLP","invalid RLP nesting/length"};
 return format.contains(error)?"TransactionException.TYPE_4_INVALID_AUTHORIZATION_FORMAT":"";
}
bool permits(const std::string& expected,const std::string& actual){
 if(actual.empty())return false;
 size_t start=0;while(start<=expected.size()){auto end=expected.find('|',start);if(expected.substr(start,end==std::string::npos?end:end-start)==actual)return true;if(end==std::string::npos)break;start=end+1;}return false;
}
}
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: nano-ethereum-transaction-runner <fixture directory or file>\n";return 2;}
 size_t matched=0,mismatched=0,unsupported=0,errors=0;std::vector<fs::path> files;
 try{
  if(fs::is_directory(argv[1])){for(const auto& f:fs::recursive_directory_iterator(argv[1]))if(f.is_regular_file()&&f.path().extension()==".json")files.push_back(f.path());std::sort(files.begin(),files.end());}else files.emplace_back(argv[1]);
  for(const auto& file:files){
   Json fixture;std::ifstream stream(file);if(!stream)throw std::runtime_error("cannot open fixture");stream>>fixture;
   for(const auto& [name,test]:fixture.items()){
    if(!test.contains("result")||!test.contains("txbytes")){++unsupported;std::cout<<Json{{"case",name},{"status","unsupported-format"}}.dump()<<'\n';continue;}
    for(const auto& [fork,expected]:test.at("result").items()){
     Json report={{"file",file.string()},{"case",name},{"fork",fork},{"scope","Malformed transaction envelope rejection; synthetic funded execution context"}};
     if((fork!="Prague"&&fork!="Osaka")||!expected.contains("exception")){++unsupported;report["status"]="unsupported-case";std::cout<<report.dump()<<'\n';continue;}
     State state;Json before=state.to_json();std::string error,actual;bool executed=false;std::string stage="decode";
     try{
      auto tx=SignedTransaction::decode(unhex(test.at("txbytes")));
      auto sender=address(tx.sender());state.get(sender).balance=Int(1)<<255;state.get(sender).nonce=tx.tx.nonce;before=state.to_json();
      Header block;block.number=1;block.gas_limit=30000000;block.base_fee=0;block.excess_blob_gas=0;
      stage="execution-validation";
      ExecutionEngine(1,{},fork=="Prague"?EvmRevision::Prague:EvmRevision::Osaka).apply_transaction(state,tx,block,0,0);executed=true;
     }catch(const std::invalid_argument& e){error=e.what();actual=reason(error);}
     catch(const std::exception& e){++errors;report["status"]="adapter-or-execution-error";report["error"]=e.what();std::cout<<report.dump()<<'\n';continue;}
     bool unchanged=state.to_json()==before;bool equal=!executed&&unchanged&&permits(expected.at("exception"),actual);
     if(equal)++matched;else ++mismatched;
     report["status"]=equal?"matched":"mismatch";report["validation"]={{"expected",expected.at("exception")},{"actual",actual},{"nativeError",error},{"stage",stage},{"stateUnchanged",unchanged},{"transactionExecuted",executed}};std::cout<<report.dump()<<'\n';
    }
   }
  }
 }catch(const std::exception& e){++errors;std::cerr<<e.what()<<'\n';}
 std::cout<<Json{{"summary",{{"files",files.size()},{"matched",matched},{"mismatched",mismatched},{"unsupported",unsupported},{"errors",errors},{"forkConformanceProven",false}}}}.dump()<<'\n';return matched&&!mismatched&&!unsupported&&!errors?0:1;
}
