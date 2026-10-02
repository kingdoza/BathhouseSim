"""BathhouseSim shared modeling library (Blender 5.2).

Style target: Fab "Stylized House Interior" (StylArts) = project Content/StylizedKitchen pack -
form language and material treatment ONLY (chunky rounded bevels, pale painted edge highlights,
edge chips, gentle top-light gradient). Colours must NOT be taken from the reference
(.md/MODELING_STYLE_GUIDE.md section 1); PAL below is a provisional palette pending replacement.

Usage inside a build script (executed through the Blender MCP bridge):
    exec(open(r"...\\_Pipeline\\bh_lib.py", encoding="utf-8").read())

Coordinates: helpers take **Unreal actor-local centimetres** (X fwd, Y right, Z up) through U(),
and convert to Blender metres with Y negated (the UE FBX importer mirrors Y), so a part placed at
UE (0,-33,42) lands where the Blueprint component expects it after import.
"""
import bpy, bmesh, math, os, json, time
from mathutils import Vector, Matrix

PIPE = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\_Pipeline"

# ------------------------------------------------------------------ coordinates
def U(x=0.0, y=0.0, z=0.0):
    """Unreal cm -> Blender m (Y mirrored)."""
    return Vector((x / 100.0, -y / 100.0, z / 100.0))

def US(x, y, z):
    """Unreal size cm -> Blender size m (no sign)."""
    return Vector((abs(x) / 100.0, abs(y) / 100.0, abs(z) / 100.0))

def T(v):
    return Matrix.Translation(v)

def R(axis, deg):
    return Matrix.Rotation(math.radians(deg), 4, axis)

# axis frames for lathe/cyl: local +Z mapped to the given Blender axis
AX = {
    "+Z": Matrix.Identity(4),
    "-Z": R("X", 180),
    "-Y": R("X", 90),
    "+Y": R("X", -90),
    "+X": R("Y", 90),
    "-X": R("Y", -90),
}
def UAX(axis):
    """Unreal axis name -> Blender axis frame (Y mirrored)."""
    return AX[{"+Y": "-Y", "-Y": "+Y"}.get(axis, axis)]

# ------------------------------------------------------------------ scene
def reset_scene():
    """Delete all scene data in the open session (keeps add-ons / bridge alive)."""
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for c in list(bpy.data.collections):
        bpy.data.collections.remove(c)
    for lib in list(bpy.data.libraries):
        bpy.data.libraries.remove(lib)
    for coll_ in (bpy.data.meshes, bpy.data.materials, bpy.data.images, bpy.data.cameras,
                  bpy.data.lights, bpy.data.node_groups, bpy.data.curves):
        for d in list(coll_):
            try:
                coll_.remove(d)
            except Exception:
                pass
    sc = bpy.context.scene
    sc.unit_settings.system = "METRIC"
    sc.unit_settings.scale_length = 1.0
    sc.unit_settings.length_unit = "CENTIMETERS"

def coll(name, parent=None):
    c = bpy.data.collections.get(name)
    if c is None:
        c = bpy.data.collections.new(name)
        (parent or bpy.context.scene.collection).children.link(c)
    return c

# ------------------------------------------------------------------ part registry
# Every modelled piece is an object in collection "<ASSET>_Parts" with custom props:
#   bh_group : export mesh it belongs to ("Body", "Door", "Needle", ...)
#   bh_bevel : bevel width (m), applied at bake time
PARTS = {"coll": None}

def begin_parts(asset):
    PARTS["coll"] = coll(asset + "_Parts", coll(asset + "_Source"))

def _finish(name, bm, mat, group, bevel, segs, sharp, src_uv=None):
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    PARTS["coll"].objects.link(ob)
    me.materials.append(mat)
    ob["bh_group"] = group
    me.uv_layers.new(name="UV0")
    src = me.uv_layers.new(name="UV_Src")
    if src_uv:
        src_uv(me)
    if bevel > 0:
        b = ob.modifiers.new("Bevel", "BEVEL")
        b.width = bevel
        b.segments = segs
        b.limit_method = "ANGLE"
        b.angle_limit = math.radians(40)
        b.profile = 0.6
        b.harden_normals = False
        b.miter_outer = "MITER_ARC"
    ob["bh_sharp"] = sharp
    return ob

def box(name, mat, center, size, group="Body", bevel=0.01, segs=3, sharp=40, rot=None, taper=None):
    """Axis-aligned box. center/size in UE cm. rot: list of (axis,deg) in UE-local terms applied
    about the box centre. taper: (sx,sy) scale of the top face (UE axes)."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    s = US(*size)
    for v in bm.verts:
        v.co = Vector((v.co.x * s.x, v.co.y * s.y, v.co.z * s.z))
        if taper and v.co.z > 0:
            v.co.x *= taper[0]; v.co.y *= taper[1]
    m = Matrix.Identity(4)
    for ax, deg in (rot or []):
        # UE rotation about Y is mirrored in Blender: negate angles about X and Z
        m = m @ R(ax, -deg if ax in ("X", "Z") else deg)
    bmesh.ops.transform(bm, matrix=T(U(*center)) @ m, verts=bm.verts)
    return _finish(name, bm, mat, group, bevel, segs, sharp)

def cyl(name, mat, center, radius, depth, axis="+Z", group="Body", verts=32, bevel=0.004, segs=2,
        sharp=40, r2=None, spin=0.0):
    """Cylinder/cone. center & sizes in UE cm, axis in UE terms."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=verts,
                          radius1=radius / 100.0, radius2=(r2 if r2 is not None else radius) / 100.0,
                          depth=depth / 100.0)
    bmesh.ops.transform(bm, matrix=T(U(*center)) @ UAX(axis) @ R("Z", spin), verts=bm.verts)
    return _finish(name, bm, mat, group, bevel, segs, sharp)

def lathe(name, mat, center, prof, axis="+Z", group="Body", verts=32, bevel=0.0, segs=2, sharp=40,
          closed=False, spin=0.0):
    """Revolve a (r,z) profile in UE cm around local axis. Profile runs bottom-out-up-in."""
    bm = bmesh.new()
    mat4 = T(U(*center)) @ UAX(axis)
    rings = []
    for r, z in prof:
        r, z = r / 100.0, z / 100.0
        if r < 1e-7:
            rings.append([bm.verts.new(mat4 @ Vector((0, 0, z)))])
        else:
            rings.append([bm.verts.new(mat4 @ Vector((r * math.cos(spin + 2 * math.pi * i / verts),
                                                      r * math.sin(spin + 2 * math.pi * i / verts), z)))
                          for i in range(verts)])
    n = len(rings)
    for a in range(n if closed else n - 1):
        A, B = rings[a], rings[(a + 1) % n]
        if len(A) == 1 and len(B) == 1:
            continue
        for i in range(verts):
            i2 = (i + 1) % verts
            if len(A) == 1:
                bm.faces.new([A[0], B[i2], B[i]])
            elif len(B) == 1:
                bm.faces.new([A[i], A[i2], B[0]])
            else:
                bm.faces.new([A[i], A[i2], B[i2], B[i]])
    return _finish(name, bm, mat, group, bevel, segs, sharp)

def torus(name, mat, center, R_, r_, axis="+Z", group="Body", verts=32, rverts=10, sharp=60):
    prof = [(R_ + r_ * math.cos(2 * math.pi * k / rverts), r_ * math.sin(2 * math.pi * k / rverts))
            for k in range(rverts)]
    return lathe(name, mat, center, prof, axis, group, verts, 0.0, 2, sharp, closed=True)

