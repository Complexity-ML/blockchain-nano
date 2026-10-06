from pathlib import Path
repo=Path('/Users/boris/Dev/blockchain-nano');p=repo/'network/capella_catalog_relay.hpp';s=p.read_text();s=s.replace('if(rejected&&s.kind_=="attestation"){','if(rejected&&(s.kind_=="attestation"||s.kind_=="block")){');s=s.replace('// Independent votes can be expired or reference a different branch. A','// Blocks may need incremental context or another branch; votes can expire. A');p.write_text(s)
p=repo/'network/capella_head_catalog_relay.hpp';s=p.read_text().replace('std::string priority_="unselected";','std::string priority_="unselected",selected_head_;')
s=s.replace('void finish(Json result){if(finished_)return;','void finish(Json result){if(finished_)return;if(result.value("success",false)&&!selected_head_.empty()&&selected_head_!=anchor_&&!knowledge_->acknowledged(selected_head_)){result["success"]=false;if(!result.contains("reason")||result["reason"].is_null()){result["reason"]="head-admission-unconfirmed";result["failureStage"]="head";}}',1)
s=s.replace('if(id==anchor_){priority_="anchor";','selected_head_=id;\n  if(id==anchor_){priority_="anchor";',1)
s=s.replace('if(ack!=Json{{"accepted",true}}){self->fail("head-admission-unconfirmed");return;}', '''if(ack==Json{{"accepted",false}}){
    // A rejected/incomplete head is never an ACK. Independent historical
    // branches may supply its missing context; preserve the same deadline,
    // admission quantum and peer knowledge while scanning that catalog.
    self->priority_="head-unconfirmed-catalog-fallback";self->catalog();return;
   }
   if(ack!=Json{{"accepted",true}}){self->fail("head-admission-unconfirmed");return;}''',1);p.write_text(s)
p=repo/'network/test_capella_catalog_relay.cpp';s=p.read_text();s=s.replace('"block-reject"})','"block-reject","block-reject-first"})',1);s=s.replace('mode=="block-reject"?"block":"attestation"','(mode=="block-reject"||mode=="block-reject-first")?"block":"attestation"',1);s=s.replace('const bool accept=mode=="reject-first"&&id==root(\'2\');','const bool accept=(mode=="reject-first"||mode=="block-reject-first")&&id==root(\'2\');',1)
s=s.replace('if(mode=="reject-first"||mode=="reject-all"){','if(mode!="lost-ack"){',1);s=s.replace('mode=="reject-all"?2:1','(mode=="reject-all"||mode=="block-reject")?2:1');s=s.replace('mode=="reject-all"?0:1','(mode=="reject-all"||mode=="block-reject")?0:1');s=s.replace('size_t(mode=="reject-first")','size_t(mode=="reject-first"||mode=="block-reject-first")',1);s=s.replace('first rejected attestation starved next page','first rejected item starved next page');s=s.replace('unknown outcome or rejected block advanced scan','unknown outcome advanced scan');p.write_text(s)
p=repo/'network/test_capella_depth_catchup.cpp';s=p.read_text().replace('pages==1&&sends==1&&admitted==0','pages==1&&sends==2&&admitted==0');p.write_text(s)
p=repo/'network/test_capella_head_catalog_relay.cpp';s=p.read_text();s=s.replace('mode=="anchor"||knowledge->acknowledged(root(\'3\'))','mode=="anchor"||mode=="reject-head"||mode=="missing-export"||knowledge->acknowledged(root(\'3\'))',1)
s=s.replace('knowledge->acknowledge(id);reply(Json{{"accepted",true}});};','if(mode=="reject-head"&&id==root(\'3\')){reply(Json{{"accepted",false}});return;}knowledge->acknowledge(id);reply(Json{{"accepted",true}});};',1)
s=s.replace('if(mode=="reject-head"||mode=="cancel-head"||mode=="timeout-head")','if(mode=="cancel-head"||mode=="timeout-head")',1)
s=s.replace('if(mode=="timeout-status"||mode=="timeout-head"||mode=="timeout-catalog")','if(mode=="reject-head")check(pages==1&&knowledge->acknowledged(root(\'4\'))&&!knowledge->acknowledged(root(\'3\'))&&result["headPriority"]=="head-unconfirmed-catalog-fallback","unconfirmed head starved independent history or gained fake ACK");\n  if(mode=="timeout-status"||mode=="timeout-head"||mode=="timeout-catalog")',1);p.write_text(s)
print('Updated bounded head/catalog fallback and relevant modeled guards')
