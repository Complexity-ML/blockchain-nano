#pragma once
#include "crypto.hpp"
#include "rlp.hpp"
namespace nano {
struct Access { std::string address; std::vector<Bytes> keys; };
struct Transaction {
 unsigned type=0;
 Int nonce=0,value=0,chain_id=1337,gas=21000,gas_price=1000000000,max_fee=0,priority_fee=0;
 std::optional<std::string> to;
 Bytes data;
 std::vector<Access> access_list;
 Rlp payload() const;
 Bytes signing_hash(bool protected_legacy=true) const;
 Int intrinsic_gas() const;
 Int effective_gas_price(const Int& base_fee) const;
 Json to_json() const;
 static Transaction from_json(const Json& j);
};
struct SignedTransaction {
 Transaction tx;
 Int v=0,r=0,s=0;
 Bytes raw() const;
 Bytes hash() const {return keccak(raw());}
 std::string sender() const;
 Json to_json() const;
 static SignedTransaction sign(Transaction tx,const Bytes& key);
 static SignedTransaction decode(const Bytes& raw);
};
std::string create_address(const std::string& sender,const Int& nonce);
std::string create2_address(const std::string& sender,const Bytes& salt,const Bytes& init_hash);
struct Account {
 Int nonce=0,balance=0;
 Bytes code;
 std::map<Int,Int> storage;
 bool empty() const {return nonce==0&&balance==0&&code.empty();}
 Bytes storage_root() const;
 Bytes encode() const;
 Json to_json() const;
};
class State {
 std::map<std::string,Account> accounts_;
 std::vector<std::map<std::string,Account>> snapshots_;
public:
 const Account& get(const std::string& addr) const;
 Account& get(const std::string& addr);
 bool exists(const std::string& addr) const;
 void erase(const std::string& addr);
 bool transfer(const std::string& from,const std::string& to,const Int& amount);
 void set_storage(const std::string& addr,const Int& key,const Int& value);
 Int get_storage(const std::string& addr,const Int& key) const;
 Bytes root() const;
 size_t snapshot();
 void revert(size_t id);
 void commit(size_t id);
 Json to_json() const;
 static State from_json(const Json& j);
};
struct Log {
 std::string address;
 std::vector<Bytes> topics;
 Bytes data;
 Json to_json() const;
};
Bytes logs_bloom(const std::vector<Log>& logs);
struct Header {
 Bytes parent_hash=Bytes(32),uncle_hash=keccak(Rlp(std::vector<Rlp>{}).encode());
 std::string coinbase="0x0000000000000000000000000000000000000000";
 Bytes state_root=keccak({}),transactions_root=keccak(Rlp(std::vector<Rlp>{}).encode()),receipts_root=transactions_root,logs_bloom=Bytes(256);
 Int difficulty=1,number=0,gas_limit=30000000,gas_used=0,timestamp=0;
 Bytes extra_data,mix_hash=Bytes(32),nonce=Bytes(8);
 std::optional<Int> base_fee=Int(1000000000);
 std::optional<Bytes> withdrawals_root;
 Bytes encode() const;
 Bytes hash() const {return keccak(encode());}
 Json to_json() const;
 static Header from_json(const Json& j);
};
struct Block {
 Header header;
 Bytes producer_signature;
 std::vector<SignedTransaction> transactions;
 std::vector<Header> uncles;
 Json withdrawals=Json::array();
 Json to_json(bool full=false) const;
 static Block from_json(const Json& j);
};
Bytes transactions_root(const std::vector<SignedTransaction>& txs);
Int next_base_fee(const Header& parent);
}