def tube(name, mat, pts, radius, group="Body", verts=16, sharp=60, bend=None):
    """Pipe through UE-cm points (polyline, optional rounded corners via bend radius cm)."""
    P = [U(*p) for p in pts]
    if bend:
        b = bend / 100.0
        Q = [P[0]]
        for i in range(1, len(P) - 1):
            a, c, d = P[i - 1], P[i], P[i + 1]
            u1 = (a - c).normalized(); u2 = (d - c).normalized()
            p0 = c + u1 * min(b, (a - c).length * 0.45)
            p1 = c + u2 * min(b, (d - c).length * 0.45)
            for k in range(7):
                t = k / 6
                Q.append((1 - t) ** 2 * p0 + 2 * (1 - t) * t * c + t ** 2 * p1)
        Q.append(P[-1])
        P = Q
    r = radius / 100.0
    bm = bmesh.new()
    rings = []
    prev_n = None
    for i, p in enumerate(P):
        if i == 0:
            tdir = (P[1] - P[0]).normalized()
        elif i == len(P) - 1:
            tdir = (P[-1] - P[-2]).normalized()
        else:
            tdir = ((P[i + 1] - P[i]).normalized() + (P[i] - P[i - 1]).normalized()).normalized()
        if prev_n is None:
            ref = Vector((0, 0, 1)) if abs(tdir.z) < 0.9 else Vector((1, 0, 0))
            nrm = tdir.cross(ref).normalized()
        else:
            nrm = (prev_n - tdir * prev_n.dot(tdir)).normalized()
        prev_n = nrm
        bin_ = tdir.cross(nrm)
        rings.append([bm.verts.new(p + r * (math.cos(2 * math.pi * k / verts) * nrm +
                                             math.sin(2 * math.pi * k / verts) * bin_)) for k in range(verts)])
    for a in range(len(rings) - 1):
        A, B = rings[a], rings[a + 1]
        for k in range(verts):
            k2 = (k + 1) % verts
            bm.faces.new([A[k], A[k2], B[k2], B[k]])
    bm.faces.new(rings[0][::-1]); bm.faces.new(rings[-1])
    return _finish(name, bm, mat, group, 0.0, 2, sharp)

def prism(name, mat, pts2d, plane, d0, d1, group="Body", bevel=0.003, segs=2, sharp=40):
    """Extrude a CCW polygon. plane='XZ' -> pts are (x,z) UE cm extruded along UE Y from d0 to d1,
    'YZ' -> (y,z) along X, 'XY' -> (x,y) along Z."""
    bm = bmesh.new()
    def P3(a, b, d):
        if plane == "XZ":
            return U(a, d, b)
        if plane == "YZ":
            return U(d, a, b)
        return U(a, b, d)
    A = [bm.verts.new(P3(a, b, d0)) for a, b in pts2d]
    B = [bm.verts.new(P3(a, b, d1)) for a, b in pts2d]
    bm.faces.new(A); bm.faces.new(B)
    n = len(pts2d)
    for i in range(n):
        bm.faces.new([A[i], A[(i + 1) % n], B[(i + 1) % n], B[i]])
    return _finish(name, bm, mat, group, bevel, segs, sharp)

def planar_src_uv(center, size_cm, axis):
    """Return a callback that writes a planar UV_Src projection (0..1 over a square of size_cm
    centred at UE center, projected along UE axis) - used for decal/dial textures."""
    c = U(*center)
    s = size_cm / 100.0
    def cb(me):
        uv = me.uv_layers["UV_Src"]
        for poly in me.polygons:
            for li in poly.loop_indices:
                co = me.vertices[me.loops[li].vertex_index].co - c
                # u = viewer's right when looking at the face from outside (Blender coords)
                if axis in ("+X", "-X"):
                    u, v = (co.y if axis == "+X" else -co.y), co.z
                elif axis in ("+Y", "-Y"):   # UE -Y face == Blender +Y face
                    u, v = (-co.x if axis == "-Y" else co.x), co.z
                else:
                    u, v = co.x, co.y
                uv.data[li].uv = (u / s + 0.5, v / s + 0.5)
    return cb

def disc(name, mat, center, radius, thick, axis, group="Body", verts=48, src_uv=False):
    """Flat disc (dial face / label). Optional UV_Src planar mapping for image textures."""
    ob = cyl(name, mat, center, radius, thick, axis, group, verts, bevel=0.0)
    if src_uv:
        planar_src_uv(center, radius * 2, axis)(ob.data)
    return ob

# ------------------------------------------------------------------ palette (linear RGB)
def srgb(h):
    h = h.lstrip("#")
    c = [int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)]
    return tuple(((x / 12.92) if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4) for x in c)

# Colour ROLES come from _Pipeline/palettes.json (single source of truth):
#   themes  - zone looks (bath / hall / work ...): material kind + colour per role
#   assets  - theme per asset + per-asset overrides (user can set colours there)
# Build scripts call apply_asset_palette("<Asset>") before standard_mats().
# A global PALETTE="<theme>" set before exec forces a theme (used for palette studies).
_PJ = json.load(open(os.path.join(PIPE, "palettes.json"), encoding="utf-8"))
PAL = {}
ROLE_KIND = {}
AGE = {"v": _PJ["tiers"]["1"]}

def _cond():
    """Base-wear multiplier: 1 at age >= 0.35 (normal stylised wear), fading to 0 at age 0 (brand new)."""
    return max(0.0, min(1.0, AGE["v"] / 0.35))

def set_theme(theme, override=None):
    t = _PJ["themes"][theme]
    cols = {**_PJ["base"], **t["colors"]}
    kinds = dict(t.get("kinds", {}))
    for k, v in (override or {}).items():
        if isinstance(v, dict):
            if "color" in v:
                cols[k] = v["color"]
            if "kind" in v:
                kinds[k] = v["kind"]
        else:
            cols[k] = v
    PAL.clear(); PAL.update({k: srgb(v) for k, v in cols.items()})
    ROLE_KIND.clear(); ROLE_KIND.update(kinds)
    return theme

def apply_asset_palette(asset):
    forced = globals().get("PALETTE")
    a = _PJ["assets"].get(asset, {})
    age = globals().get("AGE_OVERRIDE", a.get("age"))
    tier = str(globals().get("TIER_OVERRIDE", a.get("tier", 1)))
    AGE["v"] = float(_PJ["tiers"][tier]) if age is None else float(age)
    if forced:
        return set_theme(forced)
    return set_theme(a.get("theme", "bath"), a.get("override"))

set_theme("bath")

# ------------------------------------------------------------------ material node helpers
def _nodes(m):
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial"); out.location = (1200, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled"); bsdf.name = "BSDF"; bsdf.location = (900, 0)
    nt.links.new(bsdf.outputs[0], out.inputs[0])
    tc = nt.nodes.new("ShaderNodeTexCoord"); tc.name = "TC"; tc.location = (-1600, 0)
    return nt, bsdf, tc

def _sock(nt, sock, val):
    if isinstance(val, (tuple, list)):
        sock.default_value = tuple(val) if len(val) == 4 else (*val, 1.0)
    elif isinstance(val, (int, float)):
        try:
            sock.default_value = val
        except TypeError:
            sock.default_value = (val, val, val, 1.0)
    elif val is not None:
        nt.links.new(val, sock)

