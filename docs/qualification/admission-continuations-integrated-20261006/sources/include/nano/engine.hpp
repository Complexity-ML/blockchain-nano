#pragma once
#include "evm.hpp"
#include "receipt.hpp"
namespace nano {
Int next_excess_blob_gas(const Header& parent,EvmRevision revision);
void validate_execution_block_size(size_t encoded_size,EvmRevision revision);
enum class ReplayPolicy { RequireChainId, AllowLegacyUnprotected };
// External credits/roots require consensus validation; Nano rejects these inputs.
enum class ConsensusInputPolicy { RejectExternalInputs, EthereumValidated };
class ExecutionEngine {
 Int chain_id_;
 EvmRevision revision_;
 ReplayPolicy replay_policy_;
 ConsensusInputPolicy consensus_input_policy_;
 std::function<Bytes(const Int&)> block_hash_;
 std::function<void()> checkpoint_;
 Bytes system_call(State& state,const Header& header,const std::string& target,const Bytes& data,bool required=false) const;
public:
 explicit ExecutionEngine(Int chain_id,std::function<Bytes(const Int&)> block_hash={},EvmRevision revision=EvmRevision::NanoExperimental,ReplayPolicy replay_policy=ReplayPolicy::RequireChainId,ConsensusInputPolicy consensus_input_policy=ConsensusInputPolicy::RejectExternalInputs):chain_id_(std::move(chain_id)),revision_(revision),replay_policy_(replay_policy),consensus_input_policy_(consensus_input_policy),block_hash_(std::move(block_hash)){}
 // Host-only execution copy; the consensus engine keeps an empty callback.
 ExecutionEngine with_checkpoint(std::function<void()> checkpoint) const {auto copy=*this;copy.checkpoint_=std::move(checkpoint);return copy;}
 Receipt apply_transaction(State& state,const SignedTransaction& tx,const Header& block,const Int& index,const Int& cumulative_gas) const;
 // Executes a candidate block atomically, derives roots and assigns final receipt hashes.
 std::vector<Receipt> execute_block(State& state,Block& block,bool reward=true) const;
 // Replays an advertised block and rejects inconsistent commitments without changing state.
 std::vector<Receipt> verify_block(State& state,const Block& block,bool reward=true) const;
 std::vector<Receipt> verify_block(State& state,const Block& block,const Header& parent,bool reward=true) const;
};
}
