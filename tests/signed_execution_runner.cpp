#include "nano/engine.hpp"
#include <iostream>
#include <map>
using namespace nano;
// Offline diagnostic only: no RPC server, persistence, mempool or consensus activation.
int main(){try{
 Json input;std::cin>>input;
 const std::map<std::string,EvmRevision> revisions{{"Paris",EvmRevision::Paris},{"Shanghai",EvmRevision::Shanghai},{"Cancun",EvmRevision::Cancun},{"Prague",EvmRevision::Prague},{"Osaka",EvmRevision::Osaka}};
 auto revision=revisions.at(input.at("fork").get<std::string>());
 auto state=State::from_json(input.at("pre"));auto header=Header::from_json(input.at("block"));
 auto raw=unhex(input.at("rawTransaction"));auto transaction=SignedTransaction::decode(raw);
 if(transaction.hash()!=keccak(raw))throw std::invalid_argument("noncanonical signed transaction");
 // The caller must provide historical hashes if the contract accesses BLOCKHASH.
 auto ancestors=input.value("blockHashes",Json::object());
 auto hash=[&](const Int& n){auto key=quantity(n);if(!ancestors.contains(key))throw std::runtime_error("missing diagnostic ancestor hash");auto value=unhex(ancestors.at(key));if(value.size()!=32)throw std::invalid_argument("invalid diagnostic ancestor hash");return value;};
 auto receipt=ExecutionEngine(quantity(input.at("chainId")),hash,revision).apply_transaction(state,transaction,header,0,0);
 std::cout<<Json{{"receipt",receipt.to_json()},{"intrinsicGas",quantity(transaction_intrinsic_gas(transaction.tx,revision))},{"post",state.to_json()},{"stateRoot",hex(state.root())},{"scope","Single signed transaction execution; supplied account state, not full block/consensus validation"}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
