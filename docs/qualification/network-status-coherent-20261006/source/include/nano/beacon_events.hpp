#pragma once
#include "nano/types.hpp"
#include <deque>
#include <functional>
namespace nano {
// Ephemeral, bounded publication history. A lost cursor requires RPC resync;
// this is not a durable event log or a consensus parameter.
class BeaconEventJournal {
 struct Entry {uint64_t sequence;Json event;};
 std::deque<Entry> entries_;
 uint64_t sequence_=0;
public:
 static constexpr size_t capacity=256;
 void append(std::string topic,Json data){
  if(sequence_==UINT64_MAX)throw std::overflow_error("Beacon event sequence exhausted");
  entries_.push_back({sequence_+1,{{"topic",std::move(topic)},{"data",std::move(data)}}});
  ++sequence_;if(entries_.size()>capacity)entries_.pop_front();
 }
 Json read(std::optional<uint64_t> cursor)const{
  if(cursor&&(*cursor>sequence_||(!entries_.empty()&&*cursor<entries_.front().sequence-1)))
   throw std::runtime_error("Beacon event cursor lost; resynchronize via RPC");
  Json events=Json::array();if(cursor)for(const auto& entry:entries_)if(entry.sequence>*cursor)events.push_back(entry.event);
  return {{"cursor",sequence_},{"events",std::move(events)}};
 }
};
using BeaconEventsHandler=std::function<Json(std::optional<uint64_t>)>;
}
