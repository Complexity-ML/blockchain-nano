// Offline archive/audit runs as root only to read the stopped private fixture.
// Ownership changes apply solely to the newly extracted copy, never its source.
export function selectRestoreRuntime({buildVolume,packaged,buildNano}) {
 if(typeof buildVolume!=='boolean'||typeof packaged!=='boolean'||buildVolume===packaged)throw Error('Restore requires exactly one reviewed artifact mode');
 if(buildVolume&&(typeof buildNano!=='string'||!/^\/build(?:\/[a-zA-Z0-9][a-zA-Z0-9_.-]*)*\/nano$/.test(buildNano)||buildNano.split('/').some(x=>x==='.'||x==='..')))throw Error('Invalid restore build binary');
 return packaged?{nano:'/usr/local/bin/nano',archiveArgs:['--user','0:0','--entrypoint','bash'],archiveCommand:[],ownerArgs:['10001:10001'],auditUsesDirectCopy:true}:{nano:buildNano,archiveArgs:[],archiveCommand:['bash'],ownerArgs:[],auditUsesDirectCopy:false};
}
