#pragma once
#include "evm.hpp"
namespace nano {
ExecutionResult precompile(unsigned number,const Bytes& input,uint64_t gas);
}
