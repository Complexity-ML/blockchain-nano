from pathlib import Path
import json,subprocess
r=Path(__file__).parent;old=Path('/private/tmp/nano-long-dependency-profile-20261007');src=(r/'profile.cpp').read_text()
src=src.replace('size_t total_exports=0,','size_t cache_hits=0;size_t total_exports=0,',1)
src=src.replace('asio::io_context io;InboundBudget budget;', 'for(unsigned i=0;i<256;++i)cache->remember(id(100000+attempts*256+i),"{}",id(0));\n  asio::io_context io;InboundBudget budget;',1)
src=src.replace('results.push_back(', 'if(last.contains("dependencyProgress"))cache_hits+=last["dependencyProgress"].value("exportCacheHits",0u);\n  results.push_back(',1)
src=src.replace('{"mode","trusted local fixture exports and modeled receiver ACKs"}', '{"mode","modeled receiver ACKs with independent full export-cache churn between turns"},{"cacheHits",cache_hits}',1)
for mode in ['parent','draft']:
 d=r/('churn-'+mode);d.mkdir();s=src
 if mode=='draft':s=s.replace('results.push_back(', 'require(preparation->route.size()<=256,"route exceeded dependency bound");\n  results.push_back(',1)
 (d/'profile.cpp').write_text(s)
 for stage in ['compile','link']:
  args=json.loads((old/f'actual-{stage}-command.json').read_text());args=[str(d/'profile.cpp') if a==str(old/'profile-actual.cpp') else str(d/'profile.o') if a==str(old/'profile-actual.o') else str(d/'profile') if a==str(old/'profile-actual') else a for a in args]
  if stage=='compile' and mode=='draft':args.insert(1,'-I'+str(r/'network'))
  (d/f'{stage}-command.json').write_text(json.dumps(args)+'\n')
  with (d/f'{stage}.log').open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
  (d/f'{stage}-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,(mode,stage)
 for n in [513,2049]:
  with (d/f'{n}-result.json').open('w') as out,(d/f'{n}-error.log').open('w') as err:p=subprocess.run([str(d/'profile'),str(n)],stdout=out,stderr=err)
  (d/f'{n}-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,(mode,n)
  x=json.loads((d/f'{n}-result.json').read_text());print(mode,n,x['turns'],x['exportsRequested'],x['payloadsAdmitted'],flush=True)