def _noise(nt, tc, scale, detail=4, rough=0.55, w=0.0, dist=0.0, loc=(0, 0)):
    n = nt.nodes.new("ShaderNodeTexNoise"); n.location = loc
    n.noise_dimensions = "4D"
    n.inputs["Scale"].default_value = scale
    n.inputs["Detail"].default_value = detail
    n.inputs["Roughness"].default_value = rough
    n.inputs["W"].default_value = w
    n.inputs["Distortion"].default_value = dist
    nt.links.new(tc.outputs["Object"], n.inputs["Vector"])
    return n.outputs["Fac"]

def _mr(nt, src, a, b, c=0.0, d=1.0, loc=(0, 0)):
    m = nt.nodes.new("ShaderNodeMapRange"); m.location = loc; m.clamp = True
    m.inputs[1].default_value, m.inputs[2].default_value = a, b
    m.inputs[3].default_value, m.inputs[4].default_value = c, d
    nt.links.new(src, m.inputs[0])
    return m.outputs[0]

def _mix(nt, a, b, fac, blend="MIX", loc=(0, 0)):
    m = nt.nodes.new("ShaderNodeMix"); m.data_type = "RGBA"; m.blend_type = blend; m.location = loc
    m.clamp_result = True
    _sock(nt, m.inputs[0], fac); _sock(nt, m.inputs[6], a); _sock(nt, m.inputs[7], b)
    return m.outputs[2]

def _math(nt, op, a, b=None, loc=(0, 0), clamp=False):
    m = nt.nodes.new("ShaderNodeMath"); m.operation = op; m.use_clamp = clamp; m.location = loc
    _sock(nt, m.inputs[0], a)
    if b is not None:
        _sock(nt, m.inputs[1], b)
    return m.outputs[0]

def _edge(nt, radius, lo, hi, loc=(0, 0)):
    """Convex/concave edge mask from Cycles Bevel normal (works in bake)."""
    bev = nt.nodes.new("ShaderNodeBevel"); bev.location = loc; bev.samples = 8
    bev.inputs["Radius"].default_value = radius
    geo = nt.nodes.get("GEO") or nt.nodes.new("ShaderNodeNewGeometry")
    geo.name = "GEO"; geo.location = (loc[0], loc[1] - 200)
    dot = nt.nodes.new("ShaderNodeVectorMath"); dot.operation = "DOT_PRODUCT"; dot.location = (loc[0] + 200, loc[1])
    nt.links.new(bev.outputs[0], dot.inputs[0]); nt.links.new(geo.outputs["Normal"], dot.inputs[1])
    return _mr(nt, dot.outputs["Value"], hi, lo, 0.0, 1.0, (loc[0] + 400, loc[1]))

def _height_grad(nt, tc, z0, z1, lo, hi, loc=(0, 0)):
    sep = nt.nodes.new("ShaderNodeSeparateXYZ"); sep.location = loc
    nt.links.new(tc.outputs["Object"], sep.inputs[0])
    return _mr(nt, sep.outputs[2], z0, z1, lo, hi, (loc[0] + 200, loc[1]))

def _bump(nt, bsdf, height, strength, dist=0.002, loc=(600, -600)):
    b = nt.nodes.new("ShaderNodeBump"); b.location = loc
    b.inputs["Strength"].default_value = strength
    b.inputs["Distance"].default_value = dist
    nt.links.new(height, b.inputs["Height"])
    nt.links.new(b.outputs[0], bsdf.inputs["Normal"])

def _scale(c, k):
    return tuple(min(1.0, x * k) for x in c[:3])

def _toward(c, d, t):
    return tuple(c[i] * (1 - t) + d[i] * t for i in range(3))

def _get_mat(name):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m["bh_src"] = True
    return m

# ------------------------------------------------------------------ source materials
def mat_paint(name, color, under=None, under_metal=1.0, rough=0.5, wear=0.35, edge_light=0.45,
              seed=0.0, height=(0.0, 1.8), grime=0.45, glossy=False):
    """Stylized painted metal / enamel. wear 0..1 controls chip amount."""
    under = under or PAL["iron_lt"]
    m = _get_mat(name); m["bh_kind"] = "paint"
    nt, bsdf, tc = _nodes(m)
    var = _noise(nt, tc, 1.6, 3, 0.5, seed, loc=(-1300, 600))
    col = _mix(nt, _scale(color, 0.90), _scale(color, 1.07), var, loc=(-1000, 600))
    hg = _height_grad(nt, tc, height[0], height[1], 1.0 - grime * 0.45 * _cond(), 1.0 + 0.04 * _cond(), (-1300, 300))
    col = _mix(nt, col, hg, 1.0, "MULTIPLY", (-800, 500))
    # pale painted highlight on convex edges, broken by noise
    edge = _edge(nt, 0.006, 0.03, 0.22, (-1500, -100))
    en = _noise(nt, tc, 30.0, 3, 0.5, seed + 2.0, loc=(-1300, -350))
    edgef = _math(nt, "MULTIPLY", edge, _mr(nt, en, 0.35, 0.65, 0.45, 1.0, (-1100, -350)), (-900, -150), True)
    col = _mix(nt, col, _toward(_scale(color, 1.18), (1, 1, 1), 0.18), _math(nt, "MULTIPLY", edgef, edge_light),
               loc=(-600, 400))
    # hand-painted mottling (darker soft patches)
    mot = _noise(nt, tc, 4.5, 6, 0.6, seed + 7.0, loc=(-1300, 900))
    col = _mix(nt, col, _scale(color, 0.78), _mr(nt, mot, 0.48, 0.72, 0.0, 0.55 * _cond(), (-1100, 900)), loc=(-700, 800))
    # chips: noise + wide edge proximity
    edge_w = _edge(nt, 0.02, 0.03, 0.3, (-1500, -700))
    cn = _noise(nt, tc, 9.0, 8, 0.62, seed + 5.0, dist=0.15, loc=(-1300, -900))
    cv = _math(nt, "ADD", _math(nt, "MULTIPLY", cn, 0.85), _math(nt, "MULTIPLY", edge_w, 0.42), (-1000, -800))
    wear = min(1.0, wear + 0.45 * AGE["v"])
    thr = 0.575 - 0.08 * wear
    chip = _mr(nt, cv, thr, thr + 0.01, 0.0, _cond(), (-800, -800))
    chip_rim = _mr(nt, cv, thr - 0.035, thr, 0.0, _cond(), (-800, -1000))
    rim_col = _toward(_scale(color, 0.62), PAL["rust"], min(0.85, AGE["v"] * 0.9))
    col = _mix(nt, col, rim_col, _math(nt, "MULTIPLY", chip_rim, 0.75), loc=(-400, 300))
    col = _mix(nt, col, under, chip, loc=(-200, 300))
    nt.links.new(col, bsdf.inputs["Base Color"])
    r0 = 0.28 if glossy else rough
    rv = _mr(nt, var, 0.3, 0.7, r0 - 0.06, r0 + 0.08, (-400, -300))
    nt.links.new(_mix(nt, rv, 0.55, chip, loc=(-200, -300)), bsdf.inputs["Roughness"])
    nt.links.new(_math(nt, "MULTIPLY", chip, under_metal, (-200, -500)), bsdf.inputs["Metallic"])
    _bump(nt, bsdf, _math(nt, "SUBTRACT", 1.0, chip), 0.35, 0.0015)
    m.diffuse_color = (*color, 1)
    return m

