#include "nano/chain.hpp"
#include "nano/evm.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace nano;
static unsigned checks=0;
static void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 Int chain_id=argc>1?Int(argv[1]):Int(1337);unsigned tx_type=argc>2?unsigned(std::stoul(argv[2])):0;
 check(chain_id>0&&(tx_type==0||tx_type==2),"valid test chain and transaction envelope");
 Json artifact;std::ifstream("tests/fixtures/erc20.json")>>artifact;
 auto key=bytes(Int(1),32), spender_key=bytes(Int(2),32);
 auto owner=public_address(public_key(key)),spender=public_address(public_key(spender_key));
 auto recipient=public_address(public_key(bytes(Int(3),32)));
 auto token=create_address(owner,0);
 auto root=std::filesystem::temp_directory_path()/("nano-erc20-"+std::to_string(getpid()));
 struct Cleanup {std::filesystem::path path;~Cleanup(){std::filesystem::remove_all(path);}} cleanup{root};
 auto genesis_json=Json({{"config",{{"chainId",quantity(chain_id)},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",owner}}},{"alloc",{{owner,{{"balance",quantity(Int(1000000)*Int("1000000000000000000"))}}},{spender,{{"balance",quantity(Int(1000000)*Int("1000000000000000000"))}}},{recipient,{{"balance",quantity(Int(1000000)*Int("1000000000000000000"))}}}}}});
 auto genesis=Genesis::from_json(genesis_json);
 auto foreign_json=genesis_json;foreign_json["config"]["chainId"]=quantity(chain_id+1);
 auto foreign_genesis=Genesis::from_json(foreign_json);
 check(genesis.block.header.hash()!=foreign_genesis.block.header.hash(),"temporary chain identity has distinct genesis hash");
 auto data=[&](const char* method,std::initializer_list<Int> args){auto result=unhex(artifact.at("methodIdentifiers").at(method));for(const auto& arg:args){auto word=bytes(arg,32);result.insert(result.end(),word.begin(),word.end());}return result;};
 auto addr=[](const std::string& a){return integer(unhex(a));};
 auto read=[&](Blockchain& chain,const char* method,std::initializer_list<Int> args){auto state=chain.state();ExecutionContext ctx;ctx.address=token;ctx.caller=owner;ctx.origin=owner;ctx.code=state.get(token).code;ctx.data=data(method,args);ctx.chain_id=chain_id;ctx.is_static=true;auto result=EVM(state).execute(ctx);check(result.success&&result.return_data.size()==32,"ERC20 getter returns ABI word");return integer(result.return_data);};
 Int supply=Int(1000000)*Int("1000000000000000000");Bytes final_root;
 {Blockchain chain(root/"producer",genesis),peer(root/"peer",genesis),foreign(root/"foreign",foreign_genesis);
 auto foreign_root=foreign.state().root();auto foreign_tip=foreign.tip().header.hash();
 auto send=[&](const Bytes& signer,std::optional<std::string> to,Bytes payload){auto sender=public_address(public_key(signer));Transaction tx;tx.chain_id=chain_id;tx.type=tx_type;tx.max_fee=2000000000;tx.priority_fee=1000000000;tx.nonce=chain.pending_nonce(sender);tx.to=to;tx.data=std::move(payload);tx.gas=3000000;tx.gas_price=2000000000;auto signed_tx=SignedTransaction::sign(tx,signer);
 bool rejected=false;try{foreign.submit(signed_tx);}catch(const std::invalid_argument& error){rejected=std::string(error.what())=="wrong chain ID";}
 check(rejected&&foreign.pending_count()==0&&foreign.state().root()==foreign_root&&foreign.tip().header.hash()==foreign_tip,"foreign chain rejects signed contract transaction without mutation");
 // Rewrite the domain on the wire while retaining r/s: this must not authorize
 // spending by the original signer on the other identity.
 auto tampered=signed_tx;tampered.tx.chain_id=foreign_genesis.chain_id;if(tampered.tx.type==0)tampered.v+=2;
 rejected=false;try{auto decoded=SignedTransaction::decode(tampered.raw());check(address(decoded.sender())!=address(sender),"rewritten domain cannot recover original signer");foreign.submit(decoded);}catch(const std::invalid_argument&){rejected=true;}
 check(rejected&&foreign.pending_count()==0&&foreign.state().root()==foreign_root&&foreign.tip().header.hash()==foreign_tip,"rewritten chain ID cannot spend funded victim accounts");
 check(chain.submit(signed_tx),"signed ERC20 transaction admitted");auto time=Int(chain.tip().header.timestamp+12);auto block=chain.validate_signed(key,time);peer.import_block(block,time);check(chain.state().root()==peer.state().root(),"independent peer replays ERC20 state");auto receipt=chain.store().receipt(signed_tx.hash());check(receipt.has_value(),"ERC20 receipt persisted");check(receipt->to_json()==peer.store().receipt(signed_tx.hash())->to_json(),"peer reproduces receipt and logs");return *receipt;};
 check(send(key,std::nullopt,unhex(artifact.at("bytecode"))).success,"OpenZeppelin constructor deploys");
 check(chain.state().get(token).code==unhex(artifact.at("deployedBytecode")),"exact standard runtime deployed");
 check(read(chain,"totalSupply()",{})==supply&&read(chain,"balanceOf(address)",{addr(owner)})==supply,"constructor mints fixed supply");
 auto transfer=send(key,token,data("transfer(address,uint256)",{addr(recipient),100}));
 check(transfer.success&&transfer.logs.size()==1&&transfer.logs[0].topics.size()==3&&transfer.logs[0].topics[1]==bytes(addr(owner),32)&&transfer.logs[0].topics[2]==bytes(addr(recipient),32)&&integer(transfer.logs[0].data)==100,"transfer emits indexed sender recipient and amount");
 check(read(chain,"balanceOf(address)",{addr(recipient)})==100,"recipient receives exact amount");
 check(send(key,token,data("approve(address,uint256)",{addr(spender),70})).success,"owner grants allowance");
 check(send(spender_key,token,data("transferFrom(address,address,uint256)",{addr(owner),addr(recipient),40})).success,"spender spends allowance");
 check(read(chain,"allowance(address,address)",{addr(owner),addr(spender)})==30,"finite allowance decrements");
 auto refused=send(spender_key,token,data("transferFrom(address,address,uint256)",{addr(owner),addr(recipient),31}));
 check(!refused.success&&refused.logs.empty()&&read(chain,"allowance(address,address)",{addr(owner),addr(spender)})==30&&read(chain,"balanceOf(address)",{addr(recipient)})==140,"overspending rolls back balances allowance and logs");
 Int maximum=(Int(1)<<256)-1;
 check(send(key,token,data("approve(address,uint256)",{addr(spender),maximum})).success,"unlimited approval accepted");
 refused=send(spender_key,token,data("transferFrom(address,address,uint256)",{addr(owner),addr(recipient),supply}));
 check(!refused.success&&refused.logs.empty(),"insufficient owner balance reverts delegated transfer");
 check(send(spender_key,token,data("transferFrom(address,address,uint256)",{addr(owner),addr(recipient),1})).success&&read(chain,"allowance(address,address)",{addr(owner),addr(spender)})==maximum,"unlimited allowance remains unchanged");
 refused=send(key,token,data("transfer(address,uint256)",{0,1}));check(!refused.success&&refused.logs.empty(),"zero recipient rejected");
 check(send(key,token,data("approve(address,uint256)",{addr(spender),0})).success,"approval revoked");
 refused=send(spender_key,token,data("transferFrom(address,address,uint256)",{addr(owner),addr(recipient),1}));check(!refused.success,"revoked spender refused");
 check(read(chain,"balanceOf(address)",{addr(owner)})==supply-141&&read(chain,"balanceOf(address)",{addr(recipient)})==141&&read(chain,"totalSupply()",{})==supply,"supply and balances conserved across failures");
 // Exercise OpenZeppelin SafeERC20 through actual nested EVM calls.
 auto deploy_fixture=[&](const char* name){Json fixture;std::ifstream(std::string("tests/fixtures/")+name+".json")>>fixture;auto destination=create_address(owner,chain.pending_nonce(owner));check(send(key,std::nullopt,unhex(fixture.at("bytecode"))).success,"SafeERC20 fixture deploys");return destination;};
 auto safe_user=deploy_fixture("SafeTokenUser"),false_token=deploy_fixture("FalseToken"),empty_token=deploy_fixture("EmptyReturnToken");
 check(send(key,token,data("transfer(address,uint256)",{addr(safe_user),10})).success,"fund SafeERC20 caller with standard token");
 auto safe_payload=[&](const std::string& asset,Int amount){auto signature=std::string("sendToken(address,address,uint256)");auto hash=keccak(Bytes(signature.begin(),signature.end()));Bytes payload(hash.begin(),hash.begin()+4);for(const Int& value:std::vector<Int>{addr(asset),addr(owner),amount}){auto word=bytes(value,32);payload.insert(payload.end(),word.begin(),word.end());}return payload;};
 check(send(key,safe_user,safe_payload(token,10)).success&&read(chain,"balanceOf(address)",{addr(safe_user)})==0,"SafeERC20 accepts standard bool-return transfer");
 auto false_result=send(key,safe_user,safe_payload(false_token,7));
 check(!false_result.success&&false_result.logs.empty()&&chain.state().get_storage(false_token,0)==0,"SafeERC20 false response reverts nested token mutation");
 check(send(key,safe_user,safe_payload(empty_token,7)).success&&chain.state().get_storage(empty_token,0)==7,"SafeERC20 accepts successful contract with no return data");
 check(!send(key,safe_user,safe_payload(recipient,7)).success,"SafeERC20 refuses address without token code");
 // Standard NFT ownership and receiver safety on the same signed chain.
 auto nft721=deploy_fixture("TestNFT721"),nft1155=deploy_fixture("TestNFT1155");
 auto abi=[](const std::string& signature,std::initializer_list<Int> values){auto hash=keccak(Bytes(signature.begin(),signature.end()));Bytes payload(hash.begin(),hash.begin()+4);for(const auto& value:values){auto word=bytes(value,32);payload.insert(payload.end(),word.begin(),word.end());}return payload;};
 auto nft_read=[&](const std::string& destination,Bytes payload){auto state=chain.state();ExecutionContext ctx;ctx.address=destination;ctx.caller=owner;ctx.origin=owner;ctx.code=state.get(destination).code;ctx.data=std::move(payload);ctx.chain_id=chain_id;ctx.is_static=true;auto result=EVM(state).execute(ctx);check(result.success&&result.return_data.size()==32,"NFT getter returns ABI word");return integer(result.return_data);};
 check(nft_read(nft721,abi("ownerOf(uint256)",{1}))==addr(owner),"ERC721 constructor assigns ownership");
 check(!send(spender_key,nft721,abi("transferFrom(address,address,uint256)",{addr(owner),addr(recipient),1})).success,"ERC721 unauthorized operator rejected");
 check(send(key,nft721,abi("approve(address,uint256)",{addr(spender),1})).success,"ERC721 token approval granted");
 check(!send(spender_key,nft721,abi("safeTransferFrom(address,address,uint256)",{addr(owner),addr(safe_user),1})).success,"ERC721 rejects contract without receiver callback");
 check(nft_read(nft721,abi("ownerOf(uint256)",{1}))==addr(owner)&&nft_read(nft721,abi("getApproved(uint256)",{1}))==addr(spender),"failed safe NFT transfer restores owner and approval");
 check(send(spender_key,nft721,abi("safeTransferFrom(address,address,uint256)",{addr(owner),addr(recipient),1})).success,"approved operator transfers NFT to EOA");
 check(nft_read(nft721,abi("ownerOf(uint256)",{1}))==addr(recipient)&&nft_read(nft721,abi("getApproved(uint256)",{1}))==0,"ERC721 transfer updates owner and clears approval");
 auto multi_transfer=[&](const std::string& destination,unsigned amount){return abi("safeTransferFrom(address,address,uint256,uint256,bytes)",{addr(owner),addr(destination),1,amount,160,0});};
 check(nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==10,"ERC1155 constructor mints ten units");
 check(!send(spender_key,nft1155,multi_transfer(recipient,3)).success,"ERC1155 unauthorized operator rejected");
 check(send(key,nft1155,abi("setApprovalForAll(address,bool)",{addr(spender),1})).success,"ERC1155 operator approved");
 check(!send(spender_key,nft1155,multi_transfer(safe_user,3)).success&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==10,"ERC1155 rejected receiver restores balances");
 check(send(spender_key,nft1155,multi_transfer(recipient,3)).success,"ERC1155 approved transfer executes");
 check(nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==7&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(recipient),1}))==3,"ERC1155 conserves balances");
 check(!send(spender_key,nft1155,multi_transfer(recipient,8)).success,"ERC1155 insufficient balance rejected");
 check(send(key,nft1155,abi("setApprovalForAll(address,bool)",{addr(spender),0})).success&&!send(spender_key,nft1155,multi_transfer(recipient,1)).success,"ERC1155 revoked operator rejected");
 // Receiver callbacks can reenter the token legitimately; never duplicate assets.
 auto receiver=deploy_fixture("TestNFTReceiver");
 auto configure=[&](unsigned mode){check(send(key,receiver,abi("configure(uint256,address)",{mode,addr(owner)})).success,"configure adversarial NFT receiver");};
 auto recipient_key=bytes(Int(3),32);
 for(unsigned mode:{1u,2u}){
  configure(mode);
  auto failure=send(recipient_key,nft721,abi("safeTransferFrom(address,address,uint256)",{addr(recipient),addr(receiver),1}));
  check(!failure.success&&failure.logs.empty()&&nft_read(nft721,abi("ownerOf(uint256)",{1}))==addr(recipient)&&nft_read(receiver,abi("calls()",{}))==0,"wrong selector or revert rolls back NFT and receiver writes");
 }
 configure(3);
 auto forwarded=send(recipient_key,nft721,abi("safeTransferFrom(address,address,uint256)",{addr(recipient),addr(receiver),1}));
 check(forwarded.success&&forwarded.logs.size()==2&&nft_read(nft721,abi("ownerOf(uint256)",{1}))==addr(owner)&&nft_read(receiver,abi("calls()",{}))==1,"ERC721 callback may forward exactly once with coherent ownership");
 // Two entries for the same ERC1155 id exercise cumulative balance updates.
 auto batch=abi("safeBatchTransferFrom(address,address,uint256[],uint256[],bytes)",{addr(owner),addr(receiver),160,256,352,2,1,1,2,1,2,0});
 for(unsigned mode:{1u,2u}){
  configure(mode);auto failure=send(key,nft1155,batch);
  check(!failure.success&&failure.logs.empty()&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==7&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(receiver),1}))==0&&nft_read(receiver,abi("calls()",{}))==1,"ERC1155 rejected batch restores repeated-id balances and callback writes");
 }
 configure(0);auto accepted=send(key,nft1155,batch);
 check(accepted.success&&accepted.logs.size()==1&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==4&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(receiver),1}))==3&&nft_read(receiver,abi("calls()",{}))==2,"ERC1155 batch receiver accepts repeated ids without lost updates");
 configure(3);auto returned=send(key,nft1155,multi_transfer(receiver,4));
 check(returned.success&&returned.logs.size()==2&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==4&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(receiver),1}))==3&&nft_read(receiver,abi("calls()",{}))==3,"ERC1155 callback forwards incoming units without duplicating existing balance");
 // Ordinary edge cases and malicious ABI/batch inputs must remain atomic.
 auto before_zero=nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}));
 check(send(key,nft1155,multi_transfer(recipient,0)).success&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==before_zero,"zero-quantity transfer succeeds without changing balances");
 std::vector<Bytes> malformed;
 // First debit succeeds in isolation, second repeated-id debit exceeds remainder.
 malformed.push_back(abi("safeBatchTransferFrom(address,address,uint256[],uint256[],bytes)",{addr(owner),addr(receiver),160,256,352,2,1,1,2,3,3,0}));
 // ids.length=2, values.length=1, followed by valid empty bytes tail.
 malformed.push_back(abi("safeBatchTransferFrom(address,address,uint256[],uint256[],bytes)",{addr(owner),addr(receiver),160,256,320,2,1,1,1,1,0}));
 auto truncated=batch;truncated.resize(4+32*4);malformed.push_back(truncated);
 Int extreme=(Int(1)<<256)-1;
 malformed.push_back(abi("safeBatchTransferFrom(address,address,uint256[],uint256[],bytes)",{addr(owner),addr(receiver),extreme,256,352,2,1,1,2,1,2,0}));
 malformed.push_back(abi("safeTransferFrom(address,address,uint256,uint256,bytes)",{addr(owner),0,1,1,160,0}));
 auto receiver_calls=nft_read(receiver,abi("calls()",{}));
 for(const auto& payload:malformed){auto failure=send(key,nft1155,payload);
  check(!failure.success&&failure.logs.empty(),"malformed or overdrawn NFT operation fails without events");
  check(nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==4&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(receiver),1}))==3&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(recipient),1}))==3&&nft_read(receiver,abi("calls()",{}))==receiver_calls,"failed batch or ABI attack leaves every token balance and receiver write intact");
 }
 // Successful application exploit must remain deterministic and isolated.
 auto market=deploy_fixture("VulnerableTestMarket");
 check(send(key,nft721,abi("approve(address,uint256)",{addr(market),1})).success,"approve local market escrow");
 check(send(key,market,abi("list(address,uint256)",{addr(nft721),1})).success&&nft_read(nft721,abi("ownerOf(uint256)",{1}))==addr(market),"seller escrows NFT");
 auto unrelated_balance=read(chain,"balanceOf(address)",{addr(recipient)});
 auto stolen=send(spender_key,market,abi("cancel(address,uint256)",{addr(nft721),1}));
 check(stolen.success&&stolen.logs.size()==1&&nft_read(nft721,abi("ownerOf(uint256)",{1}))==addr(spender),"missing seller check allows actual NFT theft in vulnerable market");
 check(nft_read(market,abi("sellers(address,uint256)",{addr(nft721),1}))==0,"exploited listing is consumed deterministically");
 check(read(chain,"balanceOf(address)",{addr(recipient)})==unrelated_balance&&nft_read(nft1155,abi("balanceOf(address,uint256)",{addr(owner),1}))==4,"market exploit cannot mutate unrelated token state");
 check(!send(spender_key,market,abi("cancel(address,uint256)",{addr(nft721),1})).success,"repeating theft cannot duplicate NFT");
 auto nonce_before=chain.pending_nonce(spender);auto attacker_balance=chain.state().get(spender).balance;
 auto exhausted=send(spender_key,market,abi("exhaustGas()",{}));
 check(!exhausted.success&&exhausted.logs.empty()&&nft_read(market,abi("writes()",{}))==0,"unbounded market loop exhausts transaction gas and rolls back writes");
 Int charged_price=2000000000;if(tx_type==2){Int offered=*chain.tip().header.base_fee+1000000000;if(offered<charged_price)charged_price=offered;}
 check(exhausted.gas_used==3000000&&chain.state().get(spender).balance==attacker_balance-exhausted.gas_used*charged_price,"gas exhaustion charges the attacker the full gas limit");
 check(chain.pending_nonce(spender)==nonce_before+1,"failed attack consumes exactly one nonce");
 check(send(spender_key,nft721,abi("transferFrom(address,address,uint256)",{addr(spender),addr(owner),1})).success&&nft_read(nft721,abi("ownerOf(uint256)",{1}))==addr(owner),"chain executes healthy transaction after exploit and gas exhaustion");
 // Publish every concrete contract in the pinned upstream catalogue.
 Json catalogue;std::ifstream("tests/fixtures/openzeppelin-deployments.json")>>catalogue;
 std::map<std::string,Json> concrete;for(const auto& entry:catalogue.at("contracts"))concrete.emplace(entry.at("name").get<std::string>(),entry);
 auto words=[](std::initializer_list<Int> values){Bytes out;for(const auto& value:values){auto word=bytes(value,32);out.insert(out.end(),word.begin(),word.end());}return out;};
 auto publish=[&](const std::string& name,Bytes arguments){auto found=concrete.find(name);check(found!=concrete.end(),"concrete upstream deployment has fixture");auto payload=unhex(found->second.at("bytecode"));payload.insert(payload.end(),arguments.begin(),arguments.end());auto destination=create_address(owner,chain.pending_nonce(owner));check(send(key,std::nullopt,payload).success&&!chain.state().get(destination).code.empty(),"developer deploys upstream contract by signed transaction");concrete.erase(found);return destination;};
 auto manager=publish("AccessManager",words({addr(owner)}));
 check(nft_read(manager,abi("ADMIN_ROLE()",{}))==0,"AccessManager deployed role constant callable");
 auto vesting=publish("VestingWallet",words({addr(owner),100,200}));
 check(nft_read(vesting,abi("owner()",{}))==addr(owner)&&nft_read(vesting,abi("start()",{}))==100&&nft_read(vesting,abi("duration()",{}))==200,"VestingWallet constructor parameters persist");
 auto timelock=publish("TimelockController",words({10,128,192,addr(owner),1,addr(owner),1,addr(owner)}));
 check(nft_read(timelock,abi("getMinDelay()",{}))==10,"Timelock constructor arrays and delay accepted");
 auto forward_args=words({32,4});auto name_bytes=Bytes{'N','a','n','o'};forward_args.insert(forward_args.end(),name_bytes.begin(),name_bytes.end());forward_args.resize(96);
 auto forwarder=publish("ERC2771Forwarder",forward_args);
 check(nft_read(forwarder,abi("nonces(address)",{addr(owner)}))==0,"forwarder starts with unused developer nonce");
 auto admin=publish("ProxyAdmin",words({addr(owner)}));check(nft_read(admin,abi("owner()",{}))==addr(owner),"proxy administrator assigned to developer");
 auto beacon=publish("UpgradeableBeacon",words({addr(receiver),addr(owner)}));
 check(nft_read(beacon,abi("implementation()",{}))==addr(receiver)&&nft_read(beacon,abi("owner()",{}))==addr(owner),"beacon stores implementation and owner");
 auto proxy=publish("ERC1967Proxy",words({addr(receiver),64,0}));
 auto beacon_proxy=publish("BeaconProxy",words({addr(beacon),64,0}));
 auto transparent=publish("TransparentUpgradeableProxy",words({addr(receiver),addr(owner),96,0}));
 auto implementation_mode=nft_read(receiver,abi("mode()",{}));
 for(const auto& destination:{proxy,beacon_proxy,transparent}){
  check(send(key,destination,abi("configure(uint256,address)",{42,addr(owner)})).success&&nft_read(destination,abi("mode()",{}))==42,"developer calls published proxy through delegatecall");
  check(nft_read(receiver,abi("mode()",{}))==implementation_mode,"proxy calls do not mutate implementation storage");
 }
 check(concrete.empty(),"every concrete upstream contract has an explicit deployment scenario");
 check(chain.audit().at("verified")==true&&peer.audit().at("verified")==true,"full signed history replays");final_root=chain.state().root();
 }
 {Blockchain reopened(root/"producer",genesis);check(reopened.state().root()==final_root&&read(reopened,"balanceOf(address)",{addr(recipient)})==141&&read(reopened,"allowance(address,address)",{addr(owner),addr(spender)})==0,"token balances and revoked approval survive RocksDB reopen");}
 std::cout<<checks<<" OpenZeppelin checks passed on chain "<<chain_id<<" with transaction type "<<tx_type<<"\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
