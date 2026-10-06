from pathlib import Path
import subprocess
repo=Path('/Users/boris/Dev/blockchain-nano');draft=Path('/private/tmp/nano-v24-timeout-continuation-draft-20261006/draft-review-source')
assert subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip()=='d80e15a8f5e87fe2a01a9d31cc759efb09ea7feb'
assert not subprocess.check_output(['git','-C',str(repo),'status','--porcelain'],text=True).strip()
for name in ['capella_block_delivery.hpp','capella_head_catalog_relay.hpp']:(repo/'network'/name).write_bytes((draft/name).read_bytes())
(repo/'network/test_capella_dependency_preparation.cpp').write_bytes((draft/'test_timeout_continuation.cpp').read_bytes())
p=repo/'network/CMakeLists.txt';s=p.read_text();old='foreach(scenario normal duplicate unavailable unavailable-anchor refused lost cancel)';assert s.count(old)==1;p.write_text(s.replace(old,'foreach(scenario normal duplicate unavailable unavailable-anchor refused lost cancel slow slow-duplicate)'))
print('Deadline traversal checkpoint integrated; unchanged ACK, depth, byte and quantum limits')
