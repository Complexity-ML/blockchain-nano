#include "support/process.hpp"
#include "nano/rpc.hpp"
#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "../network/json_frame.hpp"
using namespace test;
int main(){try{
 Temp root;auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));
 std::string fork=std::getenv("NANO_TEST_EVM_FORK")?std::getenv("NANO_TEST_EVM_FORK"):"Paris";
 check(fork=="Paris"||fork=="Shanghai","explicit network profile");
 auto genesis=Genesis::from_json({{"gasLimit","0x2625a00"},{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bftVersion",1},{"bootstrapValidator",signer},{"evmFork",fork}}},{"alloc",{{signer,{{"balance","0x100000000000000000"}}}}}});
 Blockchain chain(root/"producer",genesis);ChainStore journal(root/"journal");
 for(unsigned nonce=0;nonce<10;++nonce){Transaction tx;tx.chain_id=1337;tx.nonce=nonce;tx.gas=4000000;tx.gas_price=2000000000;tx.to="0x00000000000000000000000000000000000000b0";tx.value=1;tx.data=Bytes(900000,0);auto signed_tx=SignedTransaction::sign(tx,key);check(signed_tx.raw().size()<1024*1024,"transaction below pool byte ceiling");check(chain.submit(signed_tx),"transaction admitted");}
 auto block=chain.block_template(signer,12,0);check(block.transactions.size()==10,"all ten fit gas budget");sign_block(block,key,1337,genesis.block.header.hash());chain.verify_authentication(block);chain.verify_proposal(block,12);
 auto wire=block.to_transport_json().dump();check(wire.size()>JsonFrame::maximum_size,"compact proposal must exceed frame cap");
 Json failures=Json::array();bool voted=false;
 try{ConsensusRound round(chain,journal,key,0,ConsensusRound::Clock::time_point{},1s,1s,1s);round.propose(block,12);voted=round.advance(ConsensusRound::Clock::time_point{},12).committed;}
 catch(const std::exception& e){failures.push_back({{"stage","voting-journal"},{"error",e.what()}});check(chain.tip().header.number==0&&!journal.latest_consensus_vote(signer,1337,genesis.block.header.hash()),"failed reservation cannot publish block or vote");}
 // An external certificate fixture isolates import/export from the failing
 // local journal. It is not evidence that live validators signed this block.
 if(!voted){FinalityVote vote;vote.height=1;vote.round=0;vote.step=VoteStep::precommit;vote.validator=signer;vote.validator_set_hash=voting_set_hash(chain.voting_set());vote.block_hash=block.header.hash();sign_vote(vote,key,1337,genesis.block.header.hash());check(verify_commit_certificate({vote},chain.voting_set(),1,0,block.header.hash(),1337,genesis.block.header.hash()),"external fixture certificate valid");chain.import_certified_block(block,0,{vote},12);}
 check(chain.tip().header.hash()==block.header.hash()&&chain.audit().at("verified")==true,"imported oversized canonical history audits");
 Rpc rpc(chain);auto call=[&](const std::string& method,Json params){return rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}});};
 auto page=call("nano_networkBlocks",Json::array({0}));bool exported=page.contains("result")&&!page.at("result").at("blocks").empty();if(!exported)failures.push_back({{"stage","block-export"},{"reply",page}});
 uint64_t cursor=0;bool announced=false;for(unsigned i=0;i<15&&!announced;++i){auto reply=call("nano_networkEvents",Json::array({cursor}));if(reply.contains("error")){failures.push_back({{"stage","block-event"},{"reply",reply}});break;}auto value=reply.at("result");for(const auto& event:value.at("events"))if(event.at("type")=="block")announced=true;auto next=value.at("cursor").get<uint64_t>();if(next==cursor)break;cursor=next;}
 bool framed=false;try{JsonFrame frame;framed=frame.feed(wire);}catch(const std::exception& e){failures.push_back({{"stage","libp2p-frame"},{"error",e.what()}});}
 std::cout<<Json{{"scenario","oversized-valid-block"},{"fork",fork},{"gasLimit",quantity(block.header.gas_limit)},{"gasUsed",quantity(block.header.gas_used)},{"transactions",10},{"compactBytes",wire.size()},{"maximumFrameBytes",JsonFrame::maximum_size},{"proposalValid",true},{"liveJournalCommitted",voted},{"externalCertificateFixture",!voted},{"canonicalAudit",true},{"exported",exported},{"announced",announced},{"framed",framed},{"failures",failures}}.dump()<<std::endl;
 check(voted&&exported&&announced&&framed,"valid oversized block cannot complete voting and transport lifecycle");return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
