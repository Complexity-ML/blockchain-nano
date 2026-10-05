import assert from 'node:assert/strict';
import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
import {writeFile, appendFile} from 'node:fs/promises';
import {join} from 'node:path';
import {setTimeout as delay} from 'node:timers/promises';
import {collectFinalizedLoadState} from './load-state.mjs';
import {collectLoadReceipts, loadNodes} from './load-receipts.mjs';

const exec = promisify(execFile);
// Public scalar from the isolated Capella fixture. Never accepts a wallet key.
const fixtureKey = '0x'+'309'.padStart(64,'0');
const target = '0x0000000000000000000000000000000000001234';
const count = 96, maxFee = 2000000000n, gas = 100000n;

export async function runBoundedLoad({rpc, genesisHash, output, sampleFinality,
  duringSubmit=submit=>submit(),freshFinalizedState=false,
  cast = '/Users/boris/.foundry/bin/cast'}) {
  const record = e => appendFile(join(output,'load.jsonl'),JSON.stringify({time:Date.now(),...e})+'\n');
  const castCommand = async args => (await exec(cast,args,{timeout:15000,maxBuffer:1024*1024})).stdout.trim();
  const sender = await castCommand(['wallet','address','--private-key',fixtureKey]);
  const anchor = await rpc('native','eth_getBlockByNumber',['finalized',false]);
  const baseline = {};
  for (const node of loadNodes) {
    assert.equal(BigInt(await rpc(node,'eth_chainId')),1337n,'Only isolated fixture chain');
    assert.equal((await rpc(node,'eth_getBlockByNumber',['0x0',false])).hash,genesisHash,'Wrong genesis');
    assert.equal((await rpc(node,'eth_getBlockByNumber',[anchor.number,false])).hash,anchor.hash,'Initial anchor mismatch');
    assert.ok(BigInt((await rpc(node,'eth_getBlockByNumber',['finalized',false])).number)>=BigInt(anchor.number),'Initial anchor not finalized');
    assert.equal(await rpc(node,'eth_getCode',[target,anchor.number]),'0x602a5f5500','Wrong fixture target');
    baseline[node] = {
      nonce:await rpc(node,'eth_getTransactionCount',[sender,anchor.number]),
      balance:await rpc(node,'eth_getBalance',[target,anchor.number]),
      funds:await rpc(node,'eth_getBalance',[sender,anchor.number]),
    };
    for(const tag of ['latest','pending']) assert.equal(
      BigInt(await rpc(node,'eth_getTransactionCount',[sender,tag])),BigInt(baseline[node].nonce),'Fixture sender already active');
    assert.ok(BigInt(baseline[node].funds)>=BigInt(count)*(gas*maxFee+1n),'Insufficient synthetic balance');
    assert.deepEqual(baseline[node],baseline.native,'Initial state mismatch');
  }
  const transactions=[];
  for(let i=0;i<count;i++) {
    const nonce=BigInt(baseline.native.nonce)+BigInt(i);
    const raw=await castCommand(['mktx',target,'--chain','1337','--nonce',String(nonce),
      '--gas-limit',String(gas),'--gas-price',String(maxFee),'--priority-gas-price','100000000',
      '--value','1','--private-key',fixtureKey,'--rpc-url','http://127.0.0.1:1']);
    assert.match(raw,/^0x02[0-9a-f]+$/,'Expected signed type-2 transaction');
    const hash=await castCommand(['keccak',raw]);
    assert.match(hash,/^0x[0-9a-f]{64}$/);
    transactions.push({nonce:String(nonce),hash,raw});
  }
  await writeFile(join(output,'load-transactions.json'),JSON.stringify({
    description:'Finite repeated SSTORE(0,42) plus 1 wei transfers; not a storage-growth or capacity benchmark',
    castVersion:await castCommand(['--version']),genesisHash,sender,target,anchor,baseline,transactions},null,2));
  const started=Date.now(), deadline=started+600000;
  const checkDeadline=()=>assert.ok(Date.now()<deadline,'Bounded workload deadline exceeded');
  try {
    await duringSubmit(async()=>{
    for(let i=0;i<count;i++) {
      checkDeadline();
      const begin=Date.now(), tx=transactions[i];
      const result=await rpc('native','eth_sendRawTransaction',[tx.raw]);
      assert.equal(result,tx.hash,'RPC changed signed transaction hash');
      await record({event:'submitted',index:i,hash:tx.hash,elapsedMs:Date.now()-begin});
      // A finite ramp; achieved submission rates are measured, not assumed.
      if((i+1)%16===0) await record({event:'load-finality-sample',rows:await sampleFinality()});
      await delay(Math.max(0,[1000,500,250][Math.floor(i/32)]-(Date.now()-begin)));
    }
    });
    let verifiedReceipts;
    for(;;) {
      checkDeadline();
      await record({event:'load-finality-sample',rows:await sampleFinality()});
      if(!verifiedReceipts){
        const result=await collectLoadReceipts(transactions.map(t=>t.hash),rpc);
        checkDeadline();await record({event:'receipt-observation',...result});
        if(!result.verdict.complete){await delay(3000);continue;}
        verifiedReceipts=result;
      }
      const result=verifiedReceipts;
      if(result.verdict.complete) {
        let anchor=result.verdict.anchor,states={};
        if(freshFinalizedState){
          const observed=await collectFinalizedLoadState({rpc,receiptAnchor:anchor,sender,target});
          checkDeadline();await record({event:'load-state-observation',...observed});
          if(!observed.complete){await delay(3000);continue;}
          anchor=observed.anchor;states=observed.states;
        }
        const tag=anchor.number;
        for(const node of loadNodes) {
          if(!freshFinalizedState)states[node]={nonce:await rpc(node,'eth_getTransactionCount',[sender,tag]),
            balance:await rpc(node,'eth_getBalance',[target,tag]),
            storage:await rpc(node,'eth_getStorageAt',[target,'0x0',tag])};
          assert.equal(BigInt(states[node].nonce),
            BigInt(baseline.native.nonce)+BigInt(count),'Finalized sender nonce mismatch');
          assert.equal(BigInt(states[node].balance),
            BigInt(baseline.native.balance)+BigInt(count),'Finalized target balance mismatch');
          assert.equal(BigInt(states[node].storage),42n,'Finalized storage mismatch');
          assert.equal((await rpc(node,'eth_getBlockByNumber',[tag,false])).hash,
            anchor.hash,'Finalized state anchor changed');
        }
        checkDeadline();
        await record({event:'load-passed',count,elapsedMs:Date.now()-started,anchor,states,...(freshFinalizedState?{receiptAnchor:result.verdict.anchor}:{})});
        return;
      }
      await delay(3000);
    }
  } catch(error) {
    await record({event:'load-failed',message:error.message,elapsedMs:Date.now()-started});
    throw error;
  }
}
