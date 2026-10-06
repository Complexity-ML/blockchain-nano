#pragma once
#include "chain.hpp"
namespace nano {
int run_node(Blockchain& chain,const std::filesystem::path& database,const std::filesystem::path& executable,const Json& config);
}
