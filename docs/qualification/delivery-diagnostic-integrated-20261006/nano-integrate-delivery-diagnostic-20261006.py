from pathlib import Path
import shutil
repo=Path('/Users/boris/Dev/blockchain-nano');draft=Path('/private/tmp/nano-delivery-cause-draft-20261006')
for n in ['capella_attestation_delivery.hpp','core_rpc_diagnostic.hpp','core.hpp','test_capella_attestation_delivery.cpp','test_core_rpc_diagnostic.cpp']:shutil.copy2(draft/n,repo/'network'/n)
p=repo/'network/main.cpp';s=p.read_text();s=s.replace(' bool trace_network_=std::getenv("NANO_TEST_NETWORK_TRACE")!=nullptr;',' bool trace_network_=std::getenv("NANO_TEST_NETWORK_TRACE")!=nullptr;\n unsigned attestation_trace_count_=0;')
old='reply(std::move(result));},quantum,export_cache);(*active_vote)->start();'
new='reply(std::move(result));},quantum,export_cache,trace_network_?CapellaAttestationDelivery::Trace{[this](const Json& row){if(attestation_trace_count_++>=256)return;auto record=row;record["event"]="test-capella-attestation-delivery";std::cerr<<record.dump()<<std::endl;}}:CapellaAttestationDelivery::Trace{});(*active_vote)->start();'
assert s.count(old)==1;s=s.replace(old,new);p.write_text(s)
print('Integrated diagnostics; delivery result, deadlines, ACK admission and retry policy unchanged')
