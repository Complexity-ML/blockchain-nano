"""Shared strict byte/FD9 and USTAR inventory logic. No operational entrypoint."""
import hashlib,json,os,pathlib,re,stat,time
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
MAX_ENTRIES=4096
MAX_BYTES=128*1024*1024
def need(ok,why):
 if not ok:raise ValueError(why)
def pin(raw):return {'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw)}
byte_pin=pin
def json_body(raw):
 def pairs(items):
  d={}
  for k,v in items:
   need(k not in d,'duplicate JSON key');d[k]=v
  return d
 return json.loads(raw,object_pairs_hook=pairs,parse_constant=lambda x:(_ for _ in()).throw(ValueError('nonfinite JSON')))
def material(path):
 p=pathlib.Path(path);need(p.is_absolute()and p.resolve(strict=True)==p,'canonical physical absolute path')
 fd=os.open(p,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  a=os.fstat(fd);r={k:getattr(a,k)for k in F};need(stat.S_ISREG(a.st_mode)and a.st_nlink==1 and 0<=a.st_size<=MAX_BYTES,'bounded regular single-link file')
  data=[];n=0;h=hashlib.sha256()
  while b:=os.read(fd,65536):
   n+=len(b);need(n<=a.st_size,'file grew');data.append(b);h.update(b)
  need(n==a.st_size and r=={k:getattr(os.fstat(fd),k)for k in F}=={k:getattr(p.lstat(),k)for k in F},'full FD9/path changed')
  return dict(r,sha256=h.hexdigest()),b''.join(data)
 finally:os.close(fd)
def snapshot(root,owner=None):
 root=pathlib.Path(root);need(root.is_absolute()and root.resolve(strict=True)==root,'canonical snapshot root');rows={};total=0
 def visit(p,rel,depth):
  nonlocal total
  need(depth<=128 and len(rows)<MAX_ENTRIES,'bounded snapshot entries/depth');s=p.lstat();r={k:getattr(s,k)for k in F}
  if owner is not None:need((s.st_uid,s.st_gid)==owner,'snapshot owner')
  if stat.S_ISDIR(s.st_mode):
   need(stat.S_IMODE(s.st_mode)==0o700,'snapshot directory mode0700');rows[rel]=dict(r,kind='directory')
   with os.scandir(p)as entries:names=sorted(x.name for x in entries)
   for name in names:
    need(name not in ('.','..')and len(name)<=255,'plain snapshot leaf');visit(p/name,rel+'/'+name if rel else name,depth+1)
   need(r=={k:getattr(p.lstat(),k)for k in F},'snapshot directory changed')
  else:
   need(stat.S_ISREG(s.st_mode)and stat.S_IMODE(s.st_mode)==0o600,'snapshot file mode0600/type');row,_=material(p);rows[rel]=dict(row,kind='file');total+=row['st_size'];need(total<=MAX_BYTES,'snapshot payload bytes')
 visit(root,'',0);return rows

def validate_tar(path, trusted_manifest, deadline, manifest_pin, owner=(1000,1000), profile_pin=None):
    MANIFEST_PIN=manifest_pin
    PROFILE_PIN=profile_pin or json_body(trusted_manifest)["profile"]
    """Strict USTAR scanner; never extract, open a DB or interpret DB payloads."""
    entries, total, manifest_raw = {}, 0, None

    def checkpoint():
        need(time.monotonic() < deadline, 'export inventory deadline')

    def text(raw):
        head, sep, tail = raw.partition(b'\0')
        need(not sep or not any(tail), 'noncanonical USTAR text padding')
        need(all(32 <= x <= 126 for x in head), 'plain ASCII USTAR text')
        return head.decode('ascii')

    def octal(raw):
        value = raw.strip(b'\0 ')
        need(value and re.fullmatch(b'[0-7]+', value), 'plain octal USTAR number')
        return int(value, 8)

    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    with os.fdopen(fd, 'rb') as archive:
        while True:
            checkpoint()
            header = archive.read(512)
            need(len(header) == 512, 'complete USTAR header')
            if header == bytes(512):
                need(archive.read(512) == bytes(512), 'two USTAR end blocks')
                while chunk := archive.read(65536):
                    checkpoint()
                    need(not any(chunk), 'no trailing nonzero archive material')
                break
            need(header[257:263] == b'ustar\0' and header[263:265] == b'00', 'USTAR only; no PAX/GNU extensions')
            need(sum(header[:148]) + 8 * 32 + sum(header[156:]) == octal(header[148:156]), 'USTAR checksum')
            kind = header[156:157]
            need(kind in (b'0', b'\0', b'5') and not text(header[157:257]), 'regular files/directories only; no links or devices')
            prefix, leaf = text(header[345:500]), text(header[:100])
            name = prefix + '/' + leaf if prefix else leaf
            if kind == b'5' and name.endswith('/'):
                name = name[:-1]
            parts = name.split('/')
            need(name and len(name) <= 4096 and len(parts) <= 129
                 and all(x and x not in ('.', '..') and '\\' not in x for x in parts)
                 and not name.startswith('/') and parts[0] == 'public-cohort'
                 and pathlib.PurePosixPath(name).as_posix() == name, 'canonical relative member under public-cohort')
            need(name not in entries and len(entries) < MAX_ENTRIES, 'unique bounded tar names')
            mode, uid, gid = octal(header[100:108]), octal(header[108:116]), octal(header[116:124])
            size, mtime = octal(header[124:136]), octal(header[136:148])
            need((uid, gid) == owner and mode <= 0o777 and mode & 0o022 == 0, 'fixture uid/gid and non-writable-to-others modes')
            directory = kind == b'5'
            need(not directory or size == 0, 'directory has no payload')
            total += size
            need(total <= MAX_BYTES and size <= MAX_BYTES, 'aggregate raw file payload bound')
            digest, remaining, chunks = hashlib.sha256(), size, []
            capture_manifest = name == 'public-cohort/manifest.json'
            if capture_manifest:
                need(not directory and size == MANIFEST_PIN['bytes'], 'exact manifest member size/type')
            while remaining:
                checkpoint()
                chunk = archive.read(min(65536, remaining))
                need(chunk, 'complete member payload')
                remaining -= len(chunk)
                digest.update(chunk)
                if capture_manifest:
                    chunks.append(chunk)
            padding = (-size) % 512
            need(archive.read(padding) == bytes(padding), 'zero USTAR member padding')
            record = {'type': 'directory' if directory else 'file', 'mode': mode,
                      'uid': uid, 'gid': gid, 'mtime': mtime, 'bytes': size}
            if not directory:
                record['sha256'] = digest.hexdigest()
            entries[name] = record
            if capture_manifest:
                manifest_raw = b''.join(chunks)

    need(manifest_raw == trusted_manifest and byte_pin(manifest_raw) == MANIFEST_PIN, 'tar manifest equals independently held bytes')
    need(entries.get('public-cohort', {}).get('type') == 'directory', 'explicit public-cohort directory')
    children = {name: [] for name, row in entries.items() if row['type'] == 'directory'}
    for name in entries:
        if name == 'public-cohort':
            continue
        parent = name.rsplit('/', 1)[0]
        need(parent in children, 'every parent is explicitly a directory')
        children[parent].append(name)
    for values in children.values():
        values.sort(key=lambda value: value.rsplit('/', 1)[1])
    need({x.rsplit('/', 1)[1] for x in children['public-cohort']}
         == {'alpha', 'beta', 'profile.json', 'manifest.json'}, 'exact four top-level entries')
    manifest = json_body(trusted_manifest)
    need(manifest.get('format') == 'nano-physical-authenticated-public-cohort-v1'
         and manifest.get('syntheticPublicFixture') is True
         and manifest.get('newTestOnlySignaturesCreated') == 14, 'known public fixture manifest')
    cohorts = manifest['cohorts']
    need(type(cohorts) is list and len(cohorts) == 2
         and [x['name'] for x in cohorts] == ['alpha', 'beta']
         and [x['children'] for x in cohorts] == [2, 3], 'exact two committed cohort names')
    roles, descriptions = {'primary', 'witness', 'signer', 'signer-witness'}, {}
    for cohort in cohorts:
        root = 'public-cohort/' + cohort['name']
        need(entries[root]['type'] == 'directory' and root in children
             and {x.rsplit('/', 1)[1] for x in children[root]} == roles
             and set(cohort['physical']) == roles, 'exact four committed role directories')
        for role in sorted(roles):
            role_root = root + '/' + role
            need(entries[role_root]['type'] == 'directory', 'explicit role directory')
            portable, aggregate = [['', 'directory']], 0

            def visit(parent, relative, depth):
                nonlocal aggregate
                checkpoint()
                need(depth <= 128, 'portable DFS depth bound')
                for child in children[parent]:
                    leaf = child.rsplit('/', 1)[1]
                    rel = relative + '/' + leaf if relative else leaf
                    item = entries[child]
                    if item['type'] == 'directory':
                        portable.append([rel, 'directory'])
                        visit(child, rel, depth + 1)
                    else:
                        aggregate += item['bytes']
                        portable.append([rel, 'file', item['bytes'], item['sha256']])
            visit(role_root, '', 0)
            raw = json.dumps(portable, ensure_ascii=False, separators=(',', ':'), allow_nan=False).encode()
            description = {'sha256': hashlib.sha256(raw).hexdigest(), 'entries': len(portable), 'bytes': aggregate}
            need(description == cohort['physical'][role], 'exact C++ portable role inventory: ' + cohort['name'] + '/' + role)
            descriptions[cohort['name'] + '/' + role] = description
    profile = entries['public-cohort/profile.json']
    need(profile['type'] == 'file'
         and {'sha256': profile['sha256'], 'bytes': profile['bytes']} == manifest['profile'] == PROFILE_PIN,
         'profile payload equals independently known pin and manifest')
    return {'format': 'nano-public-cohort-export-inventory-v1', 'entries': entries,
            'entryCount': len(entries), 'payloadBytes': total, 'portableRoleInventories': descriptions,
            'manifestPin': MANIFEST_PIN, 'profilePin': PROFILE_PIN, 'extracted': False}

