#pragma once
#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <stdexcept>
// Local authenticated export preparation, scoped to one peer session. This is
// never remote admission knowledge. Counts and bytes have independent bounds.
class CapellaExportCache {
public:
 struct Entry {std::string wire,parent;bool anchor_at_export=false;};
private:
 struct Saved {std::shared_ptr<const Entry> entry;size_t charge;};
 const size_t maximum_,budget_;size_t bytes_=0;
 std::map<std::string,Saved> entries_;std::deque<std::string> order_;
public:
 CapellaExportCache(size_t maximum,size_t budget):maximum_(maximum),budget_(budget){if(!maximum||!budget)throw std::invalid_argument("Invalid Capella export cache capacity");}
 std::shared_ptr<const Entry> find(const std::string& id)const{auto i=entries_.find(id);return i==entries_.end()?nullptr:i->second.entry;}
 void remember(const std::string& id,const std::string& wire,const std::string& parent,bool anchor_at_export=false){
  if(entries_.contains(id)||wire.size()>budget_||parent.size()>budget_||id.size()>budget_)return;
  auto entry=std::make_shared<const Entry>(Entry{wire,parent,anchor_at_export});
  // Charge string allocations and a fixed per-entry allowance. The independent
  // root bound also bounds map/deque overhead and retained shared entries.
  size_t charge=sizeof(Entry)+256;
  for(auto size:{entry->wire.capacity(),entry->parent.capacity(),id.size()}){if(size>budget_||charge>budget_-size)return;charge+=size;}
  while(!order_.empty()&&(entries_.size()>=maximum_||bytes_>budget_-charge)){auto oldest=entries_.find(order_.front());bytes_-=oldest->second.charge;entries_.erase(oldest);order_.pop_front();}
  order_.push_back(id);
  try{entries_.emplace(id,Saved{std::move(entry),charge});}catch(...){order_.pop_back();throw;}bytes_+=charge;
 }
 size_t retained()const{return entries_.size();}
 size_t charged_bytes()const{return bytes_;}
};
