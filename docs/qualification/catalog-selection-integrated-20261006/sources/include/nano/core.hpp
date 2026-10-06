#pragma once
#include "crypto.hpp"
#include "rlp.hpp"
namespace nano {
struct Access { std::string address; std::vector<Bytes> keys; };
struct Authorization {
 Int chain_id=0,nonce=0,y_parity=0,r=0,s=0;
 std::string target="0x0000000000000000000000000000000000000000";
 Rlp payload() const;
 Bytes signing_hash() const;
 std::string authority() const;
 Json to_json() const;
 static Authorization from_json(const Json& j);
};
std::optional<std::string> delegation_target(const Bytes& code);
struct Transaction {
 unsigned type=0;
 Int nonce=0,value=0,chain_id=1337,gas=21000,gas_price=1000000000,max_fee=0,priority_fee=0;
 std::optional<std::string> to;
 Bytes data;
 std::vector<Access> access_list;
 std::vector<Authorization> authorization_list;
 Int max_fee_per_blob_gas=0;
 std::vector<Bytes> blob_versioned_hashes;
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
 // Read-only enumeration; callers must not retain the view across mutation.
 const std::map<std::string,Account>& accounts_view() const {return accounts_;}
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
Bytes deposit_requests(const std::vector<Log>& logs);
Bytes execution_requests_hash(const std::vector<Bytes>& requests);
struct Header {
 Bytes parent_hash=Bytes(32),uncle_hash=keccak(Rlp(std::vector<Rlp>{}).encode());
 std::string coinbase="0x0000000000000000000000000000000000000000";
 Bytes state_root=keccak({}),transactions_root=keccak(Rlp(std::vector<Rlp>{}).encode()),receipts_root=transactions_root,logs_bloom=Bytes(256);
 Int difficulty=1,number=0,gas_limit=30000000,gas_used=0,timestamp=0;
 Bytes extra_data,mix_hash=Bytes(32),nonce=Bytes(8);
 std::optional<Int> base_fee=Int(1000000000);
 std::optional<Bytes> withdrawals_root,parent_beacon_block_root,requests_hash;
 std::optional<Int> blob_gas_used,excess_blob_gas;
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
 Json to_transport_json() const;
 static Block from_json(const Json& j);
};
size_t execution_rlp_size(const Block& block);
Bytes transactions_root(const std::vector<SignedTransaction>& txs);
Rlp withdrawal_payload(const Json& withdrawal);
Bytes withdrawals_root(const Json& withdrawals);
Int next_base_fee(const Header& parent);
// Same-fork execution fields only; consensus seals and fork activation are separate.
void validate_execution_header(const Header& header,const Header& parent,const Int& minimum_gas_limit=Int(5000));
}
