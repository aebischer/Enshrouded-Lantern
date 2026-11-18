import bpy,pathlib,re,math,sys
from mathutils import Vector,Quaternion,Matrix
root=pathlib.Path(__file__).resolve().parents[2]
folder='lantern-gold3d' if '--gold' in sys.argv else 'lantern3d'
bpy.ops.wm.open_mainfile(filepath=str(root/'Lantern/assets'/folder/'lantern.blend'))
s=bpy.context.scene;o=next(o for o in s.objects if o.type=='MESH')
text=(root/'Lantern/src/lantern_fit.inl').read_text()
def values(name):return [float(x.replace('f','')) for x in re.search(name+r'=\{([^}]+)',text)[1].split(',')]
x,y,z,w=values('rotation');q=Quaternion((w,x,y,z));anchor=Vector(values('strapAnchor'));belt=Vector(values('beltAnchor'));offset=belt-q@anchor
# Blender source -> game model -> pelvis slot -> upright body in Blender.
to_game=Matrix(((-1,0,0),(0,0,1),(0,-1,0)));to_body=Matrix(((0,-1,0),(0,0,1),(1,0,0)))
verts=[v.co.copy() for v in o.data.vertices]
s.render.engine='BLENDER_WORKBENCH';s.display.shading.light='STUDIO';s.display.shading.color_type='OBJECT';s.display.shading.show_shadows=True;s.display.shading.show_cavity=True;o.color=(.6,.35,.12,1)
s.render.resolution_x=700;s.render.resolution_y=700;s.render.resolution_percentage=100
# Geometric hip/belt reference, not a claim to reproduce the user's armour mesh.
bpy.ops.mesh.primitive_uv_sphere_add(segments=48,ring_count=24,location=(0,0,-.055));hip=bpy.context.object;hip.scale=(.21,.13,.21);hip.color=(.13,.16,.2,1)
bpy.ops.mesh.primitive_torus_add(major_radius=1,minor_radius=.035,location=(0,0,.025));b=bpy.context.object;b.scale=(.22,.14,.35);b.color=(.35,.23,.13,1)
bpy.ops.object.camera_add();cam=bpy.context.object;s.camera=cam;cam.data.type='ORTHO';cam.data.ortho_scale=.53
# +body Y is the character's front, confirmed in game on 0.1.8.
cam.location=(.9,1,.3);cam.rotation_euler=(Vector((.17,0,-.10))-cam.location).to_track_quat('-Z','Y').to_euler()
for name,rot,off in [('before',Quaternion((.5,.5,.5,-.5)),Vector((0,-.34,0))),('after',q,offset)]:
 for v,p in zip(o.data.vertices,verts):v.co=to_body@(rot@(to_game@p)+off)
 s.render.filepath=str(root/f'work/{folder}/fit-{name}.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'work'/folder/'fit-review.blend'))
print('STRAP CONTACT',list(to_body@(q@anchor+offset)),'OFFSET',list(offset))

