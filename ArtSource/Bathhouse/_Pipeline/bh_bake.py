"""BathhouseSim shared bake / export pipeline (Blender 5.2, Cycles bake, Eevee preview).

Run through the MCP bridge with globals set before exec:
    ASSET = "Shower_01"            # folder + name stem
    ROOT  = r"...\\ArtSource\\Bathhouse\\Shower_01"
    STAGE = "prep" | "bake_base" | "bake_rough" | "bake_metal" | "bake_normal" | "bake_ao"
            | "finalize" | "export" | "preview" | "validate"
    RES   = 2048
    PIVOTS = {"Body": (0,0,0), "Door": (12.5,-37,42)}      # UE cm, per export group
    HULLS  = {"Body": [[(x,y,z), ...], ...], ...}           # UE cm point clouds per convex hull

Parts come from collection "<ASSET>_Parts" (custom prop bh_group). Output:
    textures/T_<ASSET>_BaseColor.png (sRGB), _ORM.png (R=AO,G=Rough,B=Metal), _Normal.png (DirectX)
    SM_<ASSET>.fbx (+ SM_<ASSET>_<Group>.fbx for moving parts), each with UCX_ hulls.
"""
import bpy, bmesh, math, os, json, time
import numpy as np
from mathutils import Vector, Matrix

ASSET = globals()["ASSET"]
ROOT = globals()["ROOT"]
STAGE = globals().get("STAGE", "prep")
RES = globals().get("RES", 2048)
PIVOTS = globals().get("PIVOTS", {"Body": (0, 0, 0)})
HULLS = globals().get("HULLS", {})
TEX = os.path.join(ROOT, "textures")
os.makedirs(TEX, exist_ok=True)
sc = bpy.context.scene
vl = bpy.context.view_layer
BAKE = "BAKE_" + ASSET
MAT_FINAL = "M_" + ASSET
SRC = ASSET + "_Source"
EXP = ASSET + "_Export"
PRV = ASSET + "_Preview"

def U(x=0.0, y=0.0, z=0.0):
    return Vector((x / 100.0, -y / 100.0, z / 100.0))

def coll(name, parent=None):
    c = bpy.data.collections.get(name)
    if c is None:
        c = bpy.data.collections.new(name)
        (parent or sc.collection).children.link(c)
    return c

def purge(c):
    for o in list(c.objects):
        d = o.data
        bpy.data.objects.remove(o, do_unlink=True)
        if isinstance(d, bpy.types.Mesh) and d.users == 0:
            bpy.data.meshes.remove(d)

def select_only(ob):
    for o in vl.objects:
        o.select_set(False)
    ob.select_set(True)
    vl.objects.active = ob

def find_lc(lc, name):
    if lc.name == name:
        return lc
    for ch in lc.children:
        r = find_lc(ch, name)
        if r:
            return r
    return None

def group_names():
    parts = bpy.data.collections[ASSET + "_Parts"].objects
    names = []
    for o in parts:
        g = o.get("bh_group", "Body")
        if g not in names:
            names.append(g)
    names.sort(key=lambda g: (g != "Body", g))
    return names

def export_name(g):
    return "SM_" + ASSET if g == "Body" else "SM_%s_%s" % (ASSET, g)

