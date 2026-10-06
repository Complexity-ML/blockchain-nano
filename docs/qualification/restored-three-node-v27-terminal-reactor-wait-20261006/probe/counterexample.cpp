 // Diagnostic counterexample only: writer is cancelled before publication.
 // Hold the real reactor lock through the same checkpoint as the qualified
 // network-status probe, then request cancellation of a waiting inventory.
 if(std::getenv("NANO_TEST_CATALOG_REACTOR_WAIT")){
  const auto primary=reactor.current().token;
  CapellaInventorySessions catalogs(reactor,2,65536,std::chrono::seconds(60));
  const auto gen=catalogs.generation(),request="0x"+std::string(64,'a');
  std::promise<void> held,unblock;auto ready=held.get_future();auto gate=unblock.get_future().share();
  std::future<bool> writer,acquire,other;
  struct ReleaseProbeGate{std::promise<void>& p;~ReleaseProbeGate(){try{p.set_value();}catch(const std::future_error&){}}} release_gate{unblock};
  writer=std::async(std::launch::async,[&]{bool paused=false;try{return reactor.block(wire,[&]{if(!paused){paused=true;held.set_value();gate.wait();throw ExecutionCancelled("reactor wait probe writer cancelled");}});}catch(const ExecutionCancelled&){return false;}});
  if(ready.wait_for(std::chrono::seconds(5))!=std::future_status::ready){unblock.set_value();writer.get();check(false,"reactor wait probe writer not held");}
  std::atomic<bool> cancel{false};std::atomic<unsigned> checks{0};
  acquire=std::async(std::launch::async,[&]{try{catalogs.acquire("probe",gen,request,"block",[&]{++checks;if(cancel)throw ExecutionCancelled("reactor wait probe cancelled");});return false;}catch(const ExecutionCancelled&){return true;}});
  const bool waiting=acquire.wait_for(std::chrono::milliseconds(150))!=std::future_status::ready;
  cancel=true;catalogs.release("probe",gen,request);
  const bool cancel_waits=acquire.wait_for(std::chrono::milliseconds(150))!=std::future_status::ready;
  const auto while_held=checks.load();
  other=std::async(std::launch::async,[&]{try{const auto h=catalogs.acquire("probe",gen,"0x"+std::string(64,'b'),"block");catalogs.close("probe",h);return false;}catch(const CapellaInventoryCaptureBusy&){return true;}});
  const bool other_completed=other.wait_for(std::chrono::milliseconds(150))==std::future_status::ready;
  unblock.set_value();const bool writer_published=writer.get(),cancelled=acquire.get();
  const bool other_busy=other.get();
  const auto recovered=catalogs.acquire("recovery",catalogs.generation(),"0x"+std::string(64,'c'),"block");catalogs.close("recovery",recovered);
  check(waiting&&cancel_waits&&while_held==0&&other_completed&&other_busy&&cancelled&&!writer_published&&reactor.current().token==primary,"reactor wait counterexample not reproduced");
  std::cout<<Json{{"scenario","catalog-cancellation-waits-for-reactor"},{"acquisitionWaited",waiting},{"cancellationStillWaitingWhileWriterHeld",cancel_waits},{"checkpointCallsWhileWriterHeld",while_held},{"otherCaptureRejectedBusy",other_busy},{"cancelledCaptureDrained",cancelled},{"writerUnpublished",!writer_published},{"primaryUnchanged",true},{"capacityRecovered",true},{"counterexample",true},{"wanCauseProven",false},{"networkQualified",false}}.dump()<<std::endl;
 }
