"""Pinned IPv6 transport for the already authorized VPS, no shell parsing."""
import os,sys
TARGET='ubuntu@145.239.199.166'
HOST='ubuntu@2001:41d0:404:200::9c34'
PIN=['-o','AddressFamily=any','-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','HostKeyAlias=145.239.199.166']
FORBIDDEN=('stricthostkeychecking','hostkeyalias','userknownhostsfile','globalknownhostsfile','proxycommand','proxyjump','addressfamily')

def plan(tool,args):
    if tool not in ('ssh','scp'):raise ValueError('only SSH/SCP are permitted')
    if not args:raise ValueError('missing transport arguments')
    for i,arg in enumerate(args):
        if arg in ('-4','-6','-F','-J') or arg.startswith('-F') or arg.startswith('-J'):
            raise ValueError('transport config/proxy overrides are forbidden')
        value=args[i+1] if arg=='-o' and i+1<len(args) else arg[2:] if arg.startswith('-o') else ''
        if any(value.lower().startswith(k) for k in FORBIDDEN):raise ValueError('host pin override is forbidden')
    converted=[];replaced=0
    for arg in args:
        if tool=='ssh' and arg==TARGET:
            converted.append(HOST);replaced+=1
        elif tool=='scp' and arg.startswith(TARGET+':'):
            converted.append('ubuntu@[2001:41d0:404:200::9c34]:'+arg[len(TARGET)+1:]);replaced+=1
        else:converted.append(arg)
    if replaced!=1:raise ValueError('requires exactly one authorized VPS endpoint')
    return ['/usr/bin/'+tool,*PIN,*converted]

if __name__=='__main__':
    try:argv=plan(sys.argv[1],sys.argv[2:])
    except ValueError as error:print(str(error),file=sys.stderr);sys.exit(64)
    os.execv(argv[0],argv)