# ------------------------------------------------------------------ prep
def prep():
    parts = bpy.data.collections[ASSET + "_Parts"]
    bc = coll(ASSET + "_Bake", coll(SRC))
    purge(bc)
    groups = group_names()
    dg = bpy.context.evaluated_depsgraph_get()
    dups = []
    for o in parts.objects:
        me = bpy.data.meshes.new_from_object(o.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
        me.transform(o.matrix_world)
        for p in me.polygons:
            p.use_smooth = True
        me.set_sharp_from_angle(angle=math.radians(o.get("bh_sharp", 40)))
        a = me.attributes.new("bh_grp", "INT", "FACE")
        gi = groups.index(o.get("bh_group", "Body"))
        a.data.foreach_set("value", [gi] * len(me.polygons))
        d = bpy.data.objects.new(o.name + "_b", me)
        bc.objects.link(d)
        dups.append(d)
    target = dups[0]
    for o in vl.objects:
        o.select_set(False)
    for d in dups:
        d.select_set(True)
    vl.objects.active = target
    with bpy.context.temp_override(active_object=target, object=target,
                                   selected_objects=dups, selected_editable_objects=dups):
        bpy.ops.object.join()
    target.name = BAKE
    me = target.data
    me.name = BAKE
    select_only(target)
    me.uv_layers.active = me.uv_layers["UV0"]
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(55), island_margin=0.003, area_weight=0.0,
                             correct_aspect=True, scale_to_bounds=False)
    bm = bmesh.from_edit_mesh(me)
    uv = bm.loops.layers.uv["UV0"]
    # boost texel density on image-textured faces (dials, labels)
    img_idx = set()
    for i, m in enumerate(me.materials):
        if m and m.node_tree and any(n.type == "UVMAP" and n.uv_map == "UV_Src" for n in m.node_tree.nodes):
            img_idx.add(i)
    boost = [f for f in bm.faces if f.material_index in img_idx]
    if boost:
        # per connected island scale x3
        seen = set()
        for f0 in boost:
            if f0.index in seen:
                continue
            isl, stack = [], [f0]
            while stack:
                f = stack.pop()
                if f.index in seen or f.material_index not in img_idx:
                    continue
                seen.add(f.index); isl.append(f)
                for e in f.edges:
                    for lf in e.link_faces:
                        if lf.index not in seen:
                            stack.append(lf)
            loops = [l for f in isl for l in f.loops]
            c = sum((l[uv].uv for l in loops), Vector((0, 0))) / len(loops)
            for l in loops:
                l[uv].uv = c + (l[uv].uv - c) * 3.0
    bmesh.update_edit_mesh(me)
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, scale=True, margin_method="SCALED", margin=0.004, shape_method="CONCAVE")
    bpy.ops.object.mode_set(mode="OBJECT")
    me.uv_layers.active = me.uv_layers["UV0"]
    me.uv_layers["UV0"].active_render = True
    parts.hide_render = True
    lc = find_lc(vl.layer_collection, parts.name)
    if lc:
        lc.hide_viewport = True
    return {"faces": len(me.polygons), "verts": len(me.vertices), "groups": groups,
            "materials": [m.name for m in me.materials]}

# ------------------------------------------------------------------ bake
def _setup_cycles(samples):
    sc.render.engine = "CYCLES"
    sc.cycles.samples = samples
    sc.cycles.use_denoising = False
    try:
        prefs = bpy.context.preferences.addons["cycles"].preferences
        prefs.refresh_devices()
        gpus = [d for d in prefs.devices if d.type != "CPU"]
        if gpus:
            for t in ("OPTIX", "CUDA", "HIP", "ONEAPI"):
                if any(d.type == t for d in gpus):
                    prefs.compute_device_type = t
                    break
            for d in prefs.devices:
                d.use = True
            sc.cycles.device = "GPU"
        else:
            sc.cycles.device = "CPU"
    except Exception:
        sc.cycles.device = "CPU"
    b = sc.render.bake
    b.margin = 12
    b.margin_type = "EXTEND"
    b.use_clear = True
    b.target = "IMAGE_TEXTURES"
    b.use_selected_to_active = False

def _img(name, noncolor):
    img = bpy.data.images.get(name)
    if img is None or tuple(img.size) != (RES, RES):
        if img:
            bpy.data.images.remove(img)
        img = bpy.data.images.new(name, RES, RES, alpha=False, float_buffer=noncolor)
    img.colorspace_settings.name = "Non-Color" if noncolor else "sRGB"
    return img

def _set_target(ob, img):
    for m in ob.data.materials:
        nt = m.node_tree
        n = nt.nodes.get("BAKE_TARGET")
        if n is None:
            n = nt.nodes.new("ShaderNodeTexImage"); n.name = "BAKE_TARGET"; n.location = (1200, 400)
        n.image = img
        n.select = True
        nt.nodes.active = n

