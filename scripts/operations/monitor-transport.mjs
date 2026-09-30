import {execFile} from 'node:child_process';
import {promisify} from 'node:util';
const exec=promisify(execFile),quote=s=>"'"+s.replaceAll("'","'\\''")+"'";
// Configuration validation happens before constructing this adapter. Shell
// quoting applies only to the remote SSH command; local execution uses argv.
export function transportRequest(node,config,execute=exec){
 return async(url,options={})=>{
  if(new URL(url).origin!==node.url)throw Error('Unconfigured transport origin');
  const args=['exec',node.container,'curl','-sS','--max-time',String(config.timeoutMs/1000),'-w','\n%{http_code}'];
  for(const [key,value] of Object.entries(options.headers??{}))args.push('-H',key+': '+value);
  if(options.method==='POST')args.push('--data',options.body);
  args.push(url);
  const remote=node.kind==='ssh-docker',binary=remote?'ssh':'docker';
  const argv=remote?['-o','BatchMode=yes','-o','ConnectTimeout=5',node.ssh,['sudo','-n','docker',...args].map(quote).join(' ')]:args;
  const {stdout}=await execute(binary,argv,{timeout:config.timeoutMs+1000,maxBuffer:config.maximumResponseBytes+64,signal:options.signal});
  const at=stdout.lastIndexOf('\n'),status=Number(stdout.slice(at+1)),body=stdout.slice(0,at);
  if(at<0||!Number.isInteger(status)||status<100||status>599||Buffer.byteLength(body)>config.maximumResponseBytes)throw Error('Invalid/bounded transport response');
  return new Response(body,{status});
 };
}