def mat_metal(name, color, rough=0.32, metal=0.9, seed=0.0, edge_light=0.35, smudge=0.25):
    m = _get_mat(name)
    nt, bsdf, tc = _nodes(m)
    var = _noise(nt, tc, 4.0, 4, 0.55, seed, loc=(-1300, 500))
    col = _mix(nt, _scale(color, 0.82), _scale(color, 1.05), var, loc=(-1000, 500))
    edge = _edge(nt, 0.004, 0.03, 0.25, (-1500, -100))
    col = _mix(nt, col, _toward(color, (1, 1, 1), 0.35), _math(nt, "MULTIPLY", edge, edge_light), loc=(-700, 400))
    sm = _noise(nt, tc, 9.0, 6, 0.6, seed + 3.0, loc=(-1300, -400))
    nt.links.new(col, bsdf.inputs["Base Color"])
    nt.links.new(_mr(nt, sm, 0.35, 0.7, rough - 0.08, rough + smudge * 0.4, (-900, -400)), bsdf.inputs["Roughness"])
    bsdf.inputs["Metallic"].default_value = metal
    m.diffuse_color = (*color, 1)
    return m

def mat_iron(name, color, rough=0.5, metal=0.8, seed=0.0, rust=0.65):
    """Unpainted industrial steel/iron: mottled oxidation, bright worn edges, rust in cavities, floor grime."""
    m = _get_mat(name); m["bh_kind"] = "iron"
    nt, bsdf, tc = _nodes(m)
    var = _noise(nt, tc, 2.2, 4, 0.55, seed, loc=(-1300, 700))
    col = _mix(nt, _scale(color, 0.84), _scale(color, 1.07), var, loc=(-1000, 700))
    mot = _noise(nt, tc, 6.0, 7, 0.62, seed + 2.0, loc=(-1300, 450))
    col = _mix(nt, col, _scale(color, 0.62), _mr(nt, mot, 0.46, 0.7, 0.0, 0.8 * _cond(), (-1100, 450)), loc=(-800, 600))
    hg = _height_grad(nt, tc, 0.0, 1.0, 0.78, 1.03, (-1300, 250))
    col = _mix(nt, col, hg, 1.0, "MULTIPLY", (-650, 550))
    edge = _edge(nt, 0.005, 0.03, 0.22, (-1500, -100))
    edgen = _math(nt, "MULTIPLY", edge, _mr(nt, _noise(nt, tc, 25.0, 3, 0.5, seed + 4.0, loc=(-1300, -350)),
                                           0.35, 0.65, 0.4, 1.0, (-1100, -350)), (-900, -150))
    col = _mix(nt, col, _toward(color, (0.86, 0.88, 0.9), 0.65), _math(nt, "MULTIPLY", edgen, 0.75), loc=(-500, 500))
    ao = nt.nodes.new("ShaderNodeAmbientOcclusion"); ao.location = (-1500, -700); ao.only_local = True
    ao.inputs["Distance"].default_value = 0.04; ao.samples = 8
    cav = _mr(nt, ao.outputs["AO"], 0.6, 0.97, 1.0, 0.0, (-1300, -700))
    rn = _noise(nt, tc, 12.0, 8, 0.65, seed + 6.0, loc=(-1300, -950))
    rmask = _math(nt, "MULTIPLY", cav, _mr(nt, rn, 0.42, 0.6, 0.0, 1.0, (-1100, -950)), (-900, -800), True)
    rmask = _math(nt, "MULTIPLY", rmask, min(1.0, rust * (1.0 + 0.6 * AGE["v"])) * _cond(), (-750, -800), True)
    col = _mix(nt, col, PAL["rust"], rmask, loc=(-300, 400))
    nt.links.new(col, bsdf.inputs["Base Color"])
    r = _mr(nt, var, 0.3, 0.7, rough - 0.06, rough + 0.08, (-700, -300))
    r = _mix(nt, r, rough - 0.2, _math(nt, "MULTIPLY", edgen, 0.8), loc=(-500, -300))
    nt.links.new(_mix(nt, r, 0.85, rmask, loc=(-300, -300)), bsdf.inputs["Roughness"])
    nt.links.new(_math(nt, "MULTIPLY", _math(nt, "SUBTRACT", 1.0, rmask), metal, (-300, -500)), bsdf.inputs["Metallic"])
    _bump(nt, bsdf, _math(nt, "ADD", _math(nt, "MULTIPLY", mot, 0.6), rmask), 0.25, 0.002)
    m.diffuse_color = (*color, 1)
    return m

def _role_mat(name, role, seed, **paint_kw):
    """Material for a colour role, honouring the theme's material kind (paint | wood | metal)."""
    kind = ROLE_KIND.get(role, "paint")
    c = PAL[role]
    if kind == "wood":
        return mat_wood(name, c, _scale(c, 0.6), axis="Z", seed=seed)
    if kind == "metal":
        return mat_iron(name, c, seed=seed)
    return mat_paint(name, c, seed=seed, **paint_kw)

