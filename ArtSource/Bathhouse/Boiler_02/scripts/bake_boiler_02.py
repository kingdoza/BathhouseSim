"""Boiler_02 bake pipeline: join parts -> UV0 atlas + UV1_Lightmap -> Cycles bakes -> export mesh.

Set STAGE before exec: "prep", "bake_base", "bake_rough", "bake_metal", "bake_normal", "bake_ao", "finalize".
Normal map is baked DirectX style (green = -Y) for Unreal.
"""
import bpy, bmesh, math, os
import numpy as np

STAGE = globals().get("STAGE", "prep")
RES = globals().get("RES", 2048)
ROOT = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_02"
TEX = os.path.join(ROOT, "textures")
os.makedirs(TEX, exist_ok=True)
sc = bpy.context.scene
vl = bpy.context.view_layer
BAKE_NAME = "SRC_Boiler02_BakeMesh"
EXPORT_NAME = "SM_Boiler_02"

def coll(name, parent=None):
    c = bpy.data.collections.get(name)
    if c is None:
        c = bpy.data.collections.new(name)
        (parent or sc.collection).children.link(c)
    return c

def purge(c):
    for o in list(c.objects):
        data = o.data
        bpy.data.objects.remove(o, do_unlink=True)
        if isinstance(data, bpy.types.Mesh) and data.users == 0:
            bpy.data.meshes.remove(data)

def select_only(ob):
    for o in vl.objects:
        o.select_set(False)
    ob.select_set(True)
    vl.objects.active = ob

def get_img(name, noncolor):
    img = bpy.data.images.get(name)
    if img is None or tuple(img.size) != (RES, RES):
        if img:
            bpy.data.images.remove(img)
        img = bpy.data.images.new(name, RES, RES, alpha=False, float_buffer=noncolor)
    img.colorspace_settings.name = "Non-Color" if noncolor else "sRGB"
    return img

# ------------------------------------------------------------------ prep
def prep():
    root = bpy.data.collections["Boiler02_Source"]
    parts = bpy.data.collections["Boiler02_Parts"]
    bc = coll("Boiler02_Bake", root)
    purge(bc)
    dups = []
    for o in parts.objects:
        d = o.copy(); d.data = o.data.copy(); bc.objects.link(d); dups.append(d)
    target = [d for d in dups if d.name.startswith("Body")][0]
    for o in vl.objects:
        o.select_set(False)
    for d in dups:
        d.select_set(True)
    vl.objects.active = target
    with bpy.context.temp_override(active_object=target, object=target,
                                   selected_objects=dups, selected_editable_objects=dups):
        bpy.ops.object.join()
    target.name = BAKE_NAME
    me = target.data
    me.name = BAKE_NAME
    # UV0 atlas
    select_only(target)
    me.uv_layers.active = me.uv_layers["UV0"]
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=0.003, area_weight=0.0,
                             correct_aspect=True, scale_to_bounds=False)
    bm = bmesh.from_edit_mesh(me)
    uv = bm.loops.layers.uv["UV0"]
    dial_idx = [i for i, m in enumerate(me.materials) if m and m.name == "SRC_Boiler02_Dial"]
    # give the gauge dial 3x texel density (readable numbers)
    dial_faces = [f for f in bm.faces if f.material_index in dial_idx]
    if dial_faces:
        from mathutils import Vector
        c = sum((l[uv].uv for f in dial_faces for l in f.loops), Vector((0, 0))) / sum(len(f.loops) for f in dial_faces)
        for f in dial_faces:
            for l in f.loops:
                l[uv].uv = c + (l[uv].uv - c) * 3.0
    bmesh.update_edit_mesh(me)
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, scale=True, margin_method="SCALED", margin=0.004, shape_method="CONCAVE")
    # UV1 lightmap (copy + repack with generous margin)
    bpy.ops.object.mode_set(mode="OBJECT")
    lm = me.uv_layers.get("UV1_Lightmap") or me.uv_layers.new(name="UV1_Lightmap", do_init=True)
    me.uv_layers.active = lm
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, scale=True, margin_method="SCALED", margin=0.02, shape_method="CONVEX")
    bpy.ops.object.mode_set(mode="OBJECT")
    # UV order: UV0, UV_Src (bake only), UV1_Lightmap
    me.uv_layers.active = me.uv_layers["UV0"]
    me.uv_layers["UV0"].active_render = True
    parts.hide_render = True
    vl.layer_collection.children["Boiler02_Source"].children["Boiler02_Parts"].hide_viewport = True
    return {"faces": len(me.polygons), "verts": len(me.vertices), "materials": [m.name for m in me.materials],
            "uv_layers": [u.name for u in me.uv_layers]}