def _route(ob, socket_name):
    for m in ob.data.materials:
        nt = m.node_tree
        bsdf = nt.nodes["BSDF"]
        out = [n for n in nt.nodes if n.type == "OUTPUT_MATERIAL"][0]
        em = nt.nodes.get("BAKE_EMIT")
        if em is None:
            em = nt.nodes.new("ShaderNodeEmission"); em.name = "BAKE_EMIT"; em.location = (900, 300)
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
    ob = bpy.data.objects[BAKE]
    select_only(ob)
    spec = {
        "base": ("T_%s_BaseColor" % ASSET, False, "EMIT", "Base Color", 4),
        "rough": ("_bake_rough", True, "EMIT", "Roughness", 4),
        "metal": ("_bake_metal", True, "EMIT", "Metallic", 4),
        "normal": ("T_%s_Normal" % ASSET, True, "NORMAL", None, 4),
        "ao": ("_bake_ao", True, "AO", None, 64),
    }[kind]
    name, noncolor, btype, sock, samples = spec
    _setup_cycles(samples)
    img = _img(name, noncolor)
    _set_target(ob, img)
    _route(ob, sock)
    if btype == "NORMAL":
        b = sc.render.bake
        b.normal_space = "TANGENT"
        b.normal_r, b.normal_g, b.normal_b = "POS_X", "NEG_Y", "POS_Z"
    if btype == "AO":
        w = sc.world or bpy.data.worlds.new("World")
        sc.world = w
        w.light_settings.distance = 0.12
    t = time.time()
    bpy.ops.object.bake(type=btype)
    _route(ob, None)
    return {"baked": name, "sec": round(time.time() - t, 1), "device": sc.cycles.device}

# ------------------------------------------------------------------ finalize
def _px(img):
    a = np.empty(RES * RES * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    return a.reshape(RES, RES, 4)

def _save(img, path):
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()

def _final_material():
    base = bpy.data.images["T_%s_BaseColor" % ASSET]
    ao = _px(bpy.data.images["_bake_ao"])[..., 0]
    r = _px(bpy.data.images["_bake_rough"])[..., 0]
    m = _px(bpy.data.images["_bake_metal"])[..., 0]
    bp = _px(base)
    bp[..., :3] *= (0.70 + 0.30 * np.clip(ao, 0, 1))[..., None]
    base.pixels.foreach_set(bp.ravel())
    _save(base, os.path.join(TEX, "T_%s_BaseColor.png" % ASSET))
    orm = _img("T_%s_ORM" % ASSET, True)
    op = np.stack([np.clip(ao, 0, 1), np.clip(r, 0, 1), np.clip(m, 0, 1), np.ones_like(ao)], -1)
    orm.pixels.foreach_set(op.astype(np.float32).ravel())
    _save(orm, os.path.join(TEX, "T_%s_ORM.png" % ASSET))
    _save(bpy.data.images["T_%s_Normal" % ASSET], os.path.join(TEX, "T_%s_Normal.png" % ASSET))
    loaded = {}
    for key, fn, cs in (("base", "T_%s_BaseColor.png" % ASSET, "sRGB"), ("orm", "T_%s_ORM.png" % ASSET, "Non-Color"),
                        ("nrm", "T_%s_Normal.png" % ASSET, "Non-Color")):
        old = bpy.data.images.get(fn)
        if old:
            bpy.data.images.remove(old)
        im = bpy.data.images.load(os.path.join(TEX, fn))
        im.name = fn
        im.colorspace_settings.name = cs
        loaded[key] = im
    mat = bpy.data.materials.get(MAT_FINAL) or bpy.data.materials.new(MAT_FINAL)
    mat.use_nodes = True
    nt = mat.node_tree; nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial"); out.location = (700, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled"); bsdf.location = (400, 0)
    nt.links.new(bsdf.outputs[0], out.inputs[0])
    uvn = nt.nodes.new("ShaderNodeUVMap"); uvn.uv_map = "UV0"; uvn.location = (-900, 0)
    tb = nt.nodes.new("ShaderNodeTexImage"); tb.image = loaded["base"]; tb.location = (-500, 300)
    to = nt.nodes.new("ShaderNodeTexImage"); to.image = loaded["orm"]; to.location = (-500, 0)
    tn = nt.nodes.new("ShaderNodeTexImage"); tn.image = loaded["nrm"]; tn.location = (-500, -300)
    for t_ in (tb, to, tn):
        nt.links.new(uvn.outputs[0], t_.inputs[0])
    nt.links.new(tb.outputs[0], bsdf.inputs["Base Color"])
    sep = nt.nodes.new("ShaderNodeSeparateColor"); sep.location = (-200, 0)
    nt.links.new(to.outputs[0], sep.inputs[0])
    nt.links.new(sep.outputs[1], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs[2], bsdf.inputs["Metallic"])
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
    for n in ("_bake_ao", "_bake_rough", "_bake_metal", "T_%s_BaseColor" % ASSET, "T_%s_ORM" % ASSET,
              "T_%s_Normal" % ASSET):
        im = bpy.data.images.get(n)
        if im:
            bpy.data.images.remove(im)
    return mat

