// Preserve HTTP failure diagnostics; never retry or change a campaign deadline.
export const httpStatusTrailer='\nNANO_HTTP_STATUS:%{http_code}';
export function decodeHttpResponse(text,host,path){
 const match=/\nNANO_HTTP_STATUS:([0-9]{3})$/.exec(text);
 if(!match)throw Error('Missing HTTP response status trailer');
 const status=Number(match[1]),body=text.slice(0,match.index);
 if(status<100||status>599)throw Error('Invalid HTTP response status');
 let result;try{result=JSON.parse(body);}catch{
  if(status>=200&&status<300)throw Error('Invalid successful HTTP JSON response');
 }
 if(status<200||status>=300){
  const message=typeof result?.message==='string'?result.message.replace(/[\x00-\x1f\x7f]/g,' ').slice(0,256):'No JSON server diagnostic';
  const error=Error(`HTTP ${status} from ${host}${path}: ${message}`);
  error.name='HttpReadError';error.status=status;
  error.serverCode=typeof result?.code==='number'?result.code:null;
  error.serverMessage=message;throw error;
 }
 return result;
}