# ------------------------------------------------------------------ bake
def _setup_cycles(samples):
    sc.render.engine = "CYCLES"
    sc.cycles.samples = samples
    sc.cycles.use_denoising = False
    try:
        sc.cycles.device = "GPU"
        prefs = bpy.context.preferences.addons["cycles"].preferences
        prefs.refresh_devices()
        if not any(d.use for d in prefs.devices if d.type != "CPU"):
            sc.cycles.device = "CPU"
    except Exception:
        sc.cycles.device = "CPU"
    b = sc.render.bake
    b.margin = 12
    b.margin_type = "EXTEND"
    b.use_clear = True
    b.target = "IMAGE_TEXTURES"
    b.use_selected_to_active = False

def _set_target(ob, img):
    for m in ob.data.materials:
        nt = m.node_tree
        n = nt.nodes.get("BAKE_TARGET")
        if n is None:
            n = nt.nodes.new("ShaderNodeTexImage"); n.name = "BAKE_TARGET"; n.location = (900, 400)
        n.image = img
        n.select = True
        nt.nodes.active = n

def _route(ob, socket_name):
    """socket_name None -> BSDF to output; else emission of that BSDF input."""
    for m in ob.data.materials:
        nt = m.node_tree
        bsdf = nt.nodes["BSDF"]
        out = [n for n in nt.nodes if n.type == "OUTPUT_MATERIAL"][0]
        em = nt.nodes.get("BAKE_EMIT")
        if em is None:
            em = nt.nodes.new("ShaderNodeEmission"); em.name = "BAKE_EMIT"; em.location = (600, 300)
        for l in list(em.inputs["Color"].links):
            nt.links.remove(l)
        if socket_name is None:
            nt.links.new(bsdf.outputs[0], out.inputs["Surface"])
            continue
        inp = bsdf.inputs[socket_name]
        if inp.links:
            nt.links.new(inp.links[0].from_socket, em.inputs["Color"])
        else:
            v = inp.default_value
            em.inputs["Color"].default_value = tuple(v) if hasattr(v, "__len__") else (v, v, v, 1.0)
        em.inputs["Strength"].default_value = 1.0
        nt.links.new(em.outputs[0], out.inputs["Surface"])

def bake(kind):
    ob = bpy.data.objects[BAKE_NAME]
    select_only(ob)
    spec = {
        "base": ("T_Boiler02_BaseColor", False, "EMIT", "Base Color", 4),
        "rough": ("_bake_rough", True, "EMIT", "Roughness", 4),
        "metal": ("_bake_metal", True, "EMIT", "Metallic", 4),
        "normal": ("T_Boiler02_Normal", True, "NORMAL", None, 4),
        "ao": ("_bake_ao", True, "AO", None, 64),
    }[kind]
    name, noncolor, btype, sock, samples = spec
    _setup_cycles(samples)
    img = get_img(name, noncolor)
    _set_target(ob, img)
    _route(ob, sock)
    if btype == "NORMAL":
        b = sc.render.bake
        b.normal_space = "TANGENT"
        b.normal_r, b.normal_g, b.normal_b = "POS_X", "NEG_Y", "POS_Z"
    if btype == "AO" and sc.world:
        sc.world.light_settings.distance = 0.15
    import time
    t = time.time()
    bpy.ops.object.bake(type=btype)
    _route(ob, None)
    return {"baked": name, "sec": round(time.time() - t, 1)}