def _hull(name, pts, c):
    bm = bmesh.new()
    for p in pts:
        bm.verts.new(p)
    res = bmesh.ops.convex_hull(bm, input=bm.verts)
    bmesh.ops.delete(bm, geom=[g for g in res["geom_interior"] + res["geom_unused"]
                               if isinstance(g, bmesh.types.BMVert)], context="VERTS")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); c.objects.link(o)
    o.display_type = "WIRE"; o.hide_render = True
    return o

def box_pts(c0, c1):
    """UE-cm corner box -> point list (UE cm)."""
    return [(x, y, z) for x in (c0[0], c1[0]) for y in (c0[1], c1[1]) for z in (c0[2], c1[2])]

def finalize():
    mat = _final_material()
    src = bpy.data.objects[BAKE]
    ec = coll(EXP)
    purge(ec)
    groups = group_names()
    out = {}
    for gi, g in enumerate(groups):
        me = src.data.copy()
        bm = bmesh.new(); bm.from_mesh(me)
        lay = bm.faces.layers.int.get("bh_grp")
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if f[lay] != gi], context="FACES")
        ng = [f for f in bm.faces if len(f.verts) > 4]
        if ng:
            bmesh.ops.triangulate(bm, faces=ng, quad_method="BEAUTY", ngon_method="BEAUTY")
        bm.to_mesh(me); bm.free()
        piv = U(*PIVOTS.get(g, (0, 0, 0)))
        me.transform(Matrix.Translation(-piv))
        name = export_name(g)
        me.name = name
        ob = bpy.data.objects.new(name, me)
        ec.objects.link(ob)
        me.materials.clear(); me.materials.append(mat)
        for p in me.polygons:
            p.material_index = 0
        if "UV_Src" in me.uv_layers:
            me.uv_layers.remove(me.uv_layers["UV_Src"])
        if "bh_grp" in me.attributes:
            me.attributes.remove(me.attributes["bh_grp"])
        # UV1 lightmap per export mesh
        lm = me.uv_layers.new(name="UV1_Lightmap", do_init=True)
        me.uv_layers.active = lm
        select_only(ob)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.select_all(action="SELECT")
        bpy.ops.uv.pack_islands(rotate=True, scale=True, margin_method="SCALED", margin=0.025, shape_method="CONVEX")
        bpy.ops.object.mode_set(mode="OBJECT")
        me.uv_layers.active = me.uv_layers["UV0"]
        me.uv_layers["UV0"].active_render = True
        hulls = []
        for i, pts in enumerate(HULLS.get(g, [])):
            hp = [U(*p) - piv for p in pts]
            hulls.append(_hull("UCX_%s_%02d" % (name, i), hp, ec).name)
        out[name] = {"pivot_ue_cm": PIVOTS.get(g, (0, 0, 0)), "tris": sum(len(p.vertices) - 2 for p in me.polygons),
                     "hulls": hulls}
    # assembled preview (instances placed at their pivots)
    pc = coll(PRV)
    purge(pc)
    for g in groups:
        e = bpy.data.objects[export_name(g)]
        inst = bpy.data.objects.new("PRV_" + e.name, e.data)
        inst.location = U(*PIVOTS.get(g, (0, 0, 0)))
        pc.objects.link(inst)
    ec.hide_render = True
    lc = find_lc(vl.layer_collection, EXP)
    if lc:
        lc.hide_viewport = True
    slc = find_lc(vl.layer_collection, SRC)
    if slc:
        slc.exclude = True
    return out

# ------------------------------------------------------------------ export / validate
def _ucx(ec, name):
    import re
    pat = re.compile(r"^UCX_%s_\d\d$" % re.escape(name))
    return [o for o in ec.objects if pat.match(o.name)]

