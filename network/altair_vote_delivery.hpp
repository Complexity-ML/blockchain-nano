#pragma once
#include "altair_archive_relay.hpp"
#include <openssl/sha.h>

// Remember possession only after successful local admission. Capture the current
// session's object: a delayed completion must never seed a reconnected peer's
// fresh cache. This does not move the outbound inventory cursor.
inline AltairWork::Dispatch remember_message_sender(AltairWork::Dispatch dispatch,
 std::shared_ptr<AltairRelayProgress> knowledge,AltairWireKind kind){
 if(!dispatch||!knowledge||(kind!=AltairWireKind::attestation&&kind!=AltairWireKind::sync_committee_message))throw std::invalid_argument("invalid vote sender knowledge");
 return [dispatch=std::move(dispatch),knowledge=std::move(knowledge),kind](AltairWireMessage message,AltairWork::Cancellation cancel,AltairWork::Done done){
  std::string root;
  if(message.kind==kind){
   unsigned char digest[SHA256_DIGEST_LENGTH];
   SHA256(message.ssz.data(),message.ssz.size(),digest);
   constexpr char hex[]="0123456789abcdef";root="0x";root.reserve(66);
   for(auto byte:digest){root.push_back(hex[byte>>4]);root.push_back(hex[byte&15]);}
  }
  return dispatch(std::move(message),std::move(cancel),[knowledge,root=std::move(root),done=std::move(done)](AltairWork::Decision decision){
   if(decision==AltairWork::Decision::accepted&&!root.empty()){
    // Optional transport cache failure must not suppress admission completion.
    try{knowledge->acknowledge(root);}catch(const std::bad_alloc&){}
   }
   done(decision);
  });
 };
}

inline AltairWork::Dispatch remember_vote_sender(AltairWork::Dispatch dispatch,std::shared_ptr<AltairRelayProgress> knowledge){
 return remember_message_sender(std::move(dispatch),std::move(knowledge),AltairWireKind::attestation);
}
