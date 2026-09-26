#include "../src/proposal_vote_selection.hpp"
#include <iostream>
#include <stdexcept>
using nano::detail::select_proposal_votes;
void require(bool b){if(!b)throw std::runtime_error("proposal selection invariant");}
int main(){try{
 const std::vector<int> votes{1,2,3,4,5,6,7,8};std::size_t calls=0;
 auto selected=select_proposal_votes(votes,8,9,[]{},[&](const auto&){++calls;return true;});require(selected==votes&&calls==1);
 calls=0;selected=select_proposal_votes(votes,8,9,[]{},[&](const auto& v){++calls;return std::find(v.begin(),v.end(),3)==v.end();});require(selected==std::vector<int>({1,2,4,5,6,7,8})&&calls==9);
 calls=0;selected=select_proposal_votes(votes,3,9,[]{},[&](const auto& v){++calls;return std::find(v.begin(),v.end(),1)==v.end();});require(selected==std::vector<int>({2,3,4})&&calls<=9);
 for(std::size_t budget=0;budget<=10;++budget){calls=0;selected=select_proposal_votes(votes,8,budget,[]{},[&](const auto&){++calls;return false;});require(selected.empty()&&calls<=budget);}
 calls=0;selected=select_proposal_votes(votes,8,8,[]{},[&](const auto&){++calls;return true;});require(selected==votes&&calls==8);
 // Cancellation must escape; no apparently successful partial selection.
 bool cancelled=false;try{select_proposal_votes(votes,8,9,[]{},[](const auto&)->bool{throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancel";}require(cancelled);
 std::cout<<"Batch success, invalid-item fallback, capacity refill, strict budgets and cancellation passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
