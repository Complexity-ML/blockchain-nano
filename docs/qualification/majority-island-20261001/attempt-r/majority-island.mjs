import {createHash} from 'node:crypto';

// Read only the versioned public Capella exporter fixture. This is not an SSZ
// decoder for arbitrary Beacon forks, and key counts never substitute for stake.
export function fixtureStake({profileBytes,initialBytes,configuration,manifest}) {
 const hash=b=>'0x'+createHash('sha256').update(b).digest('hex');
 if(hash(profileBytes)!==configuration.profile.sha256||hash(initialBytes)!==configuration.initial.sha256)throw Error('Majority fixture hash mismatch');
 const profile=JSON.parse(profileBytes),initial=JSON.parse(initialBytes);
 if(profile.format!=='nano-capella-profile-v1'||manifest.fork!=='capella'||manifest.syntheticValidators!==64)throw Error('Expected public Capella fixture');
 const layout=profile.common?.[0]?.stateLimits,slots=BigInt(profile.common?.[0]?.ffg?.[0]);
 if(!Array.isArray(layout)||slots<=0n)throw Error('Invalid fixture layout');
 const matches=initial.blocks.filter(b=>b[0]===initial.anchor);
 if(matches.length!==1||!/^0x(?:[0-9a-f]{2})+$/.test(matches[0][2][0]))throw Error('Invalid fixture anchor');
 const state=Buffer.from(matches[0][2][0].slice(2),'hex');
 const hist=176+64*Number(BigInt(layout[0]));
 if(!Number.isSafeInteger(hist)||hist<176||hist+96>state.length)throw Error('Invalid fixture state bounds');
 const begin=state.readUInt32LE(hist+88),end=state.readUInt32LE(hist+92);
 if(begin<hist+96||end>state.length||end-begin!==64*121)throw Error('Invalid fixture validator bounds');
 const epoch=state.readBigUInt64LE(40)/slots,stakes=[];
 for(let i=0;i<64;i++){
  const at=begin+i*121,stake=state.readBigUInt64LE(at+80);
  if(state[at+88]!==0||state.readBigUInt64LE(at+97)>epoch||state.readBigUInt64LE(at+105)<=epoch||stake===0n)throw Error('Fixture requires active unslashed stake');
  stakes.push(stake);
 }
 const names=['mac-native','mac-docker','vps-docker'],seen=new Set(),weights={};
 if(manifest.nodes.length!==3)throw Error('Expected three fixture owners');
 for(const name of names){
  const owners=manifest.nodes.filter(n=>n.name===name);if(owners.length!==1)throw Error('Invalid fixture owner');
  weights[name]=0n;
  for(const index of owners[0].indices){
   if(!/^(0|[1-9][0-9]*)$/.test(index))throw Error('Invalid fixture index');
   const n=Number(index);if(n>=64||seen.has(n))throw Error('Duplicate or unknown fixture index');seen.add(n);weights[name]+=stakes[n];
  }
 }
 const total=stakes.reduce((a,b)=>a+b,0n),majority=weights['mac-native']+weights['mac-docker'];
 if(seen.size!==64||3n*majority<=2n*total||3n*weights['vps-docker']>=total)throw Error('Fixture does not provide strict majority and minority');
 return {slotsPerEpoch:String(slots),totalStake:String(total),majorityStake:String(majority),minorityStake:String(weights['vps-docker']),weights:Object.fromEntries(Object.entries(weights).map(([k,v])=>[k,String(v)])),initialSha256:hash(initialBytes),profileSha256:hash(profileBytes)};
}

export function majorityIslandPolicy({stake,peerIds,record}) {
 const slots=BigInt(stake.slotsPerEpoch);let cutoff=null,sawIsolation=false,highest=null;
 if(slots<=0n||3n*BigInt(stake.majorityStake)<=2n*BigInt(stake.totalStake))throw Error('Invalid majority stake');
 if(!peerIds['mac-native']||!peerIds['mac-docker']||peerIds['mac-native']===peerIds['mac-docker'])throw Error('Distinct majority peer IDs required');
 const find=(rows,name)=>{const matches=rows.filter(r=>r.name===name);if(matches.length!==1)throw Error('Missing majority observer');return matches[0];};
 const connected=rows=>{
  for(const [a,b] of [['mac-native','mac-docker'],['mac-docker','mac-native']]){
   const row=find(rows,a);if(row.peerId!==peerIds[a]||!Array.isArray(row.connectedPeerIds)||!row.connectedPeerIds.includes(peerIds[b]))throw Error('Direct majority link missing');
  }
 };
 const headEpoch=row=>{if(!/^(0|[1-9][0-9]*)$/.test(row.progress?.headSlot))throw Error('Majority requires head-slot evidence');return BigInt(row.progress.headSlot)/slots;};
 return {
  async before(rows){connected(rows);cutoff=rows.reduce((max,r)=>{const e=headEpoch(r)+1n;return e>max?e:max;},0n);await record({event:'majority-island-stake-bound',stake,peerIds,preCutFinalityCeiling:String(cutoff)});},
  async observe(rows){
   connected(rows);const remote=find(rows,'vps-docker');
   // Permit commitments already received before Docker disconnect completed.
   // Freeze the ceiling at the first observed zero-peer snapshot; never raise
   // it on subsequent isolated reads or from a finalized checkpoint itself.
   if(!sawIsolation&&BigInt(remote.peers)===0n){const e=headEpoch(remote)+1n;if(e>cutoff)cutoff=e;sawIsolation=true;await record({event:'majority-island-isolated-bound',finalityCeiling:String(cutoff)});}
   if(!sawIsolation)return;
   if(BigInt(remote.checkpoint.epoch)>cutoff)throw Error('Minority finalized beyond pre-cut commitments');
   const a=find(rows,'mac-native').checkpoint,b=find(rows,'mac-docker').checkpoint;
   if(a.epoch===b.epoch&&a.root===b.root&&BigInt(a.epoch)>cutoff&&(highest===null||BigInt(a.epoch)>highest)){
    highest=BigInt(a.epoch);await record({event:'majority-island-finality-advanced',checkpoint:a,minority:remote.checkpoint,finalityCeiling:String(cutoff)});
   }
  },
  async finish(){if(!sawIsolation||highest===null)throw Error('No new majority finality during isolation');await record({event:'majority-island-cut-passed',highestMajorityEpoch:String(highest),finalityCeiling:String(cutoff)});},
  recoveryFloor(){return highest;}
 };
}
