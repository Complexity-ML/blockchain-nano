#pragma once
#include <algorithm>
#include <cstddef>
#include <vector>
namespace nano::detail {
// Proposal policy only: validate must perform the complete transition without
// publishing state. Reserve enough trials to retry every item if a batch fails.
template<class Vote,class Check,class Validate>
std::vector<Vote> select_proposal_votes(const std::vector<Vote>& candidates,
 std::size_t capacity,std::size_t budget,Check check,Validate validate){
 std::vector<Vote> selected;std::size_t cursor=0,trials=0;
 while(cursor<candidates.size()&&selected.size()<capacity&&trials<budget){
  check();const auto count=std::min({candidates.size()-cursor,capacity-selected.size(),budget-trials});
  if(count>1&&count<budget-trials){
   auto batch=selected;batch.insert(batch.end(),candidates.begin()+cursor,candidates.begin()+cursor+count);
   ++trials;
   if(validate(batch)){selected=std::move(batch);cursor+=count;continue;}
  }
  for(std::size_t i=0;i<count&&trials<budget;++i){
   check();selected.push_back(candidates[cursor++]);++trials;
   if(!validate(selected))selected.pop_back();
  }
 }
 return selected;
}
}
