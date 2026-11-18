import bpy,json,pathlib,sys
from mathutils import Vector
from bpy_extras.io_utils import axis_conversion
from mathutils import Matrix
import numpy as np
root=pathlib.Path(__file__).resolve().parents[2]
gold='--gold' in sys.argv
folder='lantern-gold3d' if gold else 'lantern3d'
out=root/'work'/folder;assets=root/'Lantern/assets'/folder;out.mkdir(parents=True,exist_ok=True)
if gold:
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(next((assets/'source').rglob('*.fbx'))))
else:bpy.ops.wm.open_mainfile(filepath=str(out/'source.blend'))
o=next(o for o in bpy.context.scene.objects if o.type=='MESH');bpy.context.view_layer.objects.active=o;o.select_set(True)
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
lo=Vector(tuple(min(v.co[i] for v in o.data.vertices) for i in range(3)));hi=Vector(tuple(max(v.co[i] for v in o.data.vertices) for i in range(3)))
scale=.28/(hi.z-lo.z);pivot=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,hi.z))
for v in o.data.vertices:v.co=(v.co-pivot)*scale
mod=o.modifiers.new('Belt mesh budget','DECIMATE');mod.ratio=4000/len(o.data.polygons);bpy.ops.object.modifier_apply(modifier=mod.name)
tri=o.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
for p in o.data.polygons:p.use_smooth=True
source=next((assets/'source').rglob('*.fbx')).parent
images={}
for name,suffix in [('albedo','texture.png'),('normal','texture_normal.png'),('metallic','texture_metallic.png'),('roughness','texture_roughness.png')]:
 im=bpy.data.images.load(str(next(source.glob('*'+suffix))));im.scale(1024,1024);images[name]=np.array(im.pixels[:],dtype=np.float32).reshape(-1,4)
 if name in ['albedo','normal']:im.filepath_raw=str(out/(name+'.png'));im.file_format='PNG';im.save()
rma=images['roughness'].copy();rma[:,0]=images['roughness'][:,0];rma[:,1]=images['metallic'][:,0];rma[:,2:]=1
for name,pixels in [('rma',rma),('emissive',np.zeros_like(rma))]:
 pixels[:,3]=1;im=bpy.data.images.new(name,1024,1024,alpha=True);im.pixels.foreach_set(pixels.ravel());im.filepath_raw=str(out/(name+'.png'));im.file_format='PNG';im.save()
to_game=(axis_conversion(from_forward='-Z',from_up='Y').to_4x4()@Matrix.Diagonal((-1.,1.,1.,1.))).inverted().to_3x3()
records=[];indices=[];lods=[]
for target in [4000,1200,400]:
 obj=o.copy();obj.data=o.data.copy();bpy.context.collection.objects.link(obj);bpy.context.view_layer.objects.active=obj
 if target<4000:
  dec=obj.modifiers.new('LOD','DECIMATE');dec.ratio=target/len(obj.data.polygons);bpy.ops.object.modifier_apply(modifier=dec.name)
 mesh=obj.data;mesh.calc_loop_triangles();mesh.calc_tangents();uv=mesh.uv_layers.active.data;start=len(indices)
 for tri in mesh.loop_triangles:
  for li in tri.loops:
   l=mesh.loops[li];v=to_game@mesh.vertices[l.vertex_index].co;n=(to_game@l.normal).normalized();t=(to_game@l.tangent).normalized()
   indices.append(len(records));records.append([list(v),list(n),list(t),-l.bitangent_sign,[uv[li].uv.x,1-uv[li].uv.y]])
 lods.append([start,len(indices)-start]);bpy.data.objects.remove(obj,do_unlink=True)
(assets/'mesh.json').write_text(json.dumps({'records':records,'indices':indices,'lods':lods},separators=(',',':')))
bpy.context.view_layer.objects.active=o
bpy.ops.wm.save_as_mainfile(filepath=str(assets/'lantern.blend'))
print('EXPORT',len(records),'vertices; height .28m; upper suspension pivot; LODs',lods)