def export():
    ec = bpy.data.collections[EXP]
    lc = find_lc(vl.layer_collection, EXP)
    if lc:
        lc.hide_viewport = False; lc.exclude = False
    files = {}
    for g in group_names():
        name = export_name(g)
        objs = [bpy.data.objects[name]] + _ucx(ec, name)
        for o in vl.objects:
            o.select_set(False)
        for o in objs:
            o.hide_set(False); o.select_set(True)
        vl.objects.active = objs[0]
        path = os.path.join(ROOT, name + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={"MESH"}, global_scale=1.0,
                                 apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
                                 axis_forward="-Z", axis_up="Y", use_mesh_modifiers=True,
                                 mesh_smooth_type="FACE", use_tspace=True, add_leaf_bones=False,
                                 bake_anim=False, path_mode="AUTO", embed_textures=False)
        files[name] = [o.name for o in objs]
    if lc:
        lc.hide_viewport = True
    return files

def validate():
    rep = {"asset": ASSET, "checks": [], "meshes": {}}
    def chk(name, ok, detail=""):
        rep["checks"].append({"check": name, "ok": bool(ok), "detail": detail})
    ec = bpy.data.collections[EXP]
    tmp = coll("_VALIDATE_TMP")
    for g in group_names():
        name = export_name(g)
        src = bpy.data.objects[name]
        me = src.data
        co = np.empty(len(me.vertices) * 3, np.float32); me.vertices.foreach_get("co", co); co = co.reshape(-1, 3)
        tris = sum(len(p.vertices) - 2 for p in me.polygons)
        ngons = sum(1 for p in me.polygons if len(p.vertices) > 4)
        info = {"bbox_min_m": [round(float(x), 4) for x in co.min(0)], "bbox_max_m": [round(float(x), 4) for x in co.max(0)],
                "size_ue_cm": [round(float(x) * 100, 1) for x in (co.max(0) - co.min(0))],
                "vertices": len(me.vertices), "triangles": tris, "ngons": ngons,
                "uv_layers": [u.name for u in me.uv_layers], "materials": [m.name for m in me.materials],
                "hulls": [o.name for o in _ucx(ec, name)]}
        chk(name + " transform identity", tuple(src.location) == (0, 0, 0) and tuple(src.scale) == (1, 1, 1))
        chk(name + " uv layers", info["uv_layers"] == ["UV0", "UV1_Lightmap"], str(info["uv_layers"]))
        chk(name + " single material", len(info["materials"]) == 1)
        path = os.path.join(ROOT, name + ".fbx")
        chk(name + " fbx exists", os.path.exists(path))
        if os.path.exists(path):
            before = set(bpy.data.objects)
            bpy.ops.import_scene.fbx(filepath=path)
            new = [o for o in bpy.data.objects if o not in before]
            for o in new:
                for c in o.users_collection:
                    c.objects.unlink(o)
                tmp.objects.link(o)
            imp = [o for o in new if o.name.startswith(name) and not o.name.startswith("UCX_")]
            if imp:
                o = imp[0]
                dg = bpy.context.evaluated_depsgraph_get()
                pts = np.array([o.matrix_world @ v.co for v in o.data.vertices])
                err = float(np.abs(pts.min(0) - co.min(0)).max() + np.abs(pts.max(0) - co.max(0)).max())
                chk(name + " reimport bounds", err < 0.001, "err_m=%.6f" % err)
            chk(name + " reimport hulls", len([o for o in new if o.name.startswith("UCX_")]) == len(info["hulls"]))
            for o in new:
                d = o.data
                bpy.data.objects.remove(o, do_unlink=True)
                if isinstance(d, bpy.types.Mesh) and d.users == 0:
                    bpy.data.meshes.remove(d)
        rep["meshes"][name] = info
    bpy.data.collections.remove(tmp)
    rep["passed"] = sum(c["ok"] for c in rep["checks"])
    rep["total"] = len(rep["checks"])
    with open(os.path.join(ROOT, "validation_report.json"), "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=2)
    return {"passed": rep["passed"], "total": rep["total"],
            "failed": [c for c in rep["checks"] if not c["ok"]], "meshes": rep["meshes"]}

# ------------------------------------------------------------------ preview
def preview():
    views = globals().get("VIEWS", ["front", "front34", "right", "back34", "left", "back", "top", "detail"])
    size = globals().get("VIEW_RES", (800, 1000))
    tgt = Vector(globals().get("TARGET", (0, 0, 0.6)))
    ortho = globals().get("ORTHO", 1.6)
    detail_tgt = Vector(globals().get("DETAIL_TARGET", (0, 0, 0.6)))
    detail_dist = globals().get("DETAIL_DIST", 1.2)
    front = globals().get("FRONT", "-Y")   # Blender axis the facility front faces
    out_dir = os.path.join(ROOT, "renders")
    os.makedirs(out_dir, exist_ok=True)
    pc = coll(ASSET + "_Rig")
    def light(name, loc, energy, sz, color=(1, 1, 1)):
        o = bpy.data.objects.get(name)
        if o is None:
            o = bpy.data.objects.new(name, bpy.data.lights.new(name, "AREA")); pc.objects.link(o)
        o.data.energy = energy; o.data.size = sz; o.data.color = color
        o.location = loc
        o.rotation_euler = (tgt - Vector(loc)).to_track_quat("-Z", "Y").to_euler()
    # rotate rig so "front" is toward the camera's front view
    yaw = {"-Y": 0.0, "+X": math.pi / 2, "+Y": math.pi, "-X": -math.pi / 2}[front]
    Rz = Matrix.Rotation(yaw, 3, "Z")
    light("RIG_Key", Rz @ Vector((-1.8, -2.4, 2.6)), 300, 1.8, (1.0, 0.93, 0.85))
    light("RIG_Fill", Rz @ Vector((2.6, -1.6, 1.3)), 90, 2.4, (0.8, 0.92, 1.0))
    light("RIG_Rim", Rz @ Vector((1.4, 2.6, 2.2)), 220, 1.6)
    light("RIG_Back", Rz @ Vector((-2.2, 2.0, 1.0)), 90, 1.6)
    w = sc.world or bpy.data.worlds.new("World")
    sc.world = w; w.use_nodes = True
    bg = w.node_tree.nodes.get("Background")
    bg.inputs[0].default_value = (0.20, 0.24, 0.25, 1)
    bg.inputs[1].default_value = 0.6
    cam = bpy.data.objects.get("RIG_Camera")
    if cam is None:
        cam = bpy.data.objects.new("RIG_Camera", bpy.data.cameras.new("RIG_Camera")); pc.objects.link(cam)
    sc.camera = cam
    eng = globals().get("ENGINE", "BLENDER_EEVEE")
    sc.render.engine = eng
    if eng == "CYCLES":
        _setup_cycles(globals().get("SAMPLES", 48))
        sc.cycles.use_denoising = True
    show = globals().get("SHOW", "final")
    for nm, vis in ((SRC, show == "source"), (PRV, show == "final")):
        lc_ = find_lc(vl.layer_collection, nm)
        if lc_:
            lc_.exclude = not vis
    if show == "source":
        lp = find_lc(vl.layer_collection, ASSET + "_Parts")
        if lp:
            lp.hide_viewport = False
            bpy.data.collections[ASSET + "_Parts"].hide_render = False
        bc_ = bpy.data.collections.get(ASSET + "_Bake")
        if bc_:
            bc_.hide_render = True
    sc.render.resolution_x, sc.render.resolution_y = size
    sc.render.resolution_percentage = 100
    sc.view_settings.view_transform = globals().get("VIEW_TRANSFORM", "Standard")
    sc.view_settings.look = "None"
    sc.view_settings.exposure = globals().get("EXPOSURE", -0.45)
    sc.render.image_settings.file_format = "PNG"
    try:
        sc.eevee.use_raytracing = True
    except Exception:
        pass
    VD = {
        "front": (Vector((0, -1, 0)), True), "back": (Vector((0, 1, 0)), True),
        "right": (Vector((1, 0, 0)), True), "left": (Vector((-1, 0, 0)), True),
        "top": (Vector((0, -0.45, 1)).normalized(), False),
        "front34": (Vector((0.8, -1, 0.42)).normalized(), False),
        "front34l": (Vector((-0.8, -1, 0.42)).normalized(), False),
        "back34": (Vector((-0.8, 1, 0.42)).normalized(), False),
        "detail": (Vector((0.35, -1, 0.15)).normalized(), False),
    }
    done = []
    for v in views:
        d, is_ortho = VD[v]
        d = Rz @ d
        cam.data.type = "ORTHO" if is_ortho else "PERSP"
        cam.data.ortho_scale = ortho
        cam.data.lens = 55
        t_ = detail_tgt if v == "detail" else tgt
        dist = detail_dist if v == "detail" else ortho * 2.35
        cam.location = t_ + d * dist
        cam.rotation_euler = (t_ - cam.location).to_track_quat("-Z", "Y").to_euler()
        fp = os.path.join(out_dir, "%s_%s.png" % (ASSET, v))
        sc.render.filepath = fp
        bpy.ops.render.render(write_still=True)
        done.append(fp)
    # collage (4 columns)
    cols = 4
    rows = (len(done) + cols - 1) // cols
    W, H = size
    canvas = np.zeros((rows * H, cols * W, 4), np.float32)
    canvas[..., 3] = 1
    for i, fp in enumerate(done):
        im = bpy.data.images.load(fp)
        px = np.empty(W * H * 4, np.float32); im.pixels.foreach_get(px)
        px = px.reshape(H, W, 4)
        r, c = divmod(i, cols)
        canvas[(rows - 1 - r) * H:(rows - r) * H, c * W:(c + 1) * W] = px
        bpy.data.images.remove(im)
    sheet = bpy.data.images.new("_sheet", cols * W, rows * H, alpha=False)
    sheet.pixels.foreach_set(canvas.ravel())
    sp = os.path.join(ROOT, "%s_Turnaround.png" % ASSET)
    sheet.filepath_raw = sp; sheet.file_format = "PNG"; sheet.save()
    bpy.data.images.remove(sheet)
    return {"renders": done, "sheet": sp}

def save():
    for o in list(bpy.data.objects):
        if o.name.startswith("BAKE_") or o.name.endswith("_b"):
            pass
    path = os.path.join(ROOT, ASSET + ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=path, compress=True)
    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_mainfile(compress=True)
    # manifest
    groups = group_names()
    man = {"asset": ASSET, "source_blend": ASSET + ".blend", "units": "metres in Blender, UE cm in configs",
           "axis": "UE = (Bx, -By, Bz) * 100 (verified on Bath_01 glb import, REPORT_UNREAL_DISCOVERY MODEL-M1)",
           "meshes": {}, "textures": {"T_%s_BaseColor.png" % ASSET: "%d, sRGB" % RES,
                                      "T_%s_ORM.png" % ASSET: "%d, linear (R=AO, G=Roughness, B=Metallic)" % RES,
                                      "T_%s_Normal.png" % ASSET: "%d, linear, DirectX (green -Y)" % RES},
           "material": MAT_FINAL}
    ec = bpy.data.collections[EXP]
    for g in groups:
        n = export_name(g)
        me = bpy.data.objects[n].data
        co = np.empty(len(me.vertices) * 3, np.float32); me.vertices.foreach_get("co", co); co = co.reshape(-1, 3) * 100
        co[:, 1] *= -1
        man["meshes"][n] = {"fbx": n + ".fbx", "group": g, "pivot_ue_cm": list(PIVOTS.get(g, (0, 0, 0))),
                            "bounds_ue_cm_rel_pivot": [[round(float(v), 1) for v in co.min(0)],
                                                       [round(float(v), 1) for v in co.max(0)]],
                            "vertices": len(me.vertices), "triangles": sum(len(p.vertices) - 2 for p in me.polygons),
                            "collision": [o.name for o in _ucx(ec, n)]}
    with open(os.path.join(ROOT, "asset_manifest.json"), "w", encoding="utf-8") as f:
        json.dump(man, f, ensure_ascii=False, indent=2)
    return {"saved": path, "manifest": man["meshes"]}

if STAGE == "save":
    result = save()
elif STAGE == "prep":
    result = prep()
elif STAGE.startswith("bake_"):
    result = bake(STAGE[5:])
elif STAGE == "finalize":
    result = finalize()
elif STAGE == "export":
    result = export()
elif STAGE == "validate":
    result = validate()
elif STAGE == "preview":
    result = preview()
