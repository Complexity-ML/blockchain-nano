// Serial terminal checks, not a campaign or signing authority. A failed check
// cannot skip later cleanup/fences, replace the first error, or manufacture PASS.
export function createTerminalEvidenceSequence({firstFailure=null,recordFailure=async()=>{}}={}){
 const need=(ok,message)=>{if(!ok)throw Error('Terminal evidence sequence: '+message);};
 need(typeof recordFailure==='function','failure recorder callback');
 const retained=error=>error instanceof Error?error:Error('Terminal callback threw a non-Error value',{cause:error});
 const stages=[],failures=[],seen=new Set();let first=firstFailure===null?null:retained(firstFailure),firstAt=firstFailure===null?null:{stage:'campaign-body',phase:'run'},active=false;
 function failed(stage,phase,error){
  error=retained(error);failures.push({stage,phase,error});stages.push({stage,phase,passed:false});
  if(firstAt===null){first=error;firstAt={stage,phase};}
 }
 return {
  async attempt(stage,run){
   need(typeof stage==='string'&&/^[a-z][a-z0-9-]{0,95}$/.test(stage)&&!seen.has(stage),'unique fixed stage');
   need(typeof run==='function'&&!active,'serial check callback');seen.add(stage);active=true;
   try{
    try{await run();stages.push({stage,phase:'run',passed:true});return true;}
    catch(error){
     failed(stage,'run',error);
     try{await recordFailure({stage,error});}catch(recordError){failed(stage,'failure-record',recordError);}
     return false;
    }
   }finally{active=false;}
  },
  get firstFailure(){return first;},
  get failures(){return failures.map(row=>({...row}));},
  report(){
   return {format:'nano-terminal-evidence-stages-v1',passed:firstAt===null,
    firstFailure:firstAt===null?null:{...firstAt},stages:stages.map(row=>({...row})),
    productionQualified:false};
  }
 };
}
