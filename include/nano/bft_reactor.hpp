#pragma once
#include "consensus_round.hpp"
namespace nano {
// Serial reactor. RPC serializes calls; the relay carries only public messages.
class BftReactor {
 Blockchain& chain_;ChainStore journal_;Bytes key_;std::string signer_;
 std::unique_ptr<ConsensusRound> round_;
 Bytes parent_;std::vector<FinalityVote> outbound_;Json previous_=nullptr;
 void ensure_round(ConsensusRound::Clock::time_point now);
public:
 BftReactor(Blockchain& chain,const std::filesystem::path& journal,Bytes key);
 void tick(ConsensusRound::Clock::time_point now,const Int& wall);
 Json snapshot() const;
 void receive(const Json& message,ConsensusRound::Clock::time_point now,const Int& wall);
};
}
