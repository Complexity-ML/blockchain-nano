#pragma once
#include "consensus.hpp"
#include "evm.hpp"
#include "staking_rules.hpp"
namespace nano {
Int staking_contract_stake(const State& state,const Header& header,const std::string& contract,const std::string& account,const Int& chain_id=1337,std::function<Bytes(const Int&)> block_hash={},EvmRevision revision=EvmRevision::NanoExperimental,StakingQueryLimits limits={});
ProofOfStake sync_staking_contract(const State& state,const Header& header,const std::string& contract,const ProofOfStake& previous,const Int& chain_id=1337,std::function<Bytes(const Int&)> block_hash={},EvmRevision revision=EvmRevision::NanoExperimental,StakingQueryLimits limits={});
}
