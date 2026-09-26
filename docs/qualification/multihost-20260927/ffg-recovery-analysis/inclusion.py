"""Read admitted archived Bellatrix blocks; diagnostic, not SSZ validation."""
import collections
import gzip
import json
import sys

p = json.loads(gzip.decompress(open(sys.argv[1], 'rb').read()))
blocks = {record[0]: record for record in p['store']['blocks']}
root = p['store']['finalized'][1]
ancestors = set()
while root in blocks and root not in ancestors:
    ancestors.add(root)
    root = blocks[root][1][2]
rows = []
for root, wire in p['signedBlocks']:
    slot = int(blocks[root][1][0], 16)
    if root not in ancestors or not 56 <= slot <= 72:
        continue
    b = bytes.fromhex(wire[2:])
    def number(at, width=4):
        assert 0 <= at <= len(b) - width
        return int.from_bytes(b[at:at+width], 'little')
    message = number(0)
    body = message + number(message + 80)
    start, end = body + number(body + 208), body + number(body + 212)
    assert body <= start <= end <= len(b)
    groups = collections.Counter()
    if start < end:
        first = number(start)
        assert first > 0 and first % 4 == 0 and first <= end - start
        count = first // 4
        for i in range(count):
            off = start + number(start + 4*i)
            stop = start + number(start + 4*(i+1)) if i+1 < count else end
            assert start + first <= off <= stop <= end and stop-off >= 228
            vote_slot = number(off+4, 8)
            assert vote_slot < slot
            groups[(vote_slot, number(off+52, 8), number(off+92, 8))] += 1
    rows.append(dict(slot=slot, root=root, included=sum(groups.values()), groups=[
        dict(voteSlot=v, sourceEpoch=s, targetEpoch=t, delaySlots=slot-v, count=n)
        for (v,s,t),n in sorted(groups.items())]))
print(json.dumps(sorted(rows, key=lambda r:r['slot']), indent=2))
