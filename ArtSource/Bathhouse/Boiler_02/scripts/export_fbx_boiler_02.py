"""(Not run yet) Unreal용 FBX 익스포트 - Boiler_02.blend 를 연 상태에서 실행.
SM_Boiler_02 + UCX_SM_Boiler_02_00..03 을 한 FBX로 내보낸다."""
import bpy, os

ROOT = os.path.dirname(bpy.data.filepath)
OUT = os.path.join(ROOT, "SM_Boiler_02.fbx")
names = ["SM_Boiler_02"] + ["UCX_SM_Boiler_02_%02d" % i for i in range(4)]
vl = bpy.context.view_layer
for o in vl.objects:
    o.select_set(False)
for n in names:
    bpy.data.objects[n].select_set(True)
vl.objects.active = bpy.data.objects["SM_Boiler_02"]
bpy.ops.export_scene.fbx(
    filepath=OUT,
    use_selection=True,
    object_types={"MESH"},
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options="FBX_SCALE_NONE",
    axis_forward="-Z", axis_up="Y",
    use_mesh_modifiers=True,
    mesh_smooth_type="FACE",
    use_tspace=True,
    add_leaf_bones=False,
    bake_anim=False,
    path_mode="AUTO",
    embed_textures=False,
)
print("exported", OUT)
