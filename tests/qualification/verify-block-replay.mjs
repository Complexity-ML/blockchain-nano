import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {resolve,join} from 'node:path';
import {digest,parseReport} from './audit-evidence.mjs';

assert(process.argv[2],'Usage: node verify-block-replay.mjs <campaign directory> [repository root]');
const directory=resolve(process.argv[2]),root=resolve(process.argv[3]??'.');
const inventory=JSON.parse(await readFile(join(directory,'inventory.json'),'utf8'));
const campaigns=JSON.parse(await readFile(join(directory,'results.json'),'utf8'));
const expected=new Map();
for(const entry of inventory){
 assert.equal(await digest(entry.path),entry.sha256,'Fixture changed after archive verification');
 if(!expected.has(entry.group))expected.set(entry.group,new Map());
 for(const [name,test] of Object.entries(JSON.parse(await readFile(entry.path,'utf8')))){
  const key=JSON.stringify([entry.path,name,test.network]);
  assert(!expected.get(entry.group).has(key),'Duplicate fixture identity');
  expected.get(entry.group).set(key,test.blocks.map(b=>b.expectException??null));
 }
}
assert.equal(campaigns.length,expected.size,'Missing group report');
const results=[],seenGroups=new Set();
for(const campaign of campaigns){
 assert(!seenGroups.has(campaign.group),'Duplicate group report');seenGroups.add(campaign.group);
 const wanted=expected.get(campaign.group);assert(wanted,'Unexpected report group');
 const file=resolve(root,campaign.report);assert.equal(await digest(file),campaign.reportSha256);
 const report=parseReport(await readFile(file,'utf8'));assert.deepEqual(report.issues,[]);
 const seen=new Set();let accepted=0,rejected=0;
 for(const row of report.rows){
  const key=JSON.stringify([row.file,row.case,row.fork]);assert(wanted.has(key),'Unexpected case');assert(!seen.has(key),'Duplicate case');seen.add(key);
  const blocks=wanted.get(key),rejections=blocks.filter(x=>x!==null);
  assert.equal(row.status,'matched');assert.equal(row.postStateMatches,true);assert.equal(row.lastBlockHashMatches,true);
  assert.equal(row.blocksReplayed,blocks.length-rejections.length,'Missing accepted block');
  assert.equal(row.blocksRejected,rejections.length,'Missing rejected block');
  assert.deepEqual(row.rejections.map(r=>r.expected),rejections,'Rejection order or category differs');
  accepted+=row.blocksReplayed;rejected+=row.blocksRejected;
 }
 assert.equal(seen.size,wanted.size,'Omitted fixture case');
 assert.equal(report.summary.files,inventory.filter(x=>x.group===campaign.group).length);
 results.push({group:campaign.group,cases:seen.size,acceptedBlocks:accepted,rejectedBlocks:rejected,coverageVerified:true});
}
console.log(JSON.stringify({scope:'Exact case and per-case accepted/rejected block coverage; state/hash commitments and rejection semantics, not full Ethereum consensus validation',results},null,2));
