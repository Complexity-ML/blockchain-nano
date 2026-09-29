// A sampled bound, not an NTP synchronization claim. The remote timestamp is
// observed between local send and receive; transport asymmetry is left unknown.
export async function sampleClock(remote,now=Date.now){
 const before=now(),text=(await remote()).trim(),after=now();
 if(!Number.isSafeInteger(before)||!Number.isSafeInteger(after)||after<before||!/^\d+$/.test(text))throw Error('Invalid clock sample');
 const time=Number(text);
 if(!Number.isSafeInteger(time))throw Error('Unsafe remote timestamp');
 return {localBeforeMs:before,localAfterMs:after,remoteMs:time,offsetMinMs:time-after,offsetMaxMs:time-before,roundTripMs:after-before};
}
