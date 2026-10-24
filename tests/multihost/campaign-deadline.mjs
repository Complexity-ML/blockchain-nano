// One monotone campaign budget. No process, filesystem, signer or network API.
export function createCampaignDeadline({startedAt,maximumRunMs,closeoutReserveMs,
 now=()=>performance.now(),setTimer=setTimeout,clearTimer=clearTimeout,onExpire=()=>{}}={}){
 const need=(ok,label)=>{if(!ok)throw Error('Fresh campaign deadline: '+label);};
 need(Number.isFinite(startedAt)&&startedAt>=0,'startedAt');
 need(Number.isSafeInteger(maximumRunMs)&&maximumRunMs>0&&maximumRunMs<=2147483647,'maximumRunMs');
 need(Number.isSafeInteger(closeoutReserveMs)&&closeoutReserveMs>0&&closeoutReserveMs<maximumRunMs,'closeoutReserveMs');
 need([now,setTimer,clearTimer,onExpire].every(v=>typeof v==='function'),'callbacks');
 const overallEnd=startedAt+maximumRunMs,workEnd=overallEnd-closeoutReserveMs;
 let phase='work',controller=new AbortController(),timer=null,disposed=false,lastNow=startedAt;
 const clock=()=>{const value=now();need(Number.isFinite(value)&&value>=lastNow,'monotone clock');lastNow=value;return value;};
 const end=()=>phase==='work'?workEnd:overallEnd;
 const cancelTimer=()=>{if(timer!==null){clearTimer(timer);timer=null;}};
 function expire(at){
  if(controller.signal.aborted)return;
  cancelTimer();const reason=Error('Fresh campaign '+phase+' deadline');controller.abort(reason);
  onExpire({phase,deadline:end(),elapsedMs:at-startedAt});
 }
 function arm(){
  cancelTimer();if(disposed)return;
  const at=clock(),remaining=end()-at;
  if(remaining<=0){expire(at);return;}
  timer=setTimer(()=>{timer=null;if(!disposed)arm();},remaining);
 }
 function check(){
  need(!disposed,'disposed');const at=clock();if(at>=end())expire(at);
  if(controller.signal.aborted)throw controller.signal.reason;return at;
 }
 function remaining(requestedMs){
  need(Number.isSafeInteger(requestedMs)&&requestedMs>0,'requested command timeout');
  const left=Math.floor(end()-check());need(left>=1,'command budget exhausted');return Math.min(requestedMs,left);
 }
 function enterCloseout(){
  need(!disposed,'disposed');if(phase==='closeout')return;
  cancelTimer();if(!controller.signal.aborted)controller.abort(Error('Fresh campaign work phase ended'));
  phase='closeout';controller=new AbortController();arm();
 }
 function dispose(){if(disposed)return;cancelTimer();disposed=true;if(!controller.signal.aborted)controller.abort(Error('Fresh campaign deadline disposed'));}
 arm();
 return Object.freeze({check,remaining,enterCloseout,dispose,
  get signal(){return controller.signal;},get phase(){return phase;},
  get deadline(){return end();},workEnd,overallEnd});
}