# ------------------------------------------------------------------ finalize
def _px(img):
    a = np.empty(RES * RES * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    return a.reshape(RES, RES, 4)

def _save(img, path, noncolor):
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()

def finalize():
    base = bpy.data.images["T_Boiler02_BaseColor"]
    ao = _px(bpy.data.images["_bake_ao"])[..., 0]
    r = _px(bpy.data.images["_bake_rough"])[..., 0]
    m = _px(bpy.data.images["_bake_metal"])[..., 0]
    # mild cavity darkening into base colour (sRGB byte image -> values are display-referred)
    bp = _px(base)
    bp[..., :3] *= (0.72 + 0.28 * np.clip(ao, 0, 1))[..., None]
    base.pixels.foreach_set(bp.ravel())
    _save(base, os.path.join(TEX, "T_Boiler02_BaseColor.png"), False)
    orm = get_img("T_Boiler02_ORM", True)
    op = np.stack([np.clip(ao, 0, 1), np.clip(r, 0, 1), np.clip(m, 0, 1), np.ones_like(ao)], -1)
    orm.pixels.foreach_set(op.astype(np.float32).ravel())
    _save(orm, os.path.join(TEX, "T_Boiler02_ORM.png"), True)
    nimg = bpy.data.images["T_Boiler02_Normal"]
    _save(nimg, os.path.join(TEX, "T_Boiler02_Normal.png"), True)
    # reload saved PNGs as the final texture datablocks (8-bit on disk)
    loaded = {}
    for key, fn, cs in (("base", "T_Boiler02_BaseColor.png", "sRGB"), ("orm", "T_Boiler02_ORM.png", "Non-Color"),
                        ("nrm", "T_Boiler02_Normal.png", "Non-Color")):
        p = os.path.join(TEX, fn)
        old = bpy.data.images.get(fn)
        if old:
            bpy.data.images.remove(old)
        im = bpy.data.images.load(p)
        im.name = fn
        im.colorspace_settings.name = cs
        loaded[key] = im
    # final material
    mat = bpy.data.materials.get("M_Boiler02") or bpy.data.materials.new("M_Boiler02")
    mat.use_nodes = True
    nt = mat.node_tree; nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial"); out.location = (700, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled"); bsdf.location = (400, 0)
    nt.links.new(bsdf.outputs[0], out.inputs[0])
    uvn = nt.nodes.new("ShaderNodeUVMap"); uvn.uv_map = "UV0"; uvn.location = (-900, 0)
    tb = nt.nodes.new("ShaderNodeTexImage"); tb.image = loaded["base"]; tb.location = (-500, 300)
    to = nt.nodes.new("ShaderNodeTexImage"); to.image = loaded["orm"]; to.location = (-500, 0)
    tn = nt.nodes.new("ShaderNodeTexImage"); tn.image = loaded["nrm"]; tn.location = (-500, -300)
    for t in (tb, to, tn):
        nt.links.new(uvn.outputs[0], t.inputs[0])
    nt.links.new(tb.outputs[0], bsdf.inputs["Base Color"])
    sep = nt.nodes.new("ShaderNodeSeparateColor"); sep.location = (-200, 0)
    nt.links.new(to.outputs[0], sep.inputs[0])
    nt.links.new(sep.outputs[1], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs[2], bsdf.inputs["Metallic"])
    # DirectX -> OpenGL green flip for Blender display only
    sepn = nt.nodes.new("ShaderNodeSeparateColor"); sepn.location = (-250, -300)
    inv = nt.nodes.new("ShaderNodeMath"); inv.operation = "SUBTRACT"; inv.inputs[0].default_value = 1.0; inv.location = (-80, -380)
    comb = nt.nodes.new("ShaderNodeCombineColor"); comb.location = (80, -300)
    nm = nt.nodes.new("ShaderNodeNormalMap"); nm.uv_map = "UV0"; nm.location = (230, -300)
    nt.links.new(tn.outputs[0], sepn.inputs[0])
    nt.links.new(sepn.outputs[0], comb.inputs[0])
    nt.links.new(sepn.outputs[1], inv.inputs[1]); nt.links.new(inv.outputs[0], comb.inputs[1])
    nt.links.new(sepn.outputs[2], comb.inputs[2])
    nt.links.new(comb.outputs[0], nm.inputs["Color"])
    nt.links.new(nm.outputs[0], bsdf.inputs["Normal"])
    # export mesh
    src = bpy.data.objects[BAKE_NAME]
    ec = coll("Boiler02_Export")
    for o in list(ec.objects):
        if o.name.startswith(EXPORT_NAME) and not o.name.startswith("UCX_"):
            d = o.data; bpy.data.objects.remove(o, do_unlink=True)
            if d.users == 0: bpy.data.meshes.remove(d)
    ob = src.copy(); ob.data = src.data.copy(); ec.objects.link(ob)
    ob.name = EXPORT_NAME; ob.data.name = EXPORT_NAME
    me = ob.data
    me.materials.clear()
    me.materials.append(mat)
    for p in me.polygons:
        p.material_index = 0
    if "UV_Src" in me.uv_layers:
        me.uv_layers.remove(me.uv_layers["UV_Src"])
    me.uv_layers.active = me.uv_layers["UV0"]
    me.uv_layers["UV0"].active_render = True
    for n in ("_bake_ao", "_bake_rough", "_bake_metal"):
        im = bpy.data.images.get(n)
        if im: bpy.data.images.remove(im)
    for n in ("T_Boiler02_BaseColor", "T_Boiler02_ORM", "T_Boiler02_Normal"):
        im = bpy.data.images.get(n)
        if im: bpy.data.images.remove(im)
    return {"export": ob.name, "uv": [u.name for u in me.uv_layers], "tris": sum(len(p.vertices) - 2 for p in me.polygons)}

if STAGE == "prep":
    result = prep()
elif STAGE.startswith("bake_"):
    result = bake(STAGE[5:])
elif STAGE == "finalize":
    result = finalize()