def mat_plastic(name, color, rough=0.45, seed=0.0, edge_light=0.3):
    m = _get_mat(name)
    nt, bsdf, tc = _nodes(m)
    var = _noise(nt, tc, 3.0, 3, 0.5, seed, loc=(-1300, 500))
    col = _mix(nt, _scale(color, 0.92), _scale(color, 1.06), var, loc=(-1000, 500))
    edge = _edge(nt, 0.003, 0.03, 0.25, (-1500, -100))
    col = _mix(nt, col, _toward(color, (1, 1, 1), 0.3), _math(nt, "MULTIPLY", edge, edge_light), loc=(-700, 400))
    nt.links.new(col, bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Metallic"].default_value = 0.0
    m.diffuse_color = (*color, 1)
    return m

def mat_wood(name, color=None, dark=None, axis="X", rough=0.6, seed=0.0, ring_scale=6.0, edge_col=None):
    """Painted-look stylized wood: soft bands along `axis` (Blender axis), lighter edges."""
    color = color or PAL["wood"]; dark = dark or PAL["wood_dk"]
    m = _get_mat(name); m["bh_kind"] = "wood"
    nt, bsdf, tc = _nodes(m)
    mp = nt.nodes.new("ShaderNodeMapping"); mp.location = (-1400, 400)
    sc_ = {"X": (1.0, 7.0, 7.0), "Y": (7.0, 1.0, 7.0), "Z": (7.0, 7.0, 1.0)}[axis]
    mp.inputs["Scale"].default_value = sc_
    nt.links.new(tc.outputs["Object"], mp.inputs["Vector"])
    wv = nt.nodes.new("ShaderNodeTexNoise"); wv.location = (-1200, 400); wv.noise_dimensions = "4D"
    wv.inputs["Scale"].default_value = ring_scale; wv.inputs["Detail"].default_value = 3
    wv.inputs["Distortion"].default_value = 1.5; wv.inputs["W"].default_value = seed
    nt.links.new(mp.outputs[0], wv.inputs["Vector"])
    bands = _mr(nt, wv.outputs["Fac"], 0.35, 0.68, 0.0, 1.0, (-1000, 400))
    col = _mix(nt, dark, color, bands, loc=(-800, 400))
    var = _noise(nt, tc, 1.5, 3, 0.5, seed + 1, loc=(-1300, 700))
    col = _mix(nt, col, _scale(color, 1.12), _math(nt, "MULTIPLY", var, 0.35), loc=(-650, 500))
    edge = _edge(nt, 0.005, 0.03, 0.25, (-1500, -200))
    edge_col = edge_col or (PAL["wood_lt"] if color == PAL["wood"] else _toward(color, (1, 1, 1), 0.22))
    col = _mix(nt, col, edge_col, _math(nt, "MULTIPLY", edge, 0.6), loc=(-450, 400))
    hg = _height_grad(nt, tc, 0.0, 1.2, 0.86, 1.03, (-650, 800))
    col = _mix(nt, col, hg, 1.0, "MULTIPLY", (-300, 600))
    ag = AGE["v"]
    if ag > 0:
        # water-stain blotches
        bl = _noise(nt, tc, 3.2, 6, 0.6, seed + 9.0, loc=(-650, 1050))
        col = _mix(nt, col, _scale(dark, 0.8), _mr(nt, bl, 0.55, 0.75, 0.0, 0.55 * ag, (-450, 1050)), loc=(-150, 700))
        # fine scratches (thin stretched noise lines, lighter raw wood)
        mp2 = nt.nodes.new("ShaderNodeMapping"); mp2.location = (-850, 1300)
        mp2.inputs["Scale"].default_value = (40.0, 40.0, 2.5)
        mp2.inputs["Rotation"].default_value = (0.4, 0.3, 0.6)
        nt.links.new(tc.outputs["Object"], mp2.inputs["Vector"])
        sc2 = nt.nodes.new("ShaderNodeTexNoise"); sc2.location = (-650, 1300); sc2.noise_dimensions = "3D"
        sc2.inputs["Scale"].default_value = 1.0; sc2.inputs["Detail"].default_value = 2
        nt.links.new(mp2.outputs[0], sc2.inputs["Vector"])
        scr = _mr(nt, sc2.outputs["Fac"], 0.68, 0.72, 0.0, 0.6 * ag, (-450, 1300))
        col = _mix(nt, col, _toward(color, (1, 0.95, 0.85), 0.35), scr, loc=(0, 700))
        # worn edges: wider pale raw-wood band
        ew = _edge(nt, 0.012, 0.03, 0.28, (-850, 1550))
        col = _mix(nt, col, _toward(color, (1, 0.93, 0.8), 0.3), _math(nt, "MULTIPLY", ew, 0.55 * ag), loc=(150, 700))
    nt.links.new(col, bsdf.inputs["Base Color"])
    nt.links.new(_mr(nt, bands, 0, 1, rough + 0.08, rough - 0.05, (-500, -200)), bsdf.inputs["Roughness"])
    _bump(nt, bsdf, bands, 0.15, 0.002)
    m.diffuse_color = (*color, 1)
    return m

def mat_tile(name, color, grout=None, tile_cm=10.0, plane="XZ", rough=0.25, seed=0.0, vary=0.12, offset=0.0):
    """Glazed square tiles with grout lines. plane = Blender plane the tiles lie in."""
    grout = grout or PAL["grout"]
    m = _get_mat(name)
    nt, bsdf, tc = _nodes(m)
    sep = nt.nodes.new("ShaderNodeSeparateXYZ"); sep.location = (-1450, 300)
    nt.links.new(tc.outputs["Object"], sep.inputs[0])
    comb = nt.nodes.new("ShaderNodeCombineXYZ"); comb.location = (-1250, 300)
    a, b = {"XZ": (0, 2), "YZ": (1, 2), "XY": (0, 1)}[plane]
    nt.links.new(sep.outputs[a], comb.inputs[0]); nt.links.new(sep.outputs[b], comb.inputs[1])
    br = nt.nodes.new("ShaderNodeTexBrick"); br.location = (-1050, 300)
    br.offset = offset; br.squash = 1.0
    br.inputs["Scale"].default_value = 1.0 / (tile_cm / 100.0)
    br.inputs["Brick Width"].default_value = 1.0
    br.inputs["Row Height"].default_value = 1.0
    br.inputs["Mortar Size"].default_value = 0.035
    br.inputs["Mortar Smooth"].default_value = 0.3
    br.inputs["Color1"].default_value = (*_scale(color, 0.88), 1)
    br.inputs["Color2"].default_value = (*_scale(color, 1.08), 1)
    br.inputs["Mortar"].default_value = (*grout, 1)
    nt.links.new(comb.outputs[0], br.inputs["Vector"])
    var = _noise(nt, tc, 2.0, 3, 0.5, seed, loc=(-1300, 650))
    col = _mix(nt, br.outputs["Color"], _scale(color, 0.85), _math(nt, "MULTIPLY", var, vary * 2), loc=(-700, 400))
    edge = _edge(nt, 0.004, 0.03, 0.25, (-1500, -300))
    col = _mix(nt, col, _toward(color, (1, 1, 1), 0.3), _math(nt, "MULTIPLY", edge, 0.4), loc=(-500, 400))
    nt.links.new(col, bsdf.inputs["Base Color"])
    nt.links.new(_mr(nt, br.outputs["Fac"], 0, 1, rough, 0.85, (-600, -100)), bsdf.inputs["Roughness"])
    _bump(nt, bsdf, _math(nt, "SUBTRACT", 1.0, br.outputs["Fac"]), 0.5, 0.003)
    m.diffuse_color = (*color, 1)
    return m

def mat_image(name, path, rough=0.4, metal=0.0, tint=None):
    """Image texture on UV_Src (dials, labels, number plates)."""
    m = _get_mat(name)
    nt, bsdf, tc = _nodes(m)
    uvn = nt.nodes.new("ShaderNodeUVMap"); uvn.uv_map = "UV_Src"; uvn.location = (-1300, 300)
    im = nt.nodes.new("ShaderNodeTexImage"); im.location = (-1000, 300)
    img = bpy.data.images.load(path, check_existing=True)
    im.image = img
    im.extension = "CLIP"
    nt.links.new(uvn.outputs[0], im.inputs[0])
    col = im.outputs["Color"]
    if tint:
        col = _mix(nt, col, tint, 1.0, "MULTIPLY")
    nt.links.new(col, bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Metallic"].default_value = metal
    return m

def mat_flat(name, color, rough=0.5, metal=0.0):
    m = _get_mat(name)
    nt, bsdf, tc = _nodes(m)
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Metallic"].default_value = metal
    m.diffuse_color = (*color, 1)
    return m

def mat_mirror(name):
    """Stylized mirror: pale teal gradient with soft diagonal streaks, glossy metal."""
    m = _get_mat(name)
    nt, bsdf, tc = _nodes(m)
    sep = nt.nodes.new("ShaderNodeSeparateXYZ"); sep.location = (-1400, 300)
    nt.links.new(tc.outputs["Object"], sep.inputs[0])
    diag = _math(nt, "ADD", sep.outputs[0], sep.outputs[2])
    st = nt.nodes.new("ShaderNodeTexWave"); st.location = (-1100, 300)
    st.inputs["Scale"].default_value = 2.5
    st.inputs["Distortion"].default_value = 0.0
    nt.links.new(tc.outputs["Object"], st.inputs["Vector"])
    st.bands_direction = "DIAGONAL"
    streak = _mr(nt, st.outputs["Fac"], 0.75, 1.0, 0.0, 1.0, (-900, 300))
    g = _mr(nt, sep.outputs[2], 0.6, 2.0, 0.0, 1.0, (-900, 600))
    col = _mix(nt, _scale(PAL["mirror"], 0.62), PAL["mirror"], g, loc=(-700, 500))
    col = _mix(nt, col, (0.95, 0.98, 0.98), _math(nt, "MULTIPLY", streak, 0.45), loc=(-500, 500))
    nt.links.new(col, bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.06
    bsdf.inputs["Metallic"].default_value = 0.85
    return m

# ------------------------------------------------------------------ standard set
def standard_mats(prefix):
    """Shared library so every facility reads as one set. Keys are colour ROLES, not colour names."""
    P = prefix
    return {
        "body":    _role_mat(P + "_Body", "body", 1.0),
        "body_dk": _role_mat(P + "_BodyDark", "body_dk", 2.0),
        "light":   _role_mat(P + "_Light", "light", 3.0, under=PAL["iron_lt"], wear=0.25),
        "accent":  _role_mat(P + "_Accent", "accent", 4.0, wear=0.3, glossy=True),
        "hot":     _role_mat(P + "_Hot", "hot", 14.0, wear=0.3, glossy=True),
        "cold":    _role_mat(P + "_Cold", "cold", 15.0, wear=0.3, glossy=True),
        "white":   mat_paint(P + "_Enamel", PAL["white"], under=PAL["iron"], wear=0.15, seed=5.0, glossy=True),
        "chrome":  mat_metal(P + "_Chrome", PAL["chrome"], rough=0.22, metal=0.92, seed=6.0),
        "steel":   mat_metal(P + "_Steel", PAL["steel"], rough=0.38, metal=0.85, seed=7.0),
        "iron":    mat_metal(P + "_Iron", PAL["iron"], rough=0.55, metal=0.6, seed=8.0, edge_light=0.5),
        "brass":   mat_metal(P + "_Brass", PAL["brass"], rough=0.3, metal=0.95, seed=9.0),
        "copper":  mat_metal(P + "_Copper", PAL["copper"], rough=0.35, metal=0.95, seed=31.0),
        "rubber":  mat_plastic(P + "_Rubber", PAL["black"], rough=0.7, seed=10.0, edge_light=0.15),
        "bake":    mat_plastic(P + "_Bakelite", PAL["bakelite"], rough=0.35, seed=11.0),
        "wood":    mat_wood(P + "_Wood", seed=12.0),
        "label":   mat_plastic(P + "_Label", PAL["label"], rough=0.5, seed=13.0, edge_light=0.1),
    }

# ------------------------------------------------------------------ dial texture (rendered, no PIL)
def make_dial(path, labels=("0", "1", "2", "3", "4"), unit="bar", red_from=0.75, res=1024,
              face=None, sweep=(225.0, -45.0), title=None):
    """Render a stylized gauge dial (no needle) to PNG via a throwaway scene (orthographic top view)."""
    face = face or PAL["label"]
    ink = PAL["ink"]
    red = PAL["dial_red"]
    scn = bpy.data.scenes.new("_DialScene")
    col_ = bpy.data.collections.new("_Dial"); scn.collection.children.link(col_)
    def emit(name, c):
        m = bpy.data.materials.new(name); m.use_nodes = True
        nt = m.node_tree; nt.nodes.clear()
        o = nt.nodes.new("ShaderNodeOutputMaterial"); e = nt.nodes.new("ShaderNodeEmission")
        e.inputs[0].default_value = (*c, 1); nt.links.new(e.outputs[0], o.inputs[0])
        return m
    mf, mi, mr = emit("_dial_face", face), emit("_dial_ink", ink), emit("_dial_red", red)
    objs = []
    def mesh_from(bm, name, mat, z):
        me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
        ob = bpy.data.objects.new(name, me); col_.objects.link(ob); me.materials.append(mat)
        ob.location.z = z; objs.append(ob); return ob
    bm = bmesh.new(); bmesh.ops.create_circle(bm, cap_ends=True, segments=128, radius=1.0)
    mesh_from(bm, "_face", mf, 0)
    # outer ring
    bm = bmesh.new()
    a = bmesh.ops.create_circle(bm, segments=128, radius=0.985)["verts"]
    b = bmesh.ops.create_circle(bm, segments=128, radius=0.93)["verts"]
    for i in range(128):
        bm.faces.new([a[i], a[(i + 1) % 128], b[(i + 1) % 128], b[i]])
    mesh_from(bm, "_ring", mi, 0.01)
    A0, A1 = math.radians(sweep[0]), math.radians(sweep[1])
    def ang(t):
        return A0 + (A1 - A0) * t
    def arc_band(r0, r1, t0, t1, mat, name, n=64):
        bm = bmesh.new()
        vs = []
        for k in range(n + 1):
            t = t0 + (t1 - t0) * k / n
            aa = ang(t)
            vs.append((bm.verts.new((r0 * math.cos(aa), r0 * math.sin(aa), 0)),
                       bm.verts.new((r1 * math.cos(aa), r1 * math.sin(aa), 0))))
        for k in range(n):
            bm.faces.new([vs[k][0], vs[k + 1][0], vs[k + 1][1], vs[k][1]])
        mesh_from(bm, name, mat, 0.012)
    arc_band(0.70, 0.86, red_from, 1.0, mr, "_red")
    arc_band(0.855, 0.875, 0.0, 1.0, mi, "_arc")
    nmaj = len(labels) - 1
    for k in range(nmaj * 5 + 1):
        t = k / (nmaj * 5)
        aa = ang(t)
        major = k % 5 == 0
        r0 = 0.66 if major else 0.76
        w = 0.035 if major else 0.016
        bm = bmesh.new()
        d = Vector((math.cos(aa), math.sin(aa), 0)); p = Vector((-d.y, d.x, 0))
        q = [bm.verts.new(d * r0 + p * w / 2), bm.verts.new(d * 0.875 + p * w / 2),
             bm.verts.new(d * 0.875 - p * w / 2), bm.verts.new(d * r0 - p * w / 2)]
        bm.faces.new(q)
        mesh_from(bm, "_tick%d" % k, mi, 0.014)
    font = None
    for fp in (r"C:\Windows\Fonts\georgiab.ttf", r"C:\Windows\Fonts\arialbd.ttf"):
        if os.path.exists(fp):
            font = bpy.data.fonts.load(fp, check_existing=True); break
    def text(s, pos, size, mat):
        cu = bpy.data.curves.new("_t", "FONT"); cu.body = s; cu.size = size
        cu.align_x = "CENTER"; cu.align_y = "CENTER"
        if font:
            cu.font = font
        ob = bpy.data.objects.new("_t", cu); ob.location = (pos[0], pos[1], 0.02)
        col_.objects.link(ob); ob.data.materials.append(mat); objs.append(ob)
    for k, s in enumerate(labels):
        aa = ang(k / nmaj)
        text(s, (0.53 * math.cos(aa), 0.53 * math.sin(aa)), 0.27, mi)
    text(unit, (0, -0.36), 0.22, mi)
    if title:
        text(title, (0, -0.62), 0.15, mr)
    cam = bpy.data.objects.new("_dcam", bpy.data.cameras.new("_dcam")); col_.objects.link(cam)
    cam.data.type = "ORTHO"; cam.data.ortho_scale = 2.0; cam.location = (0, 0, 5)
    scn.camera = cam
    scn.render.engine = "BLENDER_EEVEE"
    scn.render.resolution_x = scn.render.resolution_y = res
    scn.render.film_transparent = False
    scn.view_settings.view_transform = "Standard"
    w = bpy.data.worlds.new("_dw"); w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (*face, 1)
    scn.world = w
    scn.render.filepath = path
    scn.render.image_settings.file_format = "PNG"
    bpy.ops.render.render(write_still=True, scene=scn.name)
    for o in list(col_.objects):
        d = o.data
        bpy.data.objects.remove(o, do_unlink=True)
        try:
            (bpy.data.meshes if isinstance(d, bpy.types.Mesh) else bpy.data.curves if isinstance(d, bpy.types.Curve)
             else bpy.data.cameras).remove(d)
        except Exception:
            pass
    bpy.data.collections.remove(col_)
    for m in (mf, mi, mr):
        bpy.data.materials.remove(m)
    bpy.data.worlds.remove(w)
    bpy.data.scenes.remove(scn)
    return path


def make_label(path, lines, size=(1024, 512), bg=None, ink=None, border=True, sizes=None, font_file=None):
    """Render a simple enamel/brass plate graphic (text lines) to PNG."""
    bg = bg or PAL["label"]; ink = ink or PAL["ink"]
    W, H = size
    asp = W / H
    scn = bpy.data.scenes.new("_LabelScene")
    col_ = bpy.data.collections.new("_Label"); scn.collection.children.link(col_)
    m = bpy.data.materials.new("_lbl_ink"); m.use_nodes = True
    nt = m.node_tree; nt.nodes.clear()
    o = nt.nodes.new("ShaderNodeOutputMaterial"); e = nt.nodes.new("ShaderNodeEmission")
    e.inputs[0].default_value = (*ink, 1); nt.links.new(e.outputs[0], o.inputs[0])
    objs = []
    fp = font_file or r"C:\Windows\Fonts\georgiab.ttf"
    font = bpy.data.fonts.load(fp, check_existing=True) if os.path.exists(fp) else None
    n = len(lines)
    sizes = sizes or [0.55 / max(1, n)] * n
    ys = [((n - 1) / 2 - i) * (1.6 / max(n, 1)) * 0.62 for i in range(n)]
    for s_, y, sz in zip(lines, ys, sizes):
        cu = bpy.data.curves.new("_t", "FONT"); cu.body = s_; cu.size = sz
        cu.align_x = "CENTER"; cu.align_y = "CENTER"
        if font:
            cu.font = font
        ob = bpy.data.objects.new("_t", cu); ob.location = (0, y, 0)
        col_.objects.link(ob); cu.materials.append(m); objs.append(ob)
    if border:
        bm = bmesh.new()
        def rect(w, h):
            return [bm.verts.new((sx * w, sy * h, 0)) for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
        a = rect(asp - 0.06, 0.94); b = rect(asp - 0.10, 0.90)
        for i in range(4):
            bm.faces.new([a[i], a[(i + 1) % 4], b[(i + 1) % 4], b[i]])
        me = bpy.data.meshes.new("_b"); bm.to_mesh(me); bm.free()
        ob = bpy.data.objects.new("_b", me); col_.objects.link(ob); me.materials.append(m); objs.append(ob)
    cam = bpy.data.objects.new("_lcam", bpy.data.cameras.new("_lcam")); col_.objects.link(cam)
    cam.data.type = "ORTHO"; cam.data.ortho_scale = 2.0 * asp; cam.location = (0, 0, 5)
    scn.camera = cam
    scn.render.engine = "BLENDER_EEVEE"
    scn.render.resolution_x, scn.render.resolution_y = W, H
    scn.view_settings.view_transform = "Standard"
    w = bpy.data.worlds.new("_lw"); w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (*bg, 1)
    scn.world = w
    scn.render.filepath = path
    scn.render.image_settings.file_format = "PNG"
    bpy.ops.render.render(write_still=True, scene=scn.name)
    for ob in list(col_.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    bpy.data.collections.remove(col_)
    bpy.data.materials.remove(m); bpy.data.worlds.remove(w); bpy.data.scenes.remove(scn)
    return path

def plate_src_uv(center, w_cm, h_cm, axis):
    """Planar UV_Src over a w x h rectangle (UE cm) facing UE axis."""
    c = U(*center)
    def cb(me):
        uv = me.uv_layers["UV_Src"]
        for poly in me.polygons:
            for li in poly.loop_indices:
                co = me.vertices[me.loops[li].vertex_index].co - c
                if axis in ("+X", "-X"):
                    u = co.y if axis == "+X" else -co.y
                else:
                    u = -co.x if axis == "-Y" else co.x
                uv.data[li].uv = (u / (w_cm / 100.0) + 0.5, co.z / (h_cm / 100.0) + 0.5)
    return cb

def mat_tile_box(name, color, grout=None, tile_cm=8.0, rough=0.22, seed=0.0, vary=0.12, color2=None):
    """Glazed square tiles projected per dominant face normal (box mapping) - for tiled blocks/walls."""
    grout = _toward(grout or PAL["tile_grout"], PAL["dirt"], 0.45 * AGE["v"])
    color2 = color2 or _scale(color, 1.08)
    m = _get_mat(name)
    nt, bsdf, tc = _nodes(m)
    sepn = nt.nodes.new("ShaderNodeSeparateXYZ"); sepn.location = (-1400, -400)
    nt.links.new(tc.outputs["Normal"], sepn.inputs[0])      # object-space normal (rotation independent)
    ax = [_math(nt, "ABSOLUTE", sepn.outputs[i]) for i in range(3)]
    mx = _math(nt, "GREATER_THAN", ax[0], _math(nt, "MAXIMUM", ax[1], ax[2]))
    my = _math(nt, "MULTIPLY", _math(nt, "SUBTRACT", 1.0, mx), _math(nt, "GREATER_THAN", ax[1], ax[2]))
    sep = nt.nodes.new("ShaderNodeSeparateXYZ"); sep.location = (-1400, 300)
    nt.links.new(tc.outputs["Object"], sep.inputs[0])
    def brick(a, b, loc):
        comb = nt.nodes.new("ShaderNodeCombineXYZ"); comb.location = (loc[0] - 200, loc[1])
        nt.links.new(sep.outputs[a], comb.inputs[0]); nt.links.new(sep.outputs[b], comb.inputs[1])
        br = nt.nodes.new("ShaderNodeTexBrick"); br.location = loc
        br.offset = 0.0; br.squash = 1.0
        br.inputs["Scale"].default_value = 1.0 / (tile_cm / 100.0)
        br.inputs["Brick Width"].default_value = 1.0
        br.inputs["Row Height"].default_value = 1.0
        br.inputs["Mortar Size"].default_value = 0.04
        br.inputs["Mortar Smooth"].default_value = 0.3
        br.inputs["Bias"].default_value = 0.0
        br.inputs["Color1"].default_value = (*_scale(color, 0.9), 1)
        br.inputs["Color2"].default_value = (*color2, 1)
        br.inputs["Mortar"].default_value = (*grout, 1)
        nt.links.new(comb.outputs[0], br.inputs["Vector"])
        return br
    bx, by, bz = brick(1, 2, (-1000, 600)), brick(0, 2, (-1000, 300)), brick(0, 1, (-1000, 0))
    def sel(o):
        c = _mix(nt, bz.outputs[o], by.outputs[o], my, loc=(-700, 200))
        return _mix(nt, c, bx.outputs[o], mx, loc=(-550, 200))
    col = sel("Color")
    fac = _mix(nt, bz.outputs["Fac"], by.outputs["Fac"], my, loc=(-700, -100))
    fac = _mix(nt, fac, bx.outputs["Fac"], mx, loc=(-550, -100))
    var = _noise(nt, tc, 2.0, 3, 0.5, seed, loc=(-1300, 900))
    col = _mix(nt, col, _scale(color, 0.82), _math(nt, "MULTIPLY", var, vary * 2), loc=(-400, 400))
    edge = _edge(nt, 0.004, 0.03, 0.25, (-1500, -900))
    col = _mix(nt, col, _toward(color, (1, 1, 1), 0.3), _math(nt, "MULTIPLY", edge, 0.35), loc=(-250, 400))
    nt.links.new(col, bsdf.inputs["Base Color"])
    nt.links.new(_mr(nt, fac, 0, 1, rough, 0.85, (-300, -100)), bsdf.inputs["Roughness"])
    _bump(nt, bsdf, _math(nt, "SUBTRACT", 1.0, fac), 0.6, 0.003)
    m.diffuse_color = (*color, 1)
    return m


# ------------------------------------------------------------------ ageing layer (applied last)
def _age_material(m, strength):
    """Insert an ageing stack between the BSDF inputs and their current sources:
    cavity dirt (AO), drip streaks from top, floor grime, dust on up-facing faces,
    verdigris in brass/copper cavities, duller/rougher surface where dirty."""
    nt = m.node_tree
    bsdf = nt.nodes.get("BSDF")
    if bsdf is None or m.get("bh_aged"):
        return False
    tc = nt.nodes.get("TC") or nt.nodes.new("ShaderNodeTexCoord")
    tc.name = "TC"
    X, Y = 300, -1400
    bc = bsdf.inputs["Base Color"]
    col = bc.links[0].from_socket if bc.links else tuple(bc.default_value)
    # cavity dirt
    ao = nt.nodes.new("ShaderNodeAmbientOcclusion"); ao.location = (X - 900, Y); ao.only_local = True
    ao.inputs["Distance"].default_value = 0.06; ao.samples = 8
    cav = _mr(nt, ao.outputs["AO"], 0.7, 0.99, 1.0, 0.0, (X - 700, Y))
    dn = _noise(nt, tc, 5.0, 5, 0.6, 41.0, loc=(X - 900, Y - 250))
    cavd = _math(nt, "MULTIPLY", cav, _mr(nt, dn, 0.3, 0.7, 0.45, 1.0, (X - 700, Y - 250)), (X - 500, Y), True)
    cavd = _math(nt, "MULTIPLY", cavd, 1.0 * strength, (X - 350, Y), True)
    name = m.name.lower()
    cav_col = PAL["verdigris"] if ("brass" in name or "copper" in name) else PAL["dirt"]
    col = _mix(nt, col, cav_col, cavd, loc=(X - 150, Y + 200))
    # drip streaks (stretched along Z), stronger near the top edge of each surface run
    mp = nt.nodes.new("ShaderNodeMapping"); mp.location = (X - 1100, Y - 550)
    mp.inputs["Scale"].default_value = (14.0, 14.0, 0.9)
    nt.links.new(tc.outputs["Object"], mp.inputs["Vector"])
    sn = nt.nodes.new("ShaderNodeTexNoise"); sn.location = (X - 900, Y - 550); sn.noise_dimensions = "4D"
    sn.inputs["Scale"].default_value = 1.0; sn.inputs["Detail"].default_value = 4
    sn.inputs["W"].default_value = 7.0
    nt.links.new(mp.outputs[0], sn.inputs["Vector"])
    streak = _mr(nt, sn.outputs["Fac"], 0.48, 0.62, 0.0, 1.0, (X - 700, Y - 550))
    streak = _math(nt, "MULTIPLY", streak, 0.8 * strength, (X - 500, Y - 550), True)
    kind = m.get("bh_kind", "")
    streak_col = {"iron": PAL["rust"], "paint": _toward(PAL["dirt"], PAL["rust"], 0.5),
                  "wood": _scale(PAL["dirt"], 0.75)}.get(kind, PAL["dirt"])
    col = _mix(nt, col, streak_col, _math(nt, "MULTIPLY", streak, 0.7), loc=(X, Y + 150))
    # floor grime band
    sep = nt.nodes.new("ShaderNodeSeparateXYZ"); sep.location = (X - 900, Y - 800)
    nt.links.new(tc.outputs["Object"], sep.inputs[0])
    floor = _mr(nt, sep.outputs[2], 0.0, 0.45, 1.0, 0.0, (X - 700, Y - 800))
    gn = _noise(nt, tc, 7.0, 5, 0.6, 43.0, loc=(X - 900, Y - 1000))
    floor = _math(nt, "MULTIPLY", floor, _mr(nt, gn, 0.3, 0.7, 0.4, 1.0, (X - 700, Y - 1000)), (X - 500, Y - 800), True)
    floor = _math(nt, "MULTIPLY", floor, 0.9 * strength, (X - 350, Y - 800), True)
    col = _mix(nt, col, PAL["dirt"], floor, loc=(X + 150, Y + 100))
    # dust on up-facing surfaces (object-space normal)
    sepn = nt.nodes.new("ShaderNodeSeparateXYZ"); sepn.location = (X - 900, Y - 1250)
    nt.links.new(tc.outputs["Normal"], sepn.inputs[0])
    up = _mr(nt, sepn.outputs[2], 0.6, 0.95, 0.0, 1.0, (X - 700, Y - 1250))
    dsn = _noise(nt, tc, 9.0, 6, 0.6, 47.0, loc=(X - 900, Y - 1450))
    dust = _math(nt, "MULTIPLY", up, _mr(nt, dsn, 0.35, 0.7, 0.2, 1.0, (X - 700, Y - 1450)), (X - 500, Y - 1250), True)
    dust = _math(nt, "MULTIPLY", dust, 0.4 * strength, (X - 350, Y - 1250), True)
    col = _mix(nt, col, PAL["dust"], dust, loc=(X + 300, Y + 50))
    # faded, slightly yellowed overall
    hsv = nt.nodes.new("ShaderNodeHueSaturation"); hsv.location = (X + 450, Y + 50)
    hsv.inputs["Saturation"].default_value = 1.0 - 0.28 * strength
    hsv.inputs["Value"].default_value = 1.0 - 0.1 * strength
    nt.links.new(col, hsv.inputs["Color"])
    col = _mix(nt, hsv.outputs["Color"], (0.85, 0.75, 0.55), 0.1 * strength, "MULTIPLY", (X + 600, Y + 50))
    nt.links.new(col, bc)
    # roughness/metallic: dirtier = rougher, less metallic
    dirty = _math(nt, "MAXIMUM", _math(nt, "MAXIMUM", cavd, floor), dust, (X + 150, Y - 600), True)
    ri = bsdf.inputs["Roughness"]
    rsrc = ri.links[0].from_socket if ri.links else float(ri.default_value)
    nt.links.new(_mix(nt, rsrc, 0.9, dirty, loc=(X + 300, Y - 600)), ri)
    mi = bsdf.inputs["Metallic"]
    msrc = mi.links[0].from_socket if mi.links else float(mi.default_value)
    nt.links.new(_math(nt, "MULTIPLY", msrc, _math(nt, "SUBTRACT", 1.0, dirty), (X + 300, Y - 800), True), mi)
    m["bh_aged"] = True
    return True

def age_all_materials(strength=None):
    """Apply the ageing layer to every source material created by this build (call once, at the end)."""
    st = AGE["v"] if strength is None else strength
    n = 0
    if st <= 0.0:
        return 0
    for m in bpy.data.materials:
        if m.get("bh_src") and m.use_nodes:
            k = st * (0.45 if m.node_tree.nodes.get("TC") and any(
                nd.type == "UVMAP" and nd.uv_map == "UV_Src" for nd in m.node_tree.nodes) else 1.0)
            n += _age_material(m, k)
    return n
