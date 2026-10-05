// Explicit private qualification selection, never a network clock authority.
export function monotonicClockConfiguration(config,value,profile){
 if(value===undefined)return config;
 if(typeof value!=='string'||!(/^[1-9][0-9]*$/).test(value))throw Error('Wall clock drift requires positive decimal milliseconds');
 const drift=BigInt(value);if(drift>0xffffffffffffffffn)throw Error('Wall clock drift exceeds uint64');
 if(!config?.capella||config.bellatrix||profile?.format!=='nano-capella-profile-v1')throw Error('Monotonic clock qualification requires Capella');
 const quantity=x=>{if(typeof x!=='string'||!(/^0x(?:0|[1-9a-f][0-9a-f]*)$/).test(x))throw Error('Invalid clock bound');return BigInt(x);};
 const due=quantity(profile.common?.[2]),step=quantity(config.capella.clock?.maximumForwardStepMs);
 if(drift>=due||drift>=step)throw Error('Wall clock drift reaches duty deadline or recovery step');
 const result=structuredClone(config);result.capella.clock.maximumWallClockDriftMs='0x'+drift.toString(16);return result;
}
