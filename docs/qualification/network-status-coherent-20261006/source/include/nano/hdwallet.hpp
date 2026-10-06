#pragma once
#include "wallet.hpp"
namespace nano {
std::string mnemonic_from_entropy(const Bytes& entropy);
Bytes mnemonic_entropy(const std::string& mnemonic);
Bytes mnemonic_seed(const std::string& mnemonic,const std::string& passphrase="");
Bytes derive_private_key(const Bytes& seed,const std::string& path);
class HDWallet {
 std::string mnemonic_;
 Bytes seed_;
public:
 explicit HDWallet(const std::string& mnemonic,const std::string& passphrase="");
 static HDWallet create(unsigned words=12,const std::string& passphrase="");
 const std::string& mnemonic() const {return mnemonic_;}
 Wallet derive_account(uint32_t index=0) const;
 std::vector<std::string> addresses(size_t count=10) const;
};
}
