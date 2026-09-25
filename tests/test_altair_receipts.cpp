#include "../network/altair_receipts.hpp"
#include <iostream>
using D=AltairWork::Decision;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(){try{
 AltairReceipts ledger(2);const auto session=ledger.session();unsigned calls=0;
 AltairWork::Done first,second;AltairWork::Cancellation cancel;
 AltairWork::Dispatch dispatch=[&](auto,auto flag,auto done){++calls;if(calls==1){first=std::move(done);cancel=flag;}else second=std::move(done);return true;};
 auto message=[](uint8_t byte=1){return AltairWireMessage{AltairWireKind::block,{byte}};};
 check(ledger.submit(session,1,message(),dispatch).at("state")=="pending","first pending");
 check(ledger.submit(session,1,message(),dispatch).at("state")=="pending"&&calls==1,"retry dispatched twice");
 bool conflict=false;try{ledger.submit(session,1,message(2),dispatch);}catch(const std::invalid_argument&){conflict=true;}check(conflict&&calls==1,"ID content conflict accepted");
 check(ledger.submit(session,2,message(),dispatch).at("state")=="pending","second pending");
 check(ledger.submit(session,3,message(),dispatch).at("state")=="limited"&&calls==2,"ledger exceeded capacity");
 bool active=false;try{ledger.get(session,1,false,true);}catch(const std::invalid_argument&){active=true;}check(active,"running receipt retired");
 check(ledger.get(session,1,true).at("cancelRequested")==true&&cancel->load(),"cancel not signalled");
 first(D::accepted);first(D::rejected);
 check(ledger.get(session,1).at("decision")=="accepted","late cancellation or duplicate completion rewrote decision");
 check(ledger.submit(session,1,message(),dispatch).at("decision")=="accepted"&&calls==2,"lost completion response replayed work");
 ledger.get(session,1,false,true);
 check(ledger.submit(session,1,message(),dispatch).at("state")=="retired"&&calls==2,"retired sequence replayed");
 auto rejected=[](auto,auto,auto){return false;};
 check(ledger.submit(session,3,message(),rejected).at("state")=="limited"&&ledger.list().at("highWater")=="0x2","queue refusal consumed ticket");
 auto immediate=[](auto,auto,auto done){done(D::rejected);return true;};
 check(ledger.submit(session,3,message(),immediate).at("decision")=="rejected","inline completion deadlock/loss");
 second(D::local_failure);check(ledger.get(session,2).at("decision")=="local_failure","local failure disguised as rejection");
 check(ledger.list().at("jobs").size()==2,"completed receipts evicted automatically");
 check(ledger.get(session,4,true).at("state")=="retired","cancel did not fence delayed admission");
 check(ledger.submit(session,4,message(),dispatch).at("state")=="retired"&&calls==2,"delayed submit ran after cancellation");
 bool stale=false;try{ledger.get("wrong",2);}catch(const std::invalid_argument&){stale=true;}check(stale,"foreign session accepted");
 AltairReceipts restarted(2);check(restarted.session()!=session,"session reused after restart");
 bool old=false;try{restarted.submit(session,1,message(),immediate);}catch(const std::invalid_argument&){old=true;}check(old,"old session replayed after restart");
 std::cout<<"{\"scenario\":\"altair-receipt-ledger\",\"bounded\":true,\"lostReplyDeduplicated\":true,\"restartOutcomeUnknown\":true}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
