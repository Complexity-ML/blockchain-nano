#pragma once
#include "chain.hpp"
namespace nano {
// Replay a stopped Python ChainDB into a new RocksDB directory.
// Source files are read-only; the destination appears only after full verification.
Json migrate_chaindb(const Genesis& genesis,const std::filesystem::path& source,const std::filesystem::path& destination);
}
