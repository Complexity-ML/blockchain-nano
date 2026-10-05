#include "nano/chain.hpp"
#include "nano/consensus_attestation.hpp"
#include "nano/consensus_bls.hpp"
#include "blst.h"
#include <fstream>
#include <iostream>
using namespace nano;
ConsensusRoot root(const Json& j){const auto b=unhex(j);if(b.size()!=32)throw std::runtime_error("root width");ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
ConsensusCheckpoint cp(const Json& j){return {std::stoull(j.at("epoch").get<std::string>()),root(j.at("root"))};}
int main(int argc,char** argv){try{if(argc!=2)throw std::runtime_error("input required");std::ifstream f(argv[1]);const auto j=Json::parse(f);ConsensusBytes4 ver{};auto v=unhex(j.at("forkVersion"));if(v.size()!=4)throw std::runtime_error("fork version width");std::copy(v.begin(),v.end(),ver.begin());const auto domain=consensus_domain({1,0,0,0},ver,root(j.at("genesisRoot")));std::vector<Bytes> keys;
for(uint64_t i=0;i<65;i++){uint64_t w[4]{i+1,0,0,0};blst_scalar scalar;blst_scalar_from_uint64(&scalar,w);blst_p1 p;blst_sk_to_pk_in_g1(&p,&scalar);Bytes key(48);blst_p1_compress(key.data(),&p);keys.push_back(key);}
Json out=Json::array();for(const auto& a:j.at("votes")){const auto& d=a.at("data");const ConsensusAttestationData data{std::stoull(d.at("slot").get<std::string>()),std::stoull(d.at("index").get<std::string>()),root(d.at("beacon_block_root")),cp(d.at("source")),cp(d.at("target"))};auto sign=consensus_signing_root(consensus_attestation_root(data),domain);const auto sig=unhex(a.at("signature"));Json matches=Json::array();for(uint64_t i=0;i<keys.size();++i)if(verify_consensus_bls(keys[i],sign,sig))matches.push_back(i);out.push_back({{"targetEpoch",data.target.epoch},{"slot",data.slot},{"verifiedSyntheticIndices",matches},{"signature",a.at("signature")}});}std::cout<<Json{{"votes",out},{"scope","synthetic single-signature attribution only; not stake or quorum qualification"}}.dump(2)<<'\n';return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
