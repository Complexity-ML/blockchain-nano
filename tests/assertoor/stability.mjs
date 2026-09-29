// Native-node checks complement Assertoor's shared cache. No automatic retry
// can erase a conflicting finalized root or a finality regression.
import {setTimeout as delay} from 'node:timers/promises';
export async function observeStableFinality(endpoints, epochs, record, {clock=Date.now, wait=delay, request=fetch, settleRecoveryGaps=false,pinnedStateRoots=false}={}) {
  if (!Array.isArray(endpoints) || endpoints.length < 2 || new Set(endpoints).size !== endpoints.length) throw Error('At least two distinct endpoints required');
  if (!Number.isInteger(epochs) || epochs < 2 || epochs > 16) throw Error('Stability window must be 2..16 epochs');
  const read = async (url, path) => {
    const r = await request(url + path, {headers:{Accept:'application/json'}, signal:AbortSignal.timeout(5000)});
    if (!r.ok) throw Error(`Stability REST ${r.status}: ${path}`);
    return r.json();
  };
  const specs = await Promise.all(endpoints.map(url => read(url, '/eth/v1/config/spec')));
  const slots = Number(specs[0].data.SLOTS_PER_EPOCH), seconds = Number(specs[0].data.SECONDS_PER_SLOT);
  if (!Number.isSafeInteger(slots) || !Number.isSafeInteger(seconds) || slots < 1 || seconds < 1 || specs.some(s => Number(s.data.SLOTS_PER_EPOCH) !== slots || Number(s.data.SECONDS_PER_SLOT) !== seconds)) throw Error('Inconsistent epoch configuration');
  const epochMs = slots * seconds * 1000;
  const start = clock(), deadline = start + (epochs + 3) * epochMs;
  const roots = new Map(), previous = endpoints.map(() => ({epoch:0n, slot:0n, headChanged:start}));
  let baseline, commonEpoch, commonChanged = start, advances = 0;
  while (clock() < deadline) {
    const samples = await Promise.all(endpoints.map(async (url, index) => {
      let checkpoint, head;
      const prior = previous[index];let observedEpoch = prior.epoch;
      // These REST endpoints are separate snapshots. Never combine a checkpoint
      // from the old head with a newly published head across an epoch boundary.
      for (let attempt=0; attempt<3; ++attempt) {
        const before = (await read(url, '/eth/v1/beacon/headers/head')).data;
        const stateRoot=before.header?.message?.state_root;
        if(pinnedStateRoots && !/^0x[0-9a-fA-F]{64}$/.test(stateRoot??''))throw Error('Missing or invalid Beacon state root');
        if(pinnedStateRoots && !/^0x[0-9a-fA-F]{64}$/.test(before.root??''))throw Error('Missing or invalid Beacon head root');
        checkpoint = (await read(url, '/eth/v1/beacon/states/'+(pinnedStateRoots?stateRoot:'head')+'/finality_checkpoints')).data.finalized;
        const observed = BigInt(checkpoint.epoch), known = roots.get(checkpoint.epoch);
        // Finality conflicts/regressions do not depend on the head's slot and
        // must remain fatal even when this sampling attempt races an update.
        if (observed < observedEpoch) throw Error('Native progress regressed');
        if (known && known !== checkpoint.root) throw Error('Conflicting finalized roots');
        roots.set(checkpoint.epoch, checkpoint.root);observedEpoch=observed;
        if(pinnedStateRoots){
          // The state selector is immutable even if fork choice moves while
          // these HTTP calls are in flight. Unknown/pruned roots remain fatal.
          head=before.header.message;
          await record({event:'finality-state-bound',endpoint:url,headRoot:before.root,stateRoot,checkpoint});break;
        }
        const after = (await read(url, '/eth/v1/beacon/headers/head')).data;
        if (typeof before.root !== 'string' || typeof after.root !== 'string') throw Error('Missing Beacon head root');
        if (before.root === after.root) {head=after.header.message;break;}
        await record({event:'head-changed-during-finality-read',endpoint:url,attempt,beforeRoot:before.root,afterRoot:after.root,checkpoint});
      }
      if (!head) throw Error('Consistent finality sample unavailable');
      const epoch = BigInt(checkpoint.epoch), slot = BigInt(head.slot);
      await record({event:'finality-observation',time:clock(),endpoint:url,checkpoint,headSlot:head.slot,slotsPerEpoch:slots});
      if (epoch < prior.epoch || slot < prior.slot - 2n) throw Error('Native progress regressed');
      if (slot > prior.slot) prior.headChanged = clock();
      if (clock() - prior.headChanged > seconds * 3000) throw Error('Native head stalled for three slots');
      prior.epoch = epoch; prior.slot = slot;
      if (slot / BigInt(slots) > epoch + 4n) throw Error('Finality lag exceeds four epochs');
      return {endpoint:url, checkpoint, headSlot:head.slot};
    }));
    await record({time:clock(), samples});
    if (samples.every(s => s.checkpoint.epoch === samples[0].checkpoint.epoch && s.checkpoint.root === samples[0].checkpoint.root)) {
      const epoch = BigInt(samples[0].checkpoint.epoch);
      if (epoch >= 2n && baseline === undefined) {baseline = epoch; commonEpoch = epoch; commonChanged = clock();}
      else if (baseline !== undefined && epoch > commonEpoch) {
        const gap = epoch !== commonEpoch + 1n;
        if (gap && !settleRecoveryGaps) throw Error('Skipped common finalized epoch in stability window');
        // Bind every advancing checkpoint to execution on both nodes.
        const blocks = await Promise.all(endpoints.map(async url => {
          const block = (await read(url, '/eth/v2/beacon/blocks/' + samples[0].checkpoint.root)).data.message;
          const r = await request(url, {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({jsonrpc:'2.0',id:1,method:'eth_getBlockByNumber',params:['0x'+BigInt(block.body.execution_payload.block_number).toString(16),false]}),signal:AbortSignal.timeout(5000)});
          const execution = (await r.json()).result;
          if (!r.ok || !execution || execution.hash !== block.body.execution_payload.block_hash) throw Error('Finalized execution block unavailable');
          return execution.hash;
        }));
        if (blocks.some(hash => hash !== blocks[0])) throw Error('Finalized execution hashes differ');
        if (gap) {
          // FFG may finalize a later justified checkpoint after delayed votes.
          // A gap earns no credit: restart the consecutive window, retaining
          // all safety history and the original absolute deadline.
          await record({event:'recovery-gap-window-reset',time:clock(),fromEpoch:String(commonEpoch),toEpoch:String(epoch),discardedAdvances:advances,executionHash:blocks[0]});
          baseline=epoch;advances=0;
        } else ++advances;
        commonEpoch = epoch; commonChanged = clock();
        await record({time:clock(), commonEpoch:String(epoch), executionHash:blocks[0], advances});
        if (advances === epochs) return {epochs:advances, firstEpoch:String(baseline), lastEpoch:String(commonEpoch), elapsedMs:clock()-start};
      }
    }
    if (clock() - commonChanged > 2 * epochMs) throw Error('Common finality stalled for two epochs');
    await wait(seconds * 1000);
  }
  throw Error('Stable finality window deadline exceeded');
}
