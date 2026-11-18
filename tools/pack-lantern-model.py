"""Build Lantern's private KFC4 model resources from prepared mesh/texture data.
Format reference: Baik90/EnshroudedBlenderTools (KFC4_SUPPORT.md).
No game archive is modified. Requires the locally extracted hammer descriptors.
"""
from pathlib import Path
import struct,json,hashlib,uuid,math,sys
ROOT=Path(__file__).resolve().parents[2];gold='--gold' in sys.argv
folder='lantern-gold3d' if gold else 'lantern3d'
OUT=ROOT/'Lantern/assets'/folder;WORK=ROOT/'work'/folder
def guid(name):return uuid.uuid5(uuid.NAMESPACE_URL,('lantern/gold3d/v1/' if gold else 'lantern/belt3d/v1/')+name).bytes_le
MODEL,MATERIAL,TEMPLATE=[guid(s) for s in ['model','material','template']]
def arr(b,o,stride):
 r,n=struct.unpack_from('<iI',b,o);assert 0<=o+r<=len(b) and o+r+n*stride<=len(b);return o+r,n
def append(b,field,data,count):
 while len(b)%16:b.append(0)
 pos=len(b);b.extend(data);struct.pack_into('<iI',b,field,pos-field,count)
def content_hash(data):return struct.pack('<I',len(data))+hashlib.sha256(b'Lantern Belt v1'+data).digest()[:12]
contents=[]
def content(data):
 h=content_hash(data)
 if not any(k==h for k,v in contents):contents.append((h,data))
 return h
m=json.loads((OUT/'mesh.json').read_text());rec=m['records'];ids=m['indices'];assert len(rec)<65536 and all(0<=i<len(rec) for i in ids)
lo=[min(r[0][a] for r in rec)-.00001 for a in range(3)];hi=[max(r[0][a] for r in rec)+.00001 for a in range(3)];scale=[hi[a]-lo[a] for a in range(3)];center=[(lo[a]+hi[a])/2 for a in range(3)];radius=max(math.dist(r[0],center) for r in rec)+.00002
planes=[bytearray() for _ in range(5)]
def norm(v,w):
 assert all(math.isfinite(x) for x in v)
 length=math.sqrt(sum(x*x for x in v));assert length>.5
 return sum((max(-511,min(511,round(x/length*511)))&1023)<<(10*i) for i,x in enumerate(v))|((1 if w>=0 else 3)<<30)
for pos,n,t,sign,uv in rec:
 if sum(x*x for x in t)<.25:
  axis=[1,0,0] if abs(n[0])<.9 else [0,1,0]
  t=[n[1]*axis[2]-n[2]*axis[1],n[2]*axis[0]-n[0]*axis[2],n[0]*axis[1]-n[1]*axis[0]];sign=1
 q=[round((pos[i]-lo[i])/scale[i]*0x1fffff) for i in range(3)];assert all(0<=x<=0x1fffff for x in q)
 planes[0]+=struct.pack('<II',(q[0]<<11)|(q[1]>>10),q[2]|((q[1]&1023)<<21));planes[1]+=struct.pack('<I',norm(n,1));planes[2]+=struct.pack('<I',norm(t,sign));planes[3]+=bytes([255]*4);planes[4]+=struct.pack('<2e',*uv)
vertices=b''.join(planes);data=vertices+struct.pack('<%dH'%len(ids),*ids);data+=bytes(-len(data)%16)
model=bytearray((ROOT/'work/resource-f7f94f08dd4a674abe257c8f2b62552f-0x177394c7-0.bin').read_bytes())
struct.pack_into('<3f',model,16,*lo);struct.pack_into('<3f',model,32,*hi);struct.pack_into('<4f',model,48,*center,radius);struct.pack_into('<3f',model,64,*scale);struct.pack_into('<3f',model,76,*lo);struct.pack_into('<f',model,88,radius)
ma,mc=arr(model,92,20);assert mc==1;model[ma:ma+16]=MATERIAL
for field in [100,144,212,256]:struct.pack_into('<II',model,field,0,0)
model[128:144]=bytes(16);struct.pack_into('<I',model,152,0);model[160]=0;model[264:268]=bytes(4)
meshes=bytearray();lods=bytearray()
for i,(start,count) in enumerate(m['lods']):
 mesh=bytearray(56);struct.pack_into('<4f',mesh,0,*center,radius);struct.pack_into('<4I',mesh,16,0,len(rec),start,count);mesh[52:56]=bytes([4,24,0,1]);meshes+=mesh;lods+=struct.pack('<IIf',i,1,[0,.018,.045][i])
append(model,108,meshes,len(m['lods']));append(model,116,lods,len(m['lods']));model[272:288]=content(data)
for off,start,count in [(304,len(vertices),len(ids)*2),(312,0,0),(320,0,len(vertices)),(328,0,0),(336,0,0)]:struct.pack_into('<II',model,off,start,count)
append(model,0,b'Lantern_BeltLight_Model\0',len('Lantern_BeltLight_Model'))
material=bytearray((ROOT/'work/pincer3d/hammer-material.bin').read_bytes());images,n=arr(material,16,96);assert n==4
for i,name in enumerate(['emissive','albedo','normal','rma']):
 dds=(WORK/(name+'.dds')).read_bytes();assert dds[:4]==b'DDS '
 assert dds[84:88]==b'DX10' and struct.unpack_from('<I',dds,128)[0]=={'emissive':95,'albedo':99,'normal':83,'rma':71}[name], 'texture format must match native material'
 h,w=struct.unpack_from('<II',dds,12);levels=struct.unpack_from('<I',dds,28)[0];head=148 if dds[84:88]==b'DX10' else 128;raw=dds[head:];block=8 if name=='rma' else 16
 sizes=[max(1,(max(1,w>>l)+3)//4)*max(1,(max(1,h>>l)+3)//4)*block for l in range(levels)];assert sum(sizes)==len(raw)
 o=images+i*96+8;struct.pack_into('<HHB',material,o,w,h,levels);material[o+8:o+24]=content(raw)
 ranges=[];at=0
 for size in sizes:
  if len(raw)-at<=4096:break
  ranges.append((at,len(raw)-at));at+=size
 material[o+40:o+56]=content(raw[at:]);append(material,o+72,b''.join(struct.pack('<II',*r) for r in ranges),len(ranges))
append(material,32,b'Lantern_BeltLight_Material\0',len('Lantern_BeltLight_Material'))
resources=[(MODEL,0x177394c7,bytes(model)),(MATERIAL,0xaee1894b,bytes(material))]
items=resources+[(h,0,d) for h,d in contents];pack=bytearray(b'LAN3D001'+struct.pack('<I',len(items))+bytes(32*len(items)))
for i,(g,t,d) in enumerate(items):
 while len(pack)%16:pack.append(0)
 struct.pack_into('<16sIIII',pack,12+i*32,g,t,0,len(pack),len(d));pack+=d
(OUT/'lantern.pack').write_bytes(pack)
(OUT/'build-report.json').write_text(json.dumps({'model':MODEL.hex(),'material':MATERIAL.hex(),'vertices':len(rec),'triangles_per_lod':[v[1]//3 for v in m['lods']],'bounds':[lo,hi],'pack_bytes':len(pack),'sha256':hashlib.sha256(pack).hexdigest()},indent=2))
print((OUT/'build-report.json').read_text())
