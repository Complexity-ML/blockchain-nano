#include "nano/chain.hpp"
#include "nano/migration.hpp"
#include "nano/http.hpp"
#include "nano/client.hpp"
#include "nano/node.hpp"
#include "nano/wallet.hpp"
#include "nano/beacon_journal_cli.hpp"
#include <fstream>
#include <iostream>
using namespace nano;
Json load(const std::string& path){std::ifstream input(path);if(!input)throw std::runtime_error("cannot open "+path);Json data;input>>data;return data;}
int main(int argc,char** argv){try{
 if(argc<2||std::string(argv[1])=="--help"){
  std::cout<<"Nano local chain tools\nUsage: nano <init|status|audit|mine|validate|import|serve|run> <genesis.json> <database-directory> [arguments]\n"
    "  wallet <create|import|address|sign-message|sign-transaction|sign-typed-data> <keyfile> [data]\n  wallet deployment-address <keyfile> [nonce]\n  wallet create-many <new-directory> [count] (default: 10)\n  wallet hd-create <mnemonic-file> [word-count]; wallet hd-derive <keyfile> [index]\n  rpc <URL> <info|nodes|network|balance|send|deploy|receipt|mine> [arguments]\n  rpc <URL> <stake-register|stake-add> <keyfile> <contract> <amountNano>\n  rpc <URL> stake-withdraw <keyfile> <contract>\n  rpc <URL> deploy <keyfile> <bytecode-file> [timeout] [valueNano]\n  validator <config.json> [--once] (separate client)\n  init/status: show canonical chain status\n  mine/validate: <coinbase> <timestamp> [signed-raw-transactions.json]\n"
    "  beacon-journal init <identity.json> <new-database> <new-receipt-directory>\n"
    "  beacon-journal status <identity.json> <database> <expected-commitment.json>\n"
    "  beacon-journal export <identity.json> <database> <expected-commitment.json> <new-receipt-directory>\n"
    "  beacon-journal import <identity.json> <database> <expected-commitment.json> <interchange.json> <max-bytes> <new-receipt-directory>\n"
    "  migrate: <genesis.json> <python-chaindb-directory> <new-database-directory>\n  import: <block.json> <current-timestamp>\n  run: [node-config.json] (RPC and P2P together)\n  serve: [port] [bind-address] [wallet-config.json] (defaults: 8545, 127.0.0.1)\n";return argc<2?1:0;
 }
 std::string command=argv[1];
 if(command=="beacon-journal")return beacon_journal_command(argc,argv);
 if(command=="audit"){
  if(argc!=4)throw std::invalid_argument("audit requires genesis.json existing-database-directory");
  if(!std::filesystem::is_regular_file(std::filesystem::path(argv[3])/"CURRENT"))throw std::invalid_argument("audit requires an existing RocksDB database");
  Blockchain chain(argv[3],Genesis::from_json(load(argv[2])));std::cout<<chain.audit().dump(2)<<"\n";return 0;
 }
 if(command=="migrate"){if(argc!=5)throw std::invalid_argument("migrate requires genesis.json source-directory new-database-directory");std::cout<<migrate_chaindb(Genesis::from_json(load(argv[2])),argv[3],argv[4]).dump(2)<<"\n";return 0;}
 if(command=="rpc")return rpc_command(argc,argv);
 if(command=="wallet")return wallet_command(argc,argv);
 if(command=="validator"){if(argc<3||argc>4||(argc==4&&std::string(argv[3])!="--once"))throw std::invalid_argument("validator requires config.json [--once]");return run_validator(load(argv[2]),argc==4);}
 if((command!="init"&&command!="status"&&command!="mine"&&command!="validate"&&command!="import"&&command!="serve"&&command!="run")||argc<4)throw std::invalid_argument("invalid arguments; use --help");
 if((command=="run"&&(argc<4||argc>5))||(command=="serve"&&(argc<4||argc>7))||((command=="mine"||command=="validate")&&(argc<6||argc>7))||(command=="import"&&argc!=6)||((command=="init"||command=="status")&&argc!=4))throw std::invalid_argument("invalid arguments; use --help");
 auto genesis=Genesis::from_json(load(argv[2]));Blockchain chain(argv[3],std::move(genesis));
 if(command=="run")return run_node(chain,argv[3],argv[0],argc==5?load(argv[4]):Json::object());
 if(command=="serve"){
  auto port=argc>=5?u64(quantity(Json(argv[4]))):8545;if(port>65535)throw std::invalid_argument("invalid port");Rpc rpc(chain);std::string bind=argc>=6?argv[5]:"127.0.0.1";if(argc==7)rpc.load_wallets(load(argv[6]),bind);HttpServer server(rpc,bind,uint16_t(port));std::cout<<"Nano JSON-RPC listening on "<<bind<<":"<<server.port()<<std::endl;server.run();
 }else if(command=="mine"||command=="validate"){
  if(argc==7){auto txs=load(argv[6]);if(!txs.is_array())throw std::invalid_argument("expected an array of signed transaction hex strings");for(const auto& raw:txs)if(!chain.submit(SignedTransaction::decode(unhex(raw.get<std::string>()))))throw std::invalid_argument("transaction rejected by pool");}
  Block block;
  if(command=="validate"&&chain.genesis().pos_signature_version){
   const char* secret=std::getenv("VALIDATOR_KEY");if(!secret||!*secret)throw std::invalid_argument("VALIDATOR_KEY required for signed PoS production");
   auto key=unhex(secret);if(address(public_address(public_key(key)))!=address(argv[4]))throw std::invalid_argument("validator key does not match producer address");
   block=chain.validate_signed(key,quantity(Json(argv[5])));
  }else block=command=="mine"?chain.mine(argv[4],quantity(Json(argv[5]))):chain.validate(argv[4],quantity(Json(argv[5])));
  std::cout<<block.to_json(true).dump(2)<<"\n";
 }else if(command=="import"){
  chain.import_block(Block::from_json(load(argv[4])),quantity(Json(argv[5])));std::cout<<chain.tip().to_json(true).dump(2)<<"\n";
 }else{
  Json status={{"chainId",quantity(chain.genesis().chain_id)},{"consensus",chain.genesis().consensus},{"blockNumber",quantity(chain.tip().header.number)},{"blockHash",hex(chain.tip().header.hash())},{"stateRoot",hex(chain.state().root())},{"totalDifficulty",quantity(chain.store().total_difficulty())}};
  std::cout<<status.dump(2)<<"\n";
 }
 return 0;
}catch(const std::exception& e){std::cerr<<"Nano: "<<e.what()<<"\n";return 1;}}
