#pragma once
#include "consensus_round.hpp"
#include <set>
namespace nano {
// Serial reactor. RPC serializes calls; the relay carries only public messages.
class BftReactor {
 Blockchain& chain_;ChainStore journal_;Bytes key_;std::string signer_;
 std::unique_ptr<ConsensusRound> round_;
 Bytes parent_;std::vector<std::string> outbound_;std::set<std::pair<unsigned,std::string>> outbound_slots_;Json previous_=nullptr;std::optional<Block> previous_proposal_;
 void remember_vote(const FinalityVote& vote);
 void ensure_round(ConsensusRound::Clock::time_point now);
public:
 BftReactor(Blockchain& chain,const std::filesystem::path& journal,Bytes key);
 void tick(ConsensusRound::Clock::time_point now,const Int& wall);
 Json snapshot() const;
 Json snapshot_page(bool previous=false,bool hashes=false) const;
 Json proposal_fragment(const Bytes& block_hash,const Bytes& transaction_hash,uint64_t offset) const;
 void validate_manifest(const Json& message,ConsensusRound::Clock::time_point now);
 void receive(const Json& message,ConsensusRound::Clock::time_point now,const Int& wall);
};
}
