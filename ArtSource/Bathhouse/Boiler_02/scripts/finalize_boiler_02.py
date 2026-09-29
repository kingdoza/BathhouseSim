"""Boiler_02 export prep: UCX collision hulls, transform/pivot checks, validation dump, save .blend.
Does NOT export FBX (see export_fbx_boiler_02.py for the ready-made export call)."""
import bpy, bmesh, math, json, os
import numpy as np
from mathutils import Vector, Matrix

ROOT = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_02"
NAME = "SM_Boiler_02"
sc = bpy.context.scene
vl = bpy.context.view_layer
ec = bpy.data.collections["Boiler02_Export"]
ob = bpy.data.objects[NAME]

# ---------------------------------------------------------------- collision
for o in list(ec.objects):
    if o.name.startswith("UCX_"):
        d = o.data; bpy.data.objects.remove(o, do_unlink=True)
        if d and d.users == 0: bpy.data.meshes.remove(d)

def hull(name, pts):
    bm = bmesh.new()
    for p in pts:
        bm.verts.new(p)
    res = bmesh.ops.convex_hull(bm, input=bm.verts)
    bmesh.ops.delete(bm, geom=[g for g in res["geom_interior"] + res["geom_unused"] if isinstance(g, bmesh.types.BMVert)], context="VERTS")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); ec.objects.link(o)
    o.display_type = "WIRE"; o.hide_render = True
    return o

def box(c0, c1):
    return [Vector((x, y, z)) for x in (c0[0], c1[0]) for y in (c0[1], c1[1]) for z in (c0[2], c1[2])]

def chamfer_box(hx, hy, z0, z1, ch, cht):
    pts = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            for z in (z0, z1 - cht):
                pts += [Vector((sx * (hx - ch), sy * hy, z)), Vector((sx * hx, sy * (hy - ch), z))]
            pts += [Vector((sx * (hx - ch), sy * (hy - cht), z1)), Vector((sx * (hx - cht), sy * (hy - ch), z1))]
    return pts

def cyl(cx, cy, r, z0, z1, n=12):
    return [Vector((cx + r * math.cos(2 * math.pi * i / n), cy + r * math.sin(2 * math.pi * i / n), z))
            for i in range(n) for z in (z0, z1)]

hulls = [
    hull(f"UCX_{NAME}_00", chamfer_box(0.35, 0.2275, 0.0, 1.225, 0.045, 0.06)),       # shell + legs
    hull(f"UCX_{NAME}_01", cyl(0.0, 0.06, 0.107, 1.215, 1.368)),                     # chimney
    hull(f"UCX_{NAME}_02", box((0.345, -0.012, 0.635), (0.476, 0.072, 1.037))),       # valves
    hull(f"UCX_{NAME}_03", box((-0.226, -0.294, 0.172), (0.226, -0.2175, 1.106))),    # front hardware
]

# ---------------------------------------------------------------- hygiene
for o in [ob] + hulls:
    o.location = (0, 0, 0); o.rotation_euler = (0, 0, 0); o.scale = (1, 1, 1)
    o.parent = None
me = ob.data
bm = bmesh.new(); bm.from_mesh(me)
degenerate = [f for f in bm.faces if f.calc_area() < 1e-10]
if degenerate:
    bmesh.ops.delete(bm, geom=degenerate, context="FACES")
loose = [v for v in bm.verts if not v.link_faces]
if loose:
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
bm.to_mesh(me); bm.free()
me.validate(clean_customdata=False)
me.update()

# ---------------------------------------------------------------- scene settings
sc.unit_settings.system = "METRIC"
sc.unit_settings.scale_length = 1.0
sc.unit_settings.length_unit = "METERS"

# ---------------------------------------------------------------- report
co = np.empty(len(me.vertices) * 3, np.float32); me.vertices.foreach_get("co", co); co = co.reshape(-1, 3)
tris = sum(len(p.vertices) - 2 for p in me.polygons)
ngons = sum(1 for p in me.polygons if len(p.vertices) > 4)
def uv_arr(name):
    uv = np.empty(len(me.loops) * 2, np.float32); me.uv_layers[name].data.foreach_get("uv", uv)
    return uv.reshape(-1, 2)
tmp = os.path.join(ROOT, "_validation"); os.makedirs(tmp, exist_ok=True)
me.calc_loop_triangles()
lt = np.empty(len(me.loop_triangles) * 3, np.int32); me.loop_triangles.foreach_get("loops", lt)
for nm in ("UV0", "UV1_Lightmap"):
    np.save(os.path.join(tmp, nm + "_tris.npy"), uv_arr(nm)[lt].reshape(-1, 3, 2))
uv0, uv1 = uv_arr("UV0"), uv_arr("UV1_Lightmap")
report = {
    "object": ob.name, "mesh": me.name,
    "location": list(ob.location), "rotation": list(ob.rotation_euler), "scale": list(ob.scale),
    "bbox_min": [round(float(x), 4) for x in co.min(0)], "bbox_max": [round(float(x), 4) for x in co.max(0)],
    "dimensions_m": [round(float(x), 4) for x in (co.max(0) - co.min(0))],
    "vertices": len(me.vertices), "triangles": tris, "ngons": ngons,
    "degenerate_removed": len(degenerate), "loose_removed": len(loose),
    "materials": [m.name for m in me.materials],
    "uv_layers": [u.name for u in me.uv_layers],
    "uv0_range": [round(float(uv0.min()), 4), round(float(uv0.max()), 4)],
    "uv1_range": [round(float(uv1.min()), 4), round(float(uv1.max()), 4)],
    "sharp_edges": int(sum(1 for e in me.edges if e.use_edge_sharp)),
    "collision": [{"name": h.name, "verts": len(h.data.vertices), "faces": len(h.data.polygons)} for h in hulls],
    "unit_scale": sc.unit_settings.scale_length,
    "images": {i.name: {"path": i.filepath, "size": list(i.size), "colorspace": i.colorspace_settings.name}
               for i in bpy.data.images if i.name.startswith("T_Boiler02_")},
}
result = report
