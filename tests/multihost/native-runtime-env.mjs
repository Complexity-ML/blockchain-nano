// Pure environment policy for the fresh launcher's macOS native programs.
// It does not inspect dyld, shared libraries, files or the current process.
const elfLoaderControls=new Set(['LD_PRELOAD','LD_LIBRARY_PATH','LD_AUDIT']);
const childOwnedControls=new Set(['NANO_SUPERVISOR_PID','NANO_NETWORK_TOKEN']);
const need=(value,message)=>{if(!value)throw Error('Fresh native environment: '+message);};

export function nativeRuntimeEnvironment(parent,overrides={}){
 const values=new Map();
 for(const [label,input] of [['parent',parent],['overrides',overrides]]){
  need(input!==null&&typeof input==='object'&&!Array.isArray(input),label+' must be an environment object');
  for(const [name,value] of Object.entries(input)){
   need(/^[A-Za-z_][A-Za-z0-9_]*$/.test(name),'noncanonical environment name');
   need(typeof value==='string'&&!value.includes('\0'),'environment values must be strings without NUL');
   need(!name.startsWith('DYLD_')&&!elfLoaderControls.has(name),'loader control is forbidden: '+name);
   need(!childOwnedControls.has(name),'managed-child control is forbidden: '+name);
   values.set(name,value);
  }
 }
 // A null prototype also prevents an ambient Object.prototype property from
 // becoming an inherited child_process environment entry. Values are strings.
 const environment=Object.create(null);
 for(const name of [...values.keys()].sort())environment[name]=values.get(name);
 return Object.freeze(environment);
}
