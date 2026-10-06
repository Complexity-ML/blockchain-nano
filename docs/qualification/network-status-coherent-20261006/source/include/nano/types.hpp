#pragma once
#include <gmpxx.h>
#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "json.hpp"
namespace nano {
using Bytes = std::vector<uint8_t>;
using Int = mpz_class;
using Json = nlohmann::json;
Bytes unhex(std::string s);
std::string hex(const Bytes& b);
Bytes bytes(const Int& n, size_t width = 0);
Int integer(const Bytes& b);
Int quantity(const Json& j);
std::string quantity(const Int& n);
std::string address(std::string s);
Bytes concat(Bytes a, const Bytes& b);
inline Int word(const Int& n) { Int r; mpz_fdiv_r_2exp(r.get_mpz_t(), n.get_mpz_t(), 256); return r; }
inline Int signed_word(const Int& n) { return mpz_tstbit(n.get_mpz_t(),255) ? Int(n-(Int(1)<<256)) : n; }
uint64_t u64(const Int& n);
Bytes keccak(const Bytes& input);
Bytes sha256(const Bytes& input);
std::string checksum_address(const std::string& input);
}
