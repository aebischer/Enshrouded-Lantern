import struct,sys,json
from pathlib import Path
class Reader:
 def __init__(self,b):self.b=b;self.p=0
 def take(self,n):
  assert self.p+n<=len(self.b);v=self.b[self.p:self.p+n];self.p+=n;return v
 def u(self,n):return int.from_bytes(self.take(n),'little')
 def row(self):
  start=self.p;r=bytearray(56);r[4]=self.u(1);r[8:12]=self.take(4);r[:2]=self.take(2);flags=self.u(1);assert flags<64
  for bit,off,size,present in [(1,2,1,3),(2,12,8,20),(4,24,8,32),(8,36,4,40),(16,5,2,7),(32,44,5,52)]:
   if flags&bit:r[off:off+size]=self.take(size);r[present]=1
  return dict(start=start,end=self.p,slot=r[4],id=int.from_bytes(r[8:12],'little'),count=int.from_bytes(r[:2],'little'),row=r.hex())
def parse(b):
 q=Reader(b);q.take(1+9*4+1+7*4);n=q.u(4);q.take(n*5);effects=q.u(4)
 for _ in range(effects):
  q.take(20);flags=q.u(2);assert flags<64
  for i,size in enumerate([1,4,4,1,8,52]):
   if flags&(1<<i):q.take(size)
 mask=q.u(4);equipped={}
 for i in range(16):
  if mask&(1<<i):equipped[i]=q.row()
 refs={}
 for i in range(16,20):
  if mask&(1<<i):refs[i]=(q.p,q.u(2))
 at=q.p;pages=q.u(1);count=q.u(1);bp=q.u(1);bc=q.u(1);assert pages<=16 and bp<=16 and count+bc<=248
 rows=[q.row() for _ in range(count+bc)]
 return dict(at=at,pages=pages,count=count,backpackPages=bp,backpackCount=bc,rows=rows,equipped=equipped,refs=refs,end=q.p,tail=b[q.p:].hex())
if __name__=='__main__':
 import argparse
 ap=argparse.ArgumentParser();ap.add_argument('snapshot',type=Path);args=ap.parse_args()
 print(json.dumps(parse(args.snapshot.read_bytes()),indent=2))

