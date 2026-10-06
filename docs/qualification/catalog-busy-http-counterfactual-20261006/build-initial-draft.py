from pathlib import Path
import shutil,json,subprocess,hashlib
base=Path('/private/tmp/nano-catalog-busy-http-baseline-20261006');root=Path('/private/tmp/nano-catalog-busy-http-draft-20261006');root.mkdir(exist_ok=False);shutil.copytree(base/'network',root/'network');shutil.copy2(base/'test.cpp',root/'test.cpp')
p=root/'network/core.hpp';s=p.read_text();at='class CoreRequest:public';helper='''// This hint is emitted only for an authenticated-loopback RPC refusal which
// occurs before a capture reservation. A transport failure is never a hint.
inline Json core_capture_busy_hint(const std::string& method,const Json& response){
 if(method!="nano_capellaCatalogAcquire"||!response.is_object()||response.contains("result")||response.value("jsonrpc",Json())!="2.0"||response.value("id",Json())!=1)return nullptr;
 const auto error=response.value("error",Json());
 if(!error.is_object()||error.value("code",Json())!=-32005||error.value("message",Json())!="Capella inventory capture busy")return nullptr;
 return Json{{"nanoCoreCaptureBusy",true}};
}
inline bool core_capture_busy(const Json& value){return value==Json{{"nanoCoreCaptureBusy",true}};}
'''
assert at in s;s=s.replace(at,helper+at,1);old='self->finish(nullptr,reusable);return;}';new='self->finish(response.result()==http::status::ok?core_capture_busy_hint(self->method_,json):Json(nullptr),reusable);return;}';assert old in s;s=s.replace(old,new,1);p.write_text(s)
p=root/'network/capella_catalog_relay.hpp';s=p.read_text();s=s.replace('Reply done_;asio::steady_timer timer_;','Reply done_;asio::steady_timer timer_,capture_retry_timer_;std::chrono::steady_clock::duration capture_retry_{};size_t capture_busy_retries_=0;');s=s.replace('finished_=true;timer_.cancel();','finished_=true;timer_.cancel();capture_retry_timer_.cancel();',1);s=s.replace('closing_=true;desired_success_', 'capture_retry_timer_.cancel();closing_=true;desired_success_',1);s=s.replace('timer_(io),page_size_', 'timer_(io),capture_retry_timer_(io),page_size_',1)
a=' void page(){';method=''' void acquire(){
  if(finished_||closing_)return;
  call("nano_capellaCatalogAcquire",Json::array({token_,kind_,generation_,request_}),callback([](auto& s,Json value){
   if(s.closing_)return;
   if(core_capture_busy(value)){
    if(s.capture_retry_==std::chrono::steady_clock::duration::zero()){s.failure("capture-busy");return;}
    ++s.capture_busy_retries_;s.capture_retry_timer_.expires_after(s.capture_retry_);
    s.capture_retry_timer_.async_wait([self=s.shared_from_this()](auto error){if(!error&&!self->finished_&&!self->closing_)self->acquire();});return;
   }
   if(!value.is_object()||!value.contains("handle")||!root(value["handle"])){s.close(false);return;}
   s.handle_=value["handle"].template get<std::string>();if(s.started_after_.empty())s.page();else s.seek();
  },true));
 }
''';assert a in s;s=s.replace(a,method+a,1)
s=s.replace('void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30)){','void start(std::chrono::steady_clock::duration timeout=std::chrono::seconds(30),std::chrono::steady_clock::duration capture_retry={}){',1);s=s.replace('if(started_)throw', 'if(capture_retry<std::chrono::steady_clock::duration::zero())throw std::invalid_argument("Invalid capture retry interval");capture_retry_=capture_retry;if(started_)throw',1)
old='self.call("nano_capellaCatalogAcquire",Json::array({self.token_,self.kind_,self.generation_,self.request_}),self.callback([](auto& s,Json value){if(!value.is_object()||!root(value.at("handle"))){s.close(false);return;}s.handle_=value.at("handle").template get<std::string>();if(s.started_after_.empty())s.page();else s.seek();},true));';assert old in s;s=s.replace(old,'self.acquire();',1);s=s.replace('{"exports",exports_}', '{"captureBusyRetries",capture_busy_retries_},{"exports",exports_}');p.write_text(s)
c=json.loads((base/'compile-command.json').read_text());c=[x.replace(str(base),str(root)) for x in c];c.insert(1,'-DNANO_ENABLE_CAPTURE_BUSY_RETRY=1');(root/'compile-command.json').write_text(json.dumps(c)+'\n')
with (root/'compile.log').open('w') as f:r=subprocess.run(c,stdout=f,stderr=subprocess.STDOUT)
(root/'compile-exit.json').write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,(root/'compile.log').read_text()[-2000:]
(root/'source-manifest.json').write_text(json.dumps({str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(root.rglob('*')) if p.is_file() and p.suffix in ['.cpp','.hpp']},indent=2)+'\n');print('Draft compiled; only explicit pre-capture busy permits a delayed repeat; total deadline unchanged')
