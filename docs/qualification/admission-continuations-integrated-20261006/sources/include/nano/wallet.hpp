#pragma once
#include "core.hpp"
#include "crypto.hpp"
#include <filesystem>
#include <memory>
namespace nano {
class Wallet {
 Bytes key_;
public:
 explicit Wallet(Bytes key);
 static Wallet create();
 static Wallet from_keyfile(const std::filesystem::path& path,const std::string& password);
 std::string address() const;
 Bytes public_key() const;
 Json keyfile(const std::string& password) const;
 void save_keyfile(const std::filesystem::path& path,const std::string& password) const;
 SignedTransaction sign_transaction(const Transaction& tx) const;
 Bytes sign_typed_data(const Json& data) const;
 Bytes sign_message(const Bytes& message) const;
 void sign_block(Block& block,const Int& chain_id,const Bytes& genesis_hash) const;
};
class KeyStore {
 std::map<std::string,std::shared_ptr<Wallet>> wallets_;
 std::map<std::string,std::string> names_;
 std::vector<std::string> order_;
 std::string default_;
public:
 std::string add_wallet(Wallet wallet,const std::string& name="");
 Wallet create_wallet(const std::string& name="");
 std::shared_ptr<const Wallet> get_wallet(const std::string& address_or_name) const;
 std::shared_ptr<const Wallet> get_default() const;
 bool set_default(const std::string& address_or_name);
 std::vector<std::string> addresses() const;
 bool remove_wallet(const std::string& address_or_name);
};
int wallet_command(int argc,char** argv);
}
