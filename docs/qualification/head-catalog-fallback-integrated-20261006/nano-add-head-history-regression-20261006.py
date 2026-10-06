from pathlib import Path
p=Path('/Users/boris/Dev/blockchain-nano/network/test_capella_head_catalog_relay.cpp');s=p.read_text();at=s.index(' std::cout<<cases<<')
addition=r'''
 // A valid independent branch supplies context required by the preferred head.
 // A refusal must not prevent its catalog delivery or invent a head ACK.
 for(bool omit_head:{false,true}){
  asio::io_context io;InboundBudget budget;auto ingress=std::make_shared<AltairMessageIngress>(p,budget);auto knowledge=std::make_shared<AltairRelayProgress>(8);
  unsigned done=0,pages=0,head_sends=0,historical_sends=0;Json result;
  auto frame=[&](unsigned n){std::vector<uint8_t> b(404,0);b[0]=100;b[180]=84;b[100]=n;const auto parent=n==4?0:n-1;std::fill(b.begin()+116,b.begin()+148,uint8_t(parent*17));return Json{{"chainId",p.chain_id},{"genesisHash",p.genesis_hash},{"forkDigest",p.fork_digest},{"consensusIdentity",p.consensus_identity},{"kind","block"},{"ssz",encode(b)}};};
  auto core=[&](std::string method,Json args,Reply reply){Json v;
   if(method=="nano_capellaStatus")v={{"headRoot",root('3')},{"headSlot",uint64_t(3)}};
   else if(method=="nano_capellaExport"){const auto id=args[2].get<std::string>();const unsigned n=id[2]-'0';v={{"root",id},{"available",true},{"parentRoot",root(char('0'+(n==4?0:n-1)))},{"envelope",frame(n)}};}
   else if(method=="nano_capellaCatalogGeneration")v=root('e');
   else if(method=="nano_capellaCatalogAcquire")v={{"handle",root('c')}};
   else if(method=="nano_capellaCatalogPage"){++pages;v={{"kind","block"},{"sequence","0x1"},{"digest",root('d')},{"entries",omit_head?Json::array({root('1'),root('2'),root('4')}):Json::array({root('1'),root('2'),root('3'),root('4')})},{"next",nullptr}};}
   else if(method=="nano_capellaCatalogRelease")v=true;else throw std::runtime_error("unexpected history-context method");reply(v);
  };
  auto send=[&](Json f,Reply reply){++head_sends;const auto n=std::stoul(f["ssz"].get<std::string>().substr(202,2),nullptr,16);reply(Json{{"accepted",n!=3||(!omit_head&&knowledge->acknowledged(root('4')))}});};
  auto deliver=[&](std::string id,Json,Reply reply){++historical_sends;const bool accepted=id!=root('3')||knowledge->acknowledged(root('4'));if(accepted)knowledge->acknowledge(id);reply(Json{{"accepted",accepted}});};
  for(unsigned attempt=0;attempt<2;++attempt){
   auto q=std::make_shared<CapellaDeliveryQuantum>(CapellaDeliveryQuantum{8});auto relay=std::make_shared<CapellaHeadCatalogRelay>(io,core,deliver,send,"token",root('0'),ingress,knowledge,4,8192,4,8,[&](Json r){++done;result=std::move(r);},[&](const std::string& id){return knowledge->acknowledged(id);},q,true);relay->start(100ms);io.restart();io.run();
   if(attempt==0)check(result["success"]==false&&!knowledge->acknowledged(root('3'))&&knowledge->acknowledged(root('4'))&&result["headPriority"]=="head-unconfirmed-catalog-fallback","unconfirmed head starved its independent context or gained a fake ACK");
  }
  if(omit_head)check(done==2&&result["success"]==false&&result["reason"]=="head-admission-unconfirmed"&&!knowledge->acknowledged(root('3')),"successful incomplete catalog faked head admission");
  else check(done==2&&result["success"]==true&&knowledge->acknowledged(root('3'))&&head_sends==4&&historical_sends==2&&pages==2,"independent context did not enable actual head ACK on next attempt");
  ++cases;
 }
'''
s=s[:at]+addition+s[at:];p.write_text(s)
print('Added two-round history-context and absent-head false-success guards')
