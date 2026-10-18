EXPECTED_SHA='f87d25426de26b3f04d56f23ffc057bb62b0c646eab00392bb561078753ef234'
EXPECTED_BYTES=39285

from pathlib import Path
import hashlib, os
p = Path('/home/ubuntu/main-http-persistence-tools-20261018-01/host.py')
assert os.getuid() == os.getgid() == 1000
assert p.resolve(strict=True) == p and p.stat().st_nlink == 1 and p.stat().st_mode & 0o777 == 0o400
b = p.read_bytes()
assert len(b) == EXPECTED_BYTES and hashlib.sha256(b).hexdigest() == EXPECTED_SHA
os.execv('/usr/bin/python3', ['/usr/bin/python3', str(p), '--revision',
 'bc99b0ec2cf8bc6bcb21a26ec980ef2c4e8573a8', '--python-runtime-control',
 '/home/ubuntu/main-http-python-provider-root-20261018-02/control.json',
 '6d1969aac717c7f980a6845756158d52fe5ce0436c1c13511094ac019df97100', '241262'])
