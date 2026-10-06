#include "nano/consensus_capella_cold_records.hpp"
namespace nano {
CapellaForkRecord::CapellaForkRecord(CapellaForkBlock block):hot_(std::make_shared<CapellaForkBlock>(std::move(block))){}
CapellaForkRecord::CapellaForkRecord(CapellaColdReference reference,ConsensusBeaconHeader header,Header execution,std::shared_ptr<CapellaColdRecordCache> cache,std::function<std::optional<std::string>(const Bytes&,size_t)> fetch)
 :cold_(std::move(reference)),header_(std::move(header)),execution_header_(std::move(execution)),cache_(std::move(cache)),fetch_(std::move(fetch)){
 if(!cache_||!fetch_||cold_->digest.size()!=32||!cold_->bytes||consensus_beacon_header_root(header_)!=cold_->root)throw std::invalid_argument("Invalid Capella cold record metadata");
}
std::shared_ptr<const CapellaForkBlock> CapellaForkRecord::lease(const std::function<void()>& check)const{
 if(check)check();if(hot_)return hot_;if(!cold_||!cache_)throw std::invalid_argument("Missing Capella fork record");
 auto pin=cache_->load(*cold_,fetch_,check);
 if(pin->block.header!=header_||pin->block.execution_block.header.to_json()!=execution_header_.to_json())throw std::invalid_argument("Capella cold record metadata mismatch");
 return std::shared_ptr<const CapellaForkBlock>(pin,&pin->block);
}
CapellaForkBlock& CapellaForkRecord::writable(const std::function<void()>& check){
 if(check)check();if(!hot_){const auto pin=lease(check);hot_=std::make_shared<CapellaForkBlock>(*pin);}
 else if(hot_.use_count()!=1)hot_=std::make_shared<CapellaForkBlock>(*hot_);
 cold_.reset();cache_.reset();fetch_={};return *hot_;
}
const ConsensusBeaconHeader& CapellaForkRecord::header()const{if(hot_)return hot_->header;if(cold_)return header_;throw std::invalid_argument("Missing Capella fork metadata");}
const Header& CapellaForkRecord::execution_header()const{if(hot_)return hot_->execution_block.header;if(cold_)return execution_header_;throw std::invalid_argument("Missing Capella execution metadata");}
}
