#include "nano/client.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
namespace nano {
Int nano_amount(const std::string& input){
 if(input.empty()||input.size()>100)throw std::invalid_argument("invalid Nano amount");auto point=input.find('.');auto whole=input.substr(0,point);auto fraction=point==std::string::npos?std::string():input.substr(point+1);
 auto digits=[](const std::string& value){return std::all_of(value.begin(),value.end(),[](char c){return c>='0'&&c<='9';});};
 if(whole.empty()||!digits(whole)||!digits(fraction)||fraction.size()>18||(point!=std::string::npos&&fraction.empty()))throw std::invalid_argument("amount must be a nonnegative decimal with at most 18 decimal places");
 fraction.resize(18,'0');Int amount=quantity(Json(whole))*Int("1000000000000000000")+quantity(Json(fraction));if(amount>=(Int(1)<<256))throw std::invalid_argument("amount exceeds uint256");return amount;
}
Json submit_staking(RpcClient& client,const Wallet& wallet,const std::string& target,const std::string& operation,const Int& value){
 bool withdraw=operation=="stake-withdraw";
 if(operation!="stake-register"&&operation!="stake-add"&&!withdraw)throw std::invalid_argument("invalid staking operation");
 if(value<0||(!withdraw&&value==0)||(withdraw&&value!=0))throw std::invalid_argument("invalid staking value");
 bytes(value,32);
 auto destination=address(target);
 if(unhex(client.call("eth_getCode",{destination,"latest"}).get<std::string>()).empty())throw std::invalid_argument("staking target has no deployed code");
 auto network=client.call("nno_getNetworkInfo");auto configured=network.at("network").at("stakingContract");
 if(!configured.is_string()||address(configured.get<std::string>())!=destination)throw std::invalid_argument("staking target differs from the chain staking contract");
 Transaction tx;tx.to=destination;tx.value=value;tx.chain_id=quantity(client.call("eth_chainId"));
 tx.nonce=quantity(client.call("eth_getTransactionCount",{wallet.address(),"pending"}));tx.gas_price=quantity(client.call("eth_gasPrice"));
 std::string signature=withdraw?"withdrawStake()":operation=="stake-add"?"addStake()":"registerValidator()";
 tx.data=keccak(Bytes(signature.begin(),signature.end()));tx.data.resize(4);
 Json call=tx.to_json();call.erase("gas");call["from"]=wallet.address();
 tx.gas=quantity(client.call("eth_estimateGas",{call}));auto latest=client.call("eth_getBlockByNumber",{"latest",false});
 tx.gas=std::min(quantity(latest.at("gasLimit")),Int(tx.gas+25000));
 auto signed_tx=wallet.sign_transaction(tx);auto hash=hex(signed_tx.hash());
 if(client.call("eth_sendRawTransaction",{hex(signed_tx.raw())})!=hash)throw std::runtime_error("RPC returned unexpected transaction hash");
 return {{"hash",hash},{"from",wallet.address()},{"nonce",quantity(tx.nonce)},{"operation",operation},{"valueWei",value.get_str()}};
}
int rpc_command(int argc,char** argv){
 if(argc<4)throw std::invalid_argument("rpc <URL> <info|nodes|network|balance|send|deploy|receipt|mine> [arguments]");
 std::string command=argv[3];bool send=command=="send",deploy=command=="deploy";
 bool stake_register=command=="stake-register",stake_add=command=="stake-add",stake_withdraw=command=="stake-withdraw";
 bool staking=stake_register||stake_add||stake_withdraw;
 if((command=="info"||command=="nodes"||command=="network"||command=="mine")?argc!=4:(command=="balance"||command=="receipt")?argc!=5:(send||stake_register||stake_add)?argc!=7:stake_withdraw?argc!=6:deploy?(argc<6||argc>8):true)throw std::invalid_argument("invalid RPC command arguments");
 RpcClient client({argv[2]},std::chrono::seconds(10),1);
 if(command=="mine"){std::cout<<client.call("evm_mine").dump()<<"\n";return 0;}
 if(command=="info"){
  std::cout<<Json{{"chainId",client.call("eth_chainId")},{"blockNumber",client.call("eth_blockNumber")},{"gasPrice",client.call("eth_gasPrice")},{"client",client.call("web3_clientVersion")}}.dump(2)<<"\n";return 0;
 }
 if(command=="nodes"||command=="network"){std::cout<<client.call(command=="nodes"?"nno_getNodes":"nno_getNetworkInfo").dump(2)<<"\n";return 0;}
 if(command=="balance"){
  auto addr=address(argv[4]);Int balance=quantity(client.call("eth_getBalance",{addr,"latest"}));Int scale("1000000000000000000");std::string fraction=Int(balance%scale).get_str();fraction.insert(0,18-fraction.size(),'0');while(fraction.size()>1&&fraction.back()=='0')fraction.pop_back();
  std::cout<<Json{{"address",checksum_address(addr)},{"wei",balance.get_str()},{"nano",Int(balance/scale).get_str()+"."+fraction}}.dump(2)<<"\n";return 0;
 }
 if(command=="receipt"){auto hash=unhex(argv[4]);if(hash.size()!=32)throw std::invalid_argument("expected transaction hash");std::cout<<client.call("eth_getTransactionReceipt",{hex(hash)}).dump(2)<<"\n";return 0;}
 const char* password=std::getenv("NANO_WALLET_PASSWORD");if(!password||!*password)throw std::invalid_argument("NANO_WALLET_PASSWORD is required");auto wallet=Wallet::from_keyfile(argv[4],password);
 if(staking){std::cout<<submit_staking(client,wallet,argv[5],command,stake_withdraw?Int(0):nano_amount(argv[6])).dump(2)<<"\n";return 0;}
 Transaction tx;tx.chain_id=quantity(client.call("eth_chainId"));tx.nonce=quantity(client.call("eth_getTransactionCount",{wallet.address(),"pending"}));tx.gas_price=quantity(client.call("eth_gasPrice"));
 uint64_t timeout=120;
 if(send){tx.to=address(argv[5]);tx.value=nano_amount(argv[6]);}
 else{
  if(argc>=7)timeout=u64(quantity(Json(argv[6])));if(timeout<1||timeout>3600)throw std::invalid_argument("receipt timeout must be 1 to 3600 seconds");
  if(argc==8)tx.value=nano_amount(argv[7]);
  std::ifstream file(argv[5]);if(!file)throw std::invalid_argument("cannot open bytecode file");std::string bytecode((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
  if(bytecode.size()>2*1024*1024)throw std::invalid_argument("bytecode file too large");auto last=bytecode.find_last_not_of(" \r\n\t");if(last==std::string::npos)throw std::invalid_argument("empty bytecode");bytecode.resize(last+1);bytecode.erase(0,bytecode.find_first_not_of(" \r\n\t"));tx.data=unhex(bytecode);if(tx.data.empty())throw std::invalid_argument("empty bytecode");
 }
 Json call=tx.to_json();call.erase("gas");call["from"]=wallet.address();tx.gas=quantity(client.call("eth_estimateGas",{call}));
 auto signed_tx=wallet.sign_transaction(tx);auto hash=hex(signed_tx.hash());auto accepted=client.call("eth_sendRawTransaction",{hex(signed_tx.raw())});if(accepted!=hash)throw std::runtime_error("RPC returned unexpected transaction hash");
 Json result={{"hash",hash},{"from",wallet.address()},{"nonce",quantity(tx.nonce)}};
 if(send){result["valueWei"]=tx.value.get_str();std::cout<<result.dump(2)<<"\n";return 0;}
 std::cout<<Json{{"event","submitted"},{"hash",hash}}.dump()<<std::endl;
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(timeout);
 while(std::chrono::steady_clock::now()<deadline){
  auto receipt=client.call("eth_getTransactionReceipt",{hash});if(!receipt.is_null()){
   if(quantity(receipt.at("status"))!=1)throw std::runtime_error("deployment transaction failed: "+hash);
   result["contractAddress"]=receipt.at("contractAddress");result["gasUsed"]=receipt.at("gasUsed");result["blockHash"]=receipt.at("blockHash");std::cout<<result.dump(2)<<"\n";return 0;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
 }
 throw std::runtime_error("deployment pending; receipt timeout for "+hash);
}
}
