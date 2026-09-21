#pragma once
#include "evm.hpp"
#include "receipt.hpp"
namespace nano {
enum class ReplayPolicy { RequireChainId, AllowLegacyUnprotected };
class ExecutionEngine {
 Int chain_id_;
 EvmRevision revision_;
 ReplayPolicy replay_policy_;
 std::function<Bytes(const Int&)> block_hash_;
public:
 explicit ExecutionEngine(Int chain_id,std::function<Bytes(const Int&)> block_hash={},EvmRevision revision=EvmRevision::NanoExperimental,ReplayPolicy replay_policy=ReplayPolicy::RequireChainId):chain_id_(std::move(chain_id)),revision_(revision),replay_policy_(replay_policy),block_hash_(std::move(block_hash)){}
 Receipt apply_transaction(State& state,const SignedTransaction& tx,const Header& block,const Int& index,const Int& cumulative_gas) const;
 // Executes a candidate block atomically, derives roots and assigns final receipt hashes.
 std::vector<Receipt> execute_block(State& state,Block& block,bool reward=true) const;
 // Replays an advertised block and rejects inconsistent commitments without changing state.
 std::vector<Receipt> verify_block(State& state,const Block& block,bool reward=true) const;
};
}
