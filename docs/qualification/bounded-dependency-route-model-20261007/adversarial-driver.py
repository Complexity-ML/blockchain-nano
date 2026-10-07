from pathlib import Path
import json,subprocess
r=Path(__file__).parent;d=r/'route-adversarial';d.mkdir();s=(r/'profile.cpp').read_text()
s=s.replace('size_t total_exports=0,','bool refused=false,cancelled=false;size_t total_exports=0,',1)
s=s.replace('Json last;unsigned completed=0,exports=0;', 'Json last;Reply late;const auto before_ack=acknowledged;unsigned completed=0,exports=0;',1)
s=s.replace('require(n==acknowledged+1,"child sent before acknowledged parent");', '''require(n==acknowledged+1,"child sent before acknowledged parent");
    if(!preparation->route.empty()&&!refused){refused=true;reply(Json{{"accepted",false}});reply(Json{{"accepted",true}});return;}
    if(!preparation->route.empty()&&refused&&!cancelled){late=reply;return;}''',1)
s=s.replace('relay->start(5s);io.run();', '''relay->start(5s);io.poll();
  if(late){relay->cancel();late(Json{{"accepted",true}});cancelled=true;require(acknowledged==before_ack,"fixture ACK counter changed after cancellation");}
  io.restart();io.run();require(preparation->route.size()<=256,"route exceeded bound");''',1)
s=s.replace('require(knowledge->acknowledged(id(length))&&', 'require(refused&&cancelled,"adversarial route cases not exercised");\n require(knowledge->acknowledged(id(length))&&',1)
s=s.replace('{"mode","trusted local fixture exports and modeled receiver ACKs"}', '{"mode","route refusal with contradictory duplicate ACK, cancellation with late ACK, followed by complete retry"},{"refusalExercised",refused},{"lateAckAfterCancellationExercised",cancelled}',1)
(d/'test.cpp').write_text(s)
for stage in ['compile','link']:
 a=json.loads((r/f'{stage}-command.json').read_text());a=[str(d/'test.cpp') if x==str(r/'profile.cpp') else str(d/'test.o') if x==str(r/'profile.o') else str(d/'test') if x==str(r/'profile') else x for x in a];(d/f'{stage}-command.json').write_text(json.dumps(a)+'\n')
 with (d/f'{stage}.log').open('w') as f:p=subprocess.run(a,stdout=f,stderr=subprocess.STDOUT)
 (d/f'{stage}-exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,stage
with (d/'result.json').open('w') as out,(d/'error.log').open('w') as err:p=subprocess.run([str(d/'test'),'513'],stdout=out,stderr=err)
(d/'exit.json').write_text(json.dumps({'exitCode':p.returncode})+'\n');print('adversarial',p.returncode);assert p.returncode==0
