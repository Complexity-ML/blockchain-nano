from pathlib import Path
p=Path('/Users/boris/Dev/blockchain-nano/network/capella_head_catalog_relay.hpp');s=p.read_text();old='void fail(const char* reason){\n  // Preserve';assert s.count(old)==1;p.write_text(s.replace(old,'void fail(const char* reason){\n  if(finished_)return;\n  // Preserve'))
