// Explicit opt-in for isolated Capella restore qualification only.
export function pruningRestoreMode(value,{profile,backupRestore}){
 if(value!==undefined&&value!=='0'&&value!=='1')throw Error('Invalid historical pruning selector');
 const enabled=value==='1';
 if(enabled&&(profile!=='capella'||backupRestore!==true))throw Error('Historical pruning requires standalone Capella backup restore');
 return enabled;
}
export function pruningRestoreConfiguration(config,enabled){
 if(!enabled)return config;
 if(!config?.capella||config.bellatrix)throw Error('Pruning restore requires a Capella configuration');
 const copy=structuredClone(config);
 copy.capella.automaticPruning=true;copy.capella.historicalStatePruning=true;
 return copy;
}
export function verifyPrunedRestoreAudit(audit){
 if(audit?.canonicalHistoryVerified!==true||audit.allHistoricalStatesMaterialized!==false||!Number.isSafeInteger(audit.prunedExecutionStatesReplayed)||audit.prunedExecutionStatesReplayed<=0)throw Error('Restore audit did not prove pruned states replayed from trusted genesis');
 return audit;
}
