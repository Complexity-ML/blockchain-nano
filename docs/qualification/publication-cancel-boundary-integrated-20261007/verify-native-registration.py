from pathlib import Path
import subprocess,json,os,re,hashlib
r=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');build=r/'cmake-registration-native';build.mkdir();logs=r/'registration-native';logs.mkdir()
def run(name,args):
 (logs/(name+'-command.json')).write_text(json.dumps(args)+'\n')
 with (logs/(name+'.log')).open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode})+'\n');print(name,p.returncode,flush=True);assert p.returncode==0
run('configure',['/opt/homebrew/bin/cmake','-S',str(repo),'-B',str(build),'-DCMAKE_BUILD_TYPE=Release'])
os.link(r/'repair/test-draft',build/'nano-capella-transition-tests')
s=(repo/'scripts/ci/core.sh').read_text();pattern=re.search("pattern='([^']+)'",s).group(1)
with (logs/'ci-selection.json').open('w') as f:subprocess.run(['/opt/homebrew/bin/ctest','--test-dir',str(build),'-R',pattern,'--show-only=json-v1'],stdout=f,check=True)
selection=json.loads((logs/'ci-selection.json').read_text());names=[t['name'] for t in selection['tests']];expected={'core','chain','rpc_queue_expiry','ethereum_consensus_bls_verify','ethereum_consensus_bls_fast_aggregate','storage_execution_checkpoint','consensus_capella_payload_paired_state','consensus_capella_signed_transition','consensus_capella_clock_diagnostics','consensus_local_monotonic_time','consensus_capella_published_signed_inventory','consensus_capella_publication_cancel_boundary'};assert len(names)==12 and set(names)==expected
run('registered-new-gate',['/opt/homebrew/bin/ctest','--test-dir',str(build),'-R','^consensus_capella_publication_cancel_boundary$','--output-on-failure','--no-tests=error','--output-junit',str(logs/'new-gate.xml')])
(logs/'binding.json').write_text(json.dumps({'actualCMakeConfiguration':True,'ciSelectionExact12':True,'registeredNewGateExecuted':True,'other11NotExecuted':True,'qualifiedBinaryStagedByHardlink':True,'binarySha256':hashlib.sha256((build/'nano-capella-transition-tests').read_bytes()).hexdigest(),'cleanBuild':False,'networkQualified':False},indent=2)+'\n')
