#pragma once
#include <set>
#include <string>
#include <stdexcept>

// Confined to the network I/O executor. One attempt per peer, bounded globally.
// A disconnected peer retains its permit until completion/timeout: a late reply
// cannot release a permit belonging to a newer attempt for the same identity.
class PeerRelayBudget {
 size_t maximum_;std::set<std::string> active_;
public:
 explicit PeerRelayBudget(size_t maximum):maximum_(maximum){if(!maximum||maximum>1024)throw std::invalid_argument("Relay concurrency outside 1..1024");}
 bool available()const{return active_.size()<maximum_;}
 bool busy(const std::string& peer)const{return active_.contains(peer);}
 bool acquire(const std::string& peer){return !peer.empty()&&available()&&active_.insert(peer).second;}
 void release(const std::string& peer){active_.erase(peer);}
};
