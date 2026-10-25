#include "nano/engine.hpp"
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

// Test-only derivative of tests/bridge_execution_runner.cpp. No production
// source, database, RPC server, P2P, validator identity or file is opened.
using namespace nano;
namespace {
constexpr std::size_t max_line_bytes = 2 * 1024 * 1024;
constexpr std::size_t max_requests = 5000;
const std::string kind = "nano-in-process-shanghai-v1";
const std::string coinbase = "0x0000000000000000000000000000000000009999";
void require(bool value, const char* message) { if (!value) throw std::invalid_argument(message); }
void fields(const Json& value,const std::set<std::string>& required,const std::set<std::string>& optional={}) {
    require(value.is_object(),"request must be an object");
    for (const auto& key:required) require(value.contains(key),"required field absent");
    for(auto it=value.begin();it!=value.end();++it)require(required.count(it.key())||optional.count(it.key()),"unknown request field");
}
std::string bounded_error(const std::exception& e) {
    std::string s=e.what();if(s.size()>240)s.resize(240);
    for(char& c:s){auto u=static_cast<unsigned char>(c);if(u<32||u>126)c='?';}return s;
}
bool valid_id(const Json& value) {
    if(value.is_string()){auto s=value.get<std::string>();if(s.empty()||s.size()>64)return false;for(unsigned char c:s)if(c<32||c>126)return false;return true;}
    if(!value.is_number_integer())return false;
    return value.is_number_unsigned()?value.get<uint64_t>()<=9007199254740991ULL:value.get<int64_t>()>=0&&value.get<int64_t>()<=9007199254740991LL;
}
Int uint256(const Json& value) {
    require(value.is_string(),"canonical integer string required");std::string s=value.get<std::string>();int base=10;
    if(s.size()>=2&&s.substr(0,2)=="0x"){base=16;s.erase(0,2);}
    require(!s.empty()&&s.size()<=(base==16?64U:78U),"uint256 encoding bound");
    for(unsigned char c:s)require(base==16?std::isxdigit(c)!=0:c>='0'&&c<='9',"invalid unsigned integer");
    Int out;require(mpz_set_str(out.get_mpz_t(),s.c_str(),base)==0&&out>=0&&out<(Int(1)<<256),"uint256 range");return out;
}
std::string input_address(const Json& value) {
    require(value.is_string(),"address string required");auto s=value.get<std::string>();require(s.size()==42&&s.substr(0,2)=="0x","20-byte address required");
    for(std::size_t i=2;i<s.size();++i)require(std::isxdigit(static_cast<unsigned char>(s[i]))!=0,"invalid address hex");return address(s);
}
Bytes input_data(const Json& value) {
    require(value.is_string(),"data string required");auto s=value.get<std::string>();require(s.size()>=2&&s.substr(0,2)=="0x"&&s.size()%2==0,"even 0x data required");
    for(std::size_t i=2;i<s.size();++i)require(std::isxdigit(static_cast<unsigned char>(s[i]))!=0,"invalid data hex");return unhex(s);
}
uint64_t input_gas(const Json& request) {
    Int n=request.contains("gas")?uint256(request.at("gas")):Int(30000000);
    require(n>0&&n<=Int(30000000),"gas outside 1..30000000");return u64(n);
}
Json logs_json(const std::vector<Log>& logs){Json a=Json::array();for(const auto& log:logs)a.push_back(log.to_json());return a;}
enum class Line { end,ready,oversized };
Line read_line(std::string& out) {
    out.clear();for(;;){int c=std::cin.get();if(c==std::char_traits<char>::eof()){if(std::cin.bad())throw std::runtime_error("stdin read failed");return out.empty()?Line::end:Line::ready;}
        if(c=='\n')return Line::ready;if(out.size()==max_line_bytes)return Line::oversized;out.push_back(static_cast<char>(c));}
}
class Runner {
    State state_;
    uint64_t step_=0;
    std::array<Bytes,3> keys_;
    std::array<std::string,3> senders_;
    const std::chrono::steady_clock::time_point end_=std::chrono::steady_clock::now()+std::chrono::seconds(540);
    void checkpoint() const {if(std::chrono::steady_clock::now()>=end_)throw ExecutionCancelled("test helper elapsed budget");}
    std::size_t signer(const Json& value) const {auto from=input_address(value);for(std::size_t i=0;i<senders_.size();++i)if(from==senders_[i])return i;throw std::invalid_argument("only public fixture signers allowed");}
    Header header(uint64_t step) const {Header h;h.coinbase=coinbase;h.base_fee=Int(0);h.difficulty=0;h.gas_limit=Int(60000000);
        h.number=Int(std::to_string(step));h.timestamp=Int(1700000000)+Int(12)*h.number;return h;}
    Json envelope() const {return {{"executionKind",kind},{"mined",false},{"syntheticContext",true},{"consensusValidated",false}};}
    Json send(const Json& request) {
        fields(request,{"id","chain","action","from","data"},{"to","value","gas"});checkpoint();auto i=signer(request.at("from"));
        Transaction tx;tx.type=0;tx.chain_id=31337;tx.nonce=static_cast<const State&>(state_).get(senders_[i]).nonce;
        tx.gas_price=1;tx.gas=Int(std::to_string(input_gas(request)));tx.value=request.contains("value")?uint256(request.at("value")):Int(0);tx.data=input_data(request.at("data"));
        if(request.contains("to")&&!request.at("to").is_null())tx.to=input_address(request.at("to"));
        const auto signed_tx=SignedTransaction::sign(tx,keys_[i]);
        const auto root_before=state_.root();const Int balance_before=static_cast<const State&>(state_).get(senders_[i]).balance;
        auto engine=ExecutionEngine(Int(31337),{},EvmRevision::Shanghai).with_checkpoint([this]{checkpoint();});
        const auto receipt=engine.apply_transaction(state_,signed_tx,header(step_+1),0,0);
        require(receipt.transaction_hash==signed_tx.hash(),"engine transaction hash mismatch");
        require(receipt.success||receipt.logs.empty(),"failed transaction retained logs");++step_;checkpoint();
        auto out=envelope();out["success"]=receipt.success;out["gasUsed"]=quantity(receipt.gas_used);out["effectiveGasPrice"]="0x1";
        out["transactionHash"]=hex(receipt.transaction_hash);out["rawSignedTransaction"]=hex(signed_tx.raw());
        out["from"]=senders_[i];out["to"]=tx.to?Json(*tx.to):Json(nullptr);out["value"]=quantity(tx.value);
        out["nonceBefore"]=quantity(tx.nonce);out["nonceAfter"]=quantity(static_cast<const State&>(state_).get(senders_[i]).nonce);
        out["senderBalanceBefore"]=quantity(balance_before);out["senderBalanceAfter"]=quantity(static_cast<const State&>(state_).get(senders_[i]).balance);
        out["stateRootBefore"]=hex(root_before);out["executionStateRoot"]=hex(state_.root());out["executionStep"]=quantity(Int(std::to_string(step_)));
        out["contractAddress"]=receipt.contract_address?Json(*receipt.contract_address):Json(nullptr);out["logs"]=logs_json(receipt.logs);
        // Receipt commitments for this one-transaction synthetic context only.
        out["receiptType"]=quantity(Int(receipt.type));
        out["cumulativeGasUsed"]=quantity(receipt.cumulative_gas_used);
        out["logsBloom"]=hex(nano::logs_bloom(receipt.logs));
        out["encodedReceipt"]=hex(receipt.encode());
        out["singletonReceiptsRoot"]=hex(receipts_root({receipt}));
        // receipt.block_hash is derived from our synthetic Header; NEVER expose
        // it or Receipt::to_json as a mined/consensus commitment.
        return out;
    }
    Json call(const Json& request) const {
        fields(request,{"id","chain","action","from","to","data"},{"value","gas"});checkpoint();auto i=signer(request.at("from"));auto target=input_address(request.at("to"));
        const Int value=request.contains("value")?uint256(request.at("value")):Int(0);const auto before=state_.root();auto copy=state_;
        require(copy.transfer(senders_[i],target,value),"insufficient simulated value");auto h=header(step_);
        ExecutionContext ctx;ctx.revision=EvmRevision::Shanghai;ctx.origin=senders_[i];ctx.caller=senders_[i];ctx.address=target;
        ctx.coinbase=coinbase;ctx.chain_id=31337;ctx.gas_price=1;ctx.base_fee=0;ctx.gas_limit=Int(60000000);ctx.gas=input_gas(request);
        ctx.number=h.number;ctx.timestamp=h.timestamp;ctx.prevrandao=block_randomness(h,EvmRevision::Shanghai);ctx.value=value;
        ctx.code=static_cast<const State&>(copy).get(target).code;ctx.data=input_data(request.at("data"));ctx.is_static=false;ctx.checkpoint=[this]{checkpoint();};
        const auto result=EVM(copy).execute(ctx);require(state_.root()==before,"simulation changed retained State");checkpoint();
        auto out=envelope();out["success"]=result.success;out["gasUsed"]=quantity(Int(std::to_string(result.gas_used)));out["returnData"]=hex(result.return_data);
        out["error"]=result.error.substr(0,240);out["stateDiscarded"]=true;out["executionStateRoot"]=hex(before);out["logs"]=result.success?logs_json(result.logs):Json::array();return out;
    }
public:
    Runner(){for(std::size_t i=0;i<keys_.size();++i){keys_[i]=bytes(Int(std::to_string(i+1)),32);senders_[i]=address(public_address(public_key(keys_[i])));state_.get(senders_[i]).balance=Int("1000000000000000000000000");}}
    Json handle(const Json& request) {
        checkpoint();require(request.is_object()&&request.contains("id")&&valid_id(request.at("id")),"bounded scalar id required");
        require(request.contains("chain")&&request.at("chain").is_number_integer()&&request.at("chain")==0,"test chain slot must be zero");
        require(request.contains("action")&&request.at("action").is_string(),"action string required");auto action=request.at("action").get<std::string>();
        if(action=="send")return send(request);if(action=="call")return call(request);
        auto out=envelope();out["executionStateRoot"]=hex(state_.root());out["executionStep"]=quantity(Int(std::to_string(step_)));
        if(action=="account"){
            fields(request,{"id","chain","action","address"});auto addr=input_address(request.at("address"));const auto& account=static_cast<const State&>(state_).get(addr);
            require(account.storage.size()<=1024&&account.code.size()<=256*1024,"bounded readonly account diagnostic");
            Json storage=Json::object();for(const auto& [slot,value]:account.storage){checkpoint();storage[quantity(slot)]=quantity(value);}
            checkpoint();const auto storage_root=account.storage_root();checkpoint();
            out["storageRoot"]=hex(storage_root);out["storage"]=storage;
            out["address"]=addr;out["balance"]=quantity(account.balance);out["nonce"]=quantity(account.nonce);out["code"]=hex(account.code);out["codeHash"]=hex(keccak(account.code));out["exists"]=state_.exists(addr);return out;
        }
        fields(request,{"id","chain","action"});require(action=="root"||action=="metadata","unknown test action");out["chainId"]="0x7a69";
        if(action=="metadata"){out["evmRevision"]="Shanghai";out["signers"]=senders_;out["coinbase"]=coinbase;out["gasPrice"]="0x1";out["baseFee"]="0x0";out["transactionGasLimit"]="0x1c9c380";out["contextGasLimit"]="0x3938700";}
        return out;
    }
};
}
int main(int argc,char**) {
    if(argc!=1){std::cerr<<"inventory_v2_execution_runner accepts no arguments\n";return 2;}
    try{Runner runner;bool malformed=false;std::size_t count=0;std::string line;
        for(;;){auto status=read_line(line);if(status==Line::end)break;
            if(status==Line::oversized||count==max_requests){std::cout<<Json{{"id",nullptr},{"ok",false},{"error",status==Line::oversized?"input exceeds 2 MiB":"request count exceeds 5000"}}.dump()<<'\n'<<std::flush;return 1;}
            ++count;Json id=nullptr;
            try{auto request=Json::parse(line);if(request.is_object()&&request.contains("id")&&valid_id(request.at("id")))id=request.at("id");
                auto result=runner.handle(request);std::cout<<Json{{"id",id},{"ok",true},{"result",result}}.dump()<<'\n'<<std::flush;
            }catch(const std::exception& e){malformed=true;std::cout<<Json{{"id",id},{"ok",false},{"error",bounded_error(e)}}.dump()<<'\n'<<std::flush;}
        }return malformed?1:0;
    }catch(const std::exception& e){std::cerr<<bounded_error(e)<<'\n';return 1;}
}
