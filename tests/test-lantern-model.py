"""Independent decoding checks for the private visual pack (no game writes)."""
import sys,types,struct,json,math,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
FIXTURES=Path(os.environ.get('LANTERN_FIXTURES',ROOT/'work'))
REFERENCE=Path(os.environ.get('LANTERN_MODEL_REFERENCE',ROOT/'work/enshrouded-blender-reference/enshrouded_tools/core'))
if not (REFERENCE/'render_model.py').is_file():raise SystemExit('Missing independent model decoder: set LANTERN_MODEL_REFERENCE to enshrouded_tools/core')
p=types.ModuleType('refcore');p.__path__=[str(REFERENCE)];sys.modules['refcore']=p
from refcore.render_model import parse_render_model,decode_lod
from refcore.material import parse_material
base=ROOT/'Lantern/assets'/('lantern-gold3d' if '--gold' in sys.argv else 'lantern3d');b=(base/'lantern.pack').read_bytes();n=struct.unpack_from('<I',b,8)[0];records={}
for i in range(n):
 g,t,part,o,s=struct.unpack_from('<16sIIII',b,12+i*32);assert part==0 and o%16==0 and o+s<=len(b);records[g,t]=b[o:o+s]
modelblob=next(v for (g,t),v in records.items() if t==0x177394c7);model=parse_render_model(modelblob,archive_format='KFC4');render=records[model.render_data_hash,0]
source=json.loads((base/'mesh.json').read_text());maxerror=0
for lod,expected in enumerate(json.loads((base/'build-report.json').read_text())['triangles_per_lod']):
 mesh=decode_lod(model,render,lod);assert len(mesh.triangles)==expected
 # Compare decoded positions, UVs, normals against independent Blender output.
 for i,(pos,uv,normal) in enumerate(zip(mesh.vertices,mesh.uvs,mesh.normals)):
  original=source['records'][i];error=math.dist(pos,original[0]);maxerror=max(error,maxerror);assert error<.000002
  assert abs(uv[0]-original[4][0])<.0005 and abs((1-uv[1])-original[4][1])<.0005
  assert sum(normal[j]*original[1][j] for j in range(3))>.999
 assert all(0<=idx<len(mesh.vertices) for tri in mesh.triangles for idx in tri)
mat=next(v for (g,t),v in records.items() if t==0xaee1894b);parsed=parse_material(mat,archive_format='KFC4');assert len(parsed.textures)==4
r,count=struct.unpack_from('<iI',mat,16)
for i,tex in enumerate(parsed.textures):
 o=16+r+i*96+8;full=records[mat[o+8:o+24],0];tail=records[mat[o+40:o+56],0]
 assert tex.content_hash==mat[o+8:o+24] and full.endswith(tail) and len(tail)<=4096
 ar,ac=struct.unpack_from('<iI',mat,o+72);start=o+72+ar
 spans=[struct.unpack_from('<II',mat,start+j*8) for j in range(ac)]
 assert all(a+c==len(full) for a,c in spans)
 assert tex.mip_count==max(tex.width,tex.height).bit_length()

assert max(r[0][1] for r in source["records"])-min(r[0][1] for r in source["records"])<.29
print("PASS independent decoder: model positions, normals, UVs, LODs, 28cm height and four complete texture chains")
