"""Read-only regression checks against explicitly supplied private save fixtures."""
import sys, struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from repair_inventory import repair, snapshot, container, checksum, HEADER
import zstandard

def rejected(action):
    try: action()
    except (AssertionError, StopIteration): return
    raise AssertionError('Unsafe input was accepted')

def replace_snapshot(b, raw):
    sec,_,_=snapshot(b);toc,sections=container(b)
    payload=HEADER+zstandard.ZstdCompressor().compress(raw);delta=len(payload)-sec['size']
    out=bytearray(b[:sec['offset']]+payload+b[sec['offset']+sec['size']:])
    struct.pack_into('<I',out,8,len(out));struct.pack_into('<I',out,40,toc+delta-40)
    for s in sections:
        at=s['entry']+delta
        if s['name']==sec['name']:struct.pack_into('<II',out,at+12,len(payload),checksum(payload))
        elif s['offset']>sec['offset']:struct.pack_into('<I',out,at+8,s['offset']+delta)
    return bytes(out)

current=Path(sys.argv[1]).read_bytes();previous=Path(sys.argv[2]).read_bytes()
fixed,report=repair(current,previous)
assert report['pagesBefore']==13 and report['pagesAfter']==5
assert report['existingRowsPreserved']==18 and report['equippedRowsPreserved']==10
assert len(report['recovered'])==4 and repair(fixed,previous)[0]==fixed
broken=bytearray(current);broken[100]^=1;rejected(lambda:repair(broken,previous))
other=bytearray(previous);other[12]^=1;rejected(lambda:repair(current,other))
_,raw,inv=snapshot(current)
overflow=bytearray(raw);overflow[inv['rows'][0]['start']]=40
rejected(lambda:repair(replace_snapshot(current,overflow),previous))
duplicate=bytearray(raw);duplicate[inv['rows'][1]['start']]=inv['rows'][0]['slot']
rejected(lambda:repair(replace_snapshot(current,duplicate),previous))
partial=bytearray(raw);struct.pack_into('<I',partial,inv['rows'][0]['start']+1,3051414827)
rejected(lambda:repair(replace_snapshot(current,partial),previous))
assert container(fixed) and snapshot(fixed)[2]['backpackPages']==2
print('PASS: exact item/equipment preservation, four-instance recovery, checksums, unchanged unrelated sections, idempotence; checksum, UUID, overflow, duplicate and partial-recovery rejection')
