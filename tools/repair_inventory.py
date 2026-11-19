"""Offline repair for the pinned character snapshot v5. Writes only a NEW output.
Requires zstandard and xxhash. No game process or Steam save is modified here.
"""
import argparse, hashlib, json, struct
from collections import Counter
from pathlib import Path
import zstandard, xxhash
from inventory_snapshot import parse
HEADER=b'S2BH\x02\0\0\0\x09\x01\x05\0\0\0'
CUSTOM={1197164494,2311625150,3051414827}
def checksum(b):return xxhash.xxh3_64_intdigest(b)&0xffffffff
def container(b):
    assert b[:8]==b'SV2C\x02\0\x30\0' and struct.unpack_from('<I',b,8)[0]==len(b),'Unsupported KSD header'
    toc=40+struct.unpack_from('<I',b,40)[0];n=struct.unpack_from('<I',b,44)[0]
    assert 0<n<100 and toc+n*20<=len(b)
    sections=[];end=48
    for i in range(n):
        at=toc+i*20;no,nl,off,size,h=struct.unpack_from('<IIIII',b,at)
        assert 0<nl<=31 and at+no>=toc+n*20 and at+no+nl<=len(b) and off>=end and off+size<=toc
        name=b[at+no:at+no+nl].decode('ascii');data=b[off:off+size]
        assert checksum(data)==h,('Section checksum mismatch',name)
        sections.append(dict(name=name,offset=off,size=size,entry=at,data=data));end=off+size
    assert end<=toc
    return toc,sections
def snapshot(b):
    _,sections=container(b);section=next(s for s in sections if s['name']=='player_character')
    assert section['data'].startswith(HEADER),'Only native compressed character snapshot v5 is supported'
    decoder=zstandard.ZstdDecompressor().decompressobj();raw=decoder.decompress(section['data'][14:])
    assert decoder.eof and not decoder.unused_data,'Extended saves need their own migration path'
    return section,raw,parse(raw)
def repair(b,previous=None):
    sec,raw,inv=snapshot(b);assert inv['pages']>=5
    assert inv['backpackCount']==0,'This one-time repair requires an empty backpack inventory'
    assert all(r['slot']<40 for r in inv['rows']),'Refuse to truncate occupied overflow slots'
    assert len({r['slot'] for r in inv['rows']})==len(inv['rows'])
    recovered=[];used={r['slot'] for r in inv['rows']}
    if previous:
        assert previous[12:28]==b[12:28],'Different character UUID'
        _,priorRaw,prior=snapshot(previous)
        for item in CUSTOM:
            old=[r for r in prior['rows'] if r['id']==item and r['count']]
            current=[r for r in inv['rows'] if r['id']==item and r['count']]
            if current: # Never duplicate instances already present.
                assert len(current)==len(old),'Ambiguous partial recovery';continue
            for r in old:
                slot=r['slot'] if r['slot']<40 and r['slot'] not in used else next((i for i in range(40) if i not in used),None)
                assert slot is not None,'Not enough space for all recovered items'
                wire=bytearray(priorRaw[r['start']:r['end']]);wire[0]=slot;used.add(slot)
                recovered.append((r,bytes(wire)))
    out=bytearray(raw);out[inv['at']]=5;out[inv['at']+1]=inv['count']+len(recovered)
    out[inv['end']:inv['end']]=b''.join(w for _,w in recovered)
    checked=parse(out)
    assert checked['pages']==5 and checked['equipped']==inv['equipped'] and checked['refs']==inv['refs'] and checked['tail']==inv['tail']
    assert checked['rows'][:len(inv['rows'])]==inv['rows'],'An existing item changed'
    for (old,_),new in zip(recovered,checked['rows'][len(inv['rows']):]):
        a=bytearray.fromhex(old['row']);c=bytearray.fromhex(new['row']);a[4]=c[4]=0;assert a==c
    if bytes(out)==raw:return b,dict(changed=False)
    payload=HEADER+zstandard.ZstdCompressor(level=3).compress(bytes(out));delta=len(payload)-sec['size']
    toc,sections=container(b)
    rebuilt=bytearray(b[:sec['offset']]+payload+b[sec['offset']+sec['size']:])
    struct.pack_into('<I',rebuilt,8,len(rebuilt));struct.pack_into('<I',rebuilt,40,toc+delta-40)
    for s in sections:
        at=s['entry']+delta
        if s['name']==sec['name']:
            struct.pack_into('<II',rebuilt,at+12,len(payload),checksum(payload))
        elif s['offset']>sec['offset']:struct.pack_into('<I',rebuilt,at+8,s['offset']+delta)
    _,verified=container(rebuilt)
    for s,v in zip(sections,verified):
        if s['name']!=sec['name']:assert s['data']==v['data'],'Unrelated KSD section changed'
    _,againRaw,again=snapshot(rebuilt);assert againRaw==out and again==checked
    return bytes(rebuilt),dict(changed=True,pagesBefore=inv['pages'],pagesAfter=5,existingRowsPreserved=len(inv['rows']),equippedRowsPreserved=len(inv['equipped']),recovered=[dict(item=r['id'],count=r['count'],slot=w[0]) for r,w in recovered],inputSha256=hashlib.sha256(b).hexdigest(),outputSha256=hashlib.sha256(rebuilt).hexdigest())
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--input',required=True,type=Path);ap.add_argument('--output',required=True,type=Path);ap.add_argument('--recover',type=Path);args=ap.parse_args()
    assert args.input.resolve()!=args.output.resolve() and not args.output.exists(),'Output must be a new file'
    b=args.input.read_bytes();previous=args.recover.read_bytes() if args.recover else None
    result,report=repair(b,previous)
    assert repair(result,previous)[0]==result,'Repair is not idempotent'
    args.output.write_bytes(result);args.output.with_suffix('.repair.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
