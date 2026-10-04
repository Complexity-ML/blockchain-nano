import {test} from 'node:test';
import assert from 'node:assert/strict';
import {selectRestoreRuntime} from './restore-runtime.mjs';
test('packaged restoration selects installed binary and root offline tools, then only restored-copy owner',()=>{assert.deepEqual(selectRestoreRuntime({buildVolume:false,packaged:true}),{nano:'/usr/local/bin/nano',archiveArgs:['--user','0:0','--entrypoint','bash'],archiveCommand:[],ownerArgs:['10001:10001'],auditArgs:['--user','0:0']});});
test('build volume preserves selected binary and existing ownership',()=>{assert.deepEqual(selectRestoreRuntime({buildVolume:true,packaged:false,buildNano:'/build/frozen-amd64/nano'}),{nano:'/build/frozen-amd64/nano',archiveArgs:[],archiveCommand:['bash'],ownerArgs:[],auditArgs:[]});});
for(const modes of [{buildVolume:false,packaged:false},{buildVolume:true,packaged:true},{buildVolume:1,packaged:false},{buildVolume:false,packaged:'yes'}])test('refuses missing, mixed or untyped modes '+JSON.stringify(modes),()=>assert.throws(()=>selectRestoreRuntime(modes)));
for(const path of ['/usr/local/bin/nano','/build/../nano','/build/./nano','/build/-option/nano','/build/a b/nano',undefined])test('refuses unreviewed build path '+String(path),()=>assert.throws(()=>selectRestoreRuntime({buildVolume:true,packaged:false,buildNano:path})));
