"""Boiler_02 - vintage red industrial boiler, built from the 8-view reference sheet.

Run inside Blender 5.2 (Blender MCP execute or Text Editor). Rebuilds the
"Boiler02_Parts" collection from scratch with procedural source materials.
Units: metres. Pivot: bottom centre (0,0,0). Front faces -Y, valves on +X.
"""
import bpy, bmesh, math
from mathutils import Vector, Matrix

# ---------------------------------------------------------------- dimensions
W, D = 0.70, 0.455
HX, HY = W / 2, D / 2
LEG_H = 0.09
Z0, Z1 = LEG_H, 1.225            # painted shell bottom / top
ZC, HZ = (Z0 + Z1) / 2, (Z1 - Z0) / 2
FY = -HY                          # front face plane
CH_Y = 0.06                       # chimney pushed slightly to the back
SEAM = 0.07                       # seam groove distance from front/back
DIAL_TEX = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_02\textures\T_Boiler02_Dial_Src.png"

# ---------------------------------------------------------------- helpers
def coll(name, parent=None):
    c = bpy.data.collections.get(name)
    if c is None:
        c = bpy.data.collections.new(name)
        (parent or bpy.context.scene.collection).children.link(c)
    return c

def purge(c):
    for o in list(c.objects):
        data = o.data
        bpy.data.objects.remove(o, do_unlink=True)
        if isinstance(data, bpy.types.Mesh) and data.users == 0:
            bpy.data.meshes.remove(data)

def T(x=0, y=0, z=0):
    return Matrix.Translation((x, y, z))

ROT = {
    "+Z": Matrix.Identity(4),
    "-Y": Matrix.Rotation(math.radians(90), 4, "X"),
    "+Y": Matrix.Rotation(math.radians(-90), 4, "X"),
    "+X": Matrix.Rotation(math.radians(90), 4, "Y"),
    "-X": Matrix.Rotation(math.radians(-90), 4, "Y"),
}
def M(pos, axis="+Z", spin=0.0):
    return T(*pos) @ ROT[axis] @ Matrix.Rotation(spin, 4, "Z")

def _ticks(h, rn, rp, rim, step, extra=()):
    t = []
    for i in range(rim, 0, -1):
        t.append(-h + rn - rn * math.tan(math.radians(45) * i / rim))
    a, b = -h + rn, h - rp
    n = max(1, round((b - a) / step))
    t += [a + (b - a) * i / n for i in range(n + 1)]
    for i in range(1, rim + 1):
        t.append(h - rp + rp * math.tan(math.radians(45) * i / rim))
    t += list(extra)
    return sorted(set(round(v, 6) for v in t))

def rbox(bm, h, rp, rn=None, rim=5, step=0.04, extra=((), (), ()), grooves=(), mat=None):
    """Rounded box (ellipsoidal corners, per-axis/per-side radii), quad grid."""
    rn = rn or rp
    mat = mat or Matrix.Identity(4)
    Tk = [_ticks(h[a], rn[a], rp[a], rim, step, extra[a]) for a in range(3)]
    vmap, faces = {}, []
    def V(cc):
        key = tuple(round(x, 6) for x in cc)
        v = vmap.get(key)
        if v is None:
            v = vmap[key] = bm.verts.new(cc)
        return v
    for k in range(3):
        i, j = [a for a in range(3) if a != k]
        for s in (-1, 1):
            for ii in range(len(Tk[i]) - 1):
                for jj in range(len(Tk[j]) - 1):
                    q = []
                    for di, dj in ((0, 0), (1, 0), (1, 1), (0, 1)):
                        cc = [0.0, 0.0, 0.0]
                        cc[k] = s * h[k]; cc[i] = Tk[i][ii + di]; cc[j] = Tk[j][jj + dj]
                        q.append(V(cc))
                    faces.append(bm.faces.new(q))
    for key, v in vmap.items():
        p = Vector(key)
        q, R = Vector(), Vector()
        for a in range(3):
            r = rp[a] if p[a] > 0 else rn[a]
            q[a] = min(max(p[a], -h[a] + rn[a]), h[a] - rp[a])
            R[a] = max(r, 1e-6)
        dd = Vector([(p[a] - q[a]) / R[a] for a in range(3)])
        if dd.length > 1e-9:
            n = dd.normalized()
            co = Vector([q[a] + n[a] * R[a] for a in range(3)])
            nn = Vector([n[a] / R[a] for a in range(3)]).normalized()
        else:
            co, nn = p.copy(), Vector()
        for axis, pos, depth in grooves:
            if abs(p[axis] - pos) < 1e-5:
                co -= nn * depth
        v.co = co
    # orient outward (shape is convex about local origin)
    for f in faces:
        if f.normal_update() or True:
            if f.calc_center_median().dot(f.normal) < 0:
                f.normal_flip()
    for v in vmap.values():
        v.co = mat @ v.co
    return faces

def lathe(bm, prof, segs=32, mat=None, closed=False, spin=0.0):
    """Revolve (r,z) profile around local Z. Profile must run CCW in the r-z half plane
    (outward along the bottom, up the outside, inward along the top) for outward normals."""
    mat = mat or Matrix.Identity(4)
    rings = []
    for r, z in prof:
        if r < 1e-7:
            rings.append([bm.verts.new(mat @ Vector((0, 0, z)))])
        else:
            rings.append([bm.verts.new(mat @ Vector((r * math.cos(spin + 2 * math.pi * i / segs),
                                                     r * math.sin(spin + 2 * math.pi * i / segs), z)))
                          for i in range(segs)])
    n = len(rings)
    faces = []
    for a in range(n if closed else n - 1):
        A, B = rings[a], rings[(a + 1) % n]
        if len(A) == 1 and len(B) == 1:
            continue
        for i in range(segs):
            i2 = (i + 1) % segs
            if len(A) == 1:
                f = [A[0], B[i2], B[i]]
            elif len(B) == 1:
                f = [A[i], A[i2], B[0]]
            else:
                f = [A[i], A[i2], B[i2], B[i]]
            faces.append(bm.faces.new(f))
    return faces

def torus(bm, R, r, segs=32, rsegs=10, mat=None):
    prof = [(R + r * math.cos(2 * math.pi * k / rsegs), r * math.sin(2 * math.pi * k / rsegs)) for k in range(rsegs)]
    return lathe(bm, prof, segs, mat, closed=True)

def prism(bm, pts2d, y0, y1, mat=None):
    """Extrude a CCW 2D polygon (x,z) between y0 and y1 (convex)."""
    mat = mat or Matrix.Identity(4)
    a = [bm.verts.new(mat @ Vector((x, y0, z))) for x, z in pts2d]
    b = [bm.verts.new(mat @ Vector((x, y1, z))) for x, z in pts2d]
    fs = [bm.faces.new(a), bm.faces.new(b)]
    n = len(pts2d)
    for i in range(n):
        fs.append(bm.faces.new([a[i], a[(i + 1) % n], b[(i + 1) % n], b[i]]))
    cen = sum((v.co for v in a + b), Vector()) / (2 * n)
    for f in fs:
        f.normal_update()
        if (f.calc_center_median() - cen).dot(f.normal) < 0:
            f.normal_flip()
    return fs

PARTS = None
def part(name, mat, build, sharp=35):
    bm = bmesh.new()
    build(bm)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    PARTS.objects.link(ob)
    me.materials.append(mat)
    for p in me.polygons:
        p.use_smooth = True
    me.set_sharp_from_angle(angle=math.radians(sharp))
    me.uv_layers.new(name="UV0")
    me.uv_layers.new(name="UV_Src")
    return ob

# ---------------------------------------------------------------- materials
def _nodes(m):
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial"); out.location = (900, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled"); bsdf.name = "BSDF"; bsdf.location = (600, 0)
    nt.links.new(bsdf.outputs[0], out.inputs[0])
    tc = nt.nodes.new("ShaderNodeTexCoord"); tc.location = (-1400, 0)
    return nt, bsdf, tc

def _noise(nt, tc, scale, detail=6, rough=0.55, loc=(0, 0), w=None):
    n = nt.nodes.new("ShaderNodeTexNoise"); n.location = loc
    n.inputs["Scale"].default_value = scale
    n.inputs["Detail"].default_value = detail
    n.inputs["Roughness"].default_value = rough
    if w is not None:
        n.noise_dimensions = "4D"; n.inputs["W"].default_value = w
    nt.links.new(tc.outputs["Object"], n.inputs["Vector"])
    return n

def _ramp(nt, src, stops, loc=(0, 0)):
    r = nt.nodes.new("ShaderNodeValToRGB"); r.location = loc
    el = r.color_ramp.elements
    el[0].position, el[0].color = stops[0][0], stops[0][1]
    el[1].position, el[1].color = stops[1][0], stops[1][1]
    for p, c in stops[2:]:
        e = el.new(p); e.color = c
    nt.links.new(src, r.inputs[0])
    return r

def _mix(nt, a, b, fac, loc=(0, 0), blend="MIX"):
    m = nt.nodes.new("ShaderNodeMix"); m.data_type = "RGBA"; m.blend_type = blend; m.location = loc
    for idx, val in ((6, a), (7, b), (0, fac)):
        sock = m.inputs[idx]
        if isinstance(val, (tuple, list)):
            sock.default_value = val if len(val) == 4 else (*val, 1.0)
        elif isinstance(val, (int, float)):
            sock.default_value = val if idx == 0 else (val, val, val, 1.0)
        else:
            nt.links.new(val, sock)
    return m.outputs[2]

def _math(nt, op, a, b=None, loc=(0, 0), clamp=True):
    m = nt.nodes.new("ShaderNodeMath"); m.operation = op; m.use_clamp = clamp; m.location = loc
    for sock, val in ((m.inputs[0], a), (m.inputs[1], b)):
        if val is None:
            continue
        if isinstance(val, (int, float)):
            sock.default_value = val
        else:
            nt.links.new(val, sock)
    return m.outputs[0]

def _bump(nt, bsdf, height, strength, dist=0.002, loc=(300, -500)):
    b = nt.nodes.new("ShaderNodeBump"); b.location = loc
    b.inputs["Strength"].default_value = strength
    b.inputs["Distance"].default_value = dist
    nt.links.new(height, b.inputs["Height"])
    nt.links.new(b.outputs[0], bsdf.inputs["Normal"])

def mat_paint(name, base=(0.40, 0.062, 0.04), seed=0.0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt, bsdf, tc = _nodes(m)
    var = _noise(nt, tc, 2.5, 4, 0.5, (-1100, 400), w=seed)
    col = _mix(nt, (base[0] * 0.8, base[1] * 0.75, base[2] * 0.75), (base[0] * 1.15, base[1] * 1.2, base[2] * 1.1),
               var.outputs["Fac"], (-700, 400))
    # chips: medium noise + fine breakup, stronger on convex edges
    mot = _noise(nt, tc, 5.0, 8, 0.6, (-1100, 800), w=seed + 4.1)
    motm = _ramp(nt, mot.outputs["Fac"], [(0.45, (0, 0, 0, 1)), (0.78, (1, 1, 1, 1))], (-850, 800))
    col = _mix(nt, col, (base[0] * 0.5, base[1] * 0.5, base[2] * 0.5),
               _math(nt, "MULTIPLY", motm.outputs[0], 0.55), (-500, 700))
    chipn = _noise(nt, tc, 16.0, 12, 0.68, (-1100, 0), w=seed + 1.3)
    geo = nt.nodes.new("ShaderNodeNewGeometry"); geo.location = (-1100, -300)
    edge = _ramp(nt, geo.outputs["Pointiness"], [(0.5, (0, 0, 0, 1)), (0.555, (1, 1, 1, 1))], (-850, -300))
    boost = _math(nt, "MULTIPLY", edge.outputs[0], 0.2, (-600, -300))
    chipv = _math(nt, "ADD", chipn.outputs["Fac"], boost, (-600, 0))
    zsep = nt.nodes.new("ShaderNodeSeparateXYZ"); zsep.location = (-1100, -550)
    nt.links.new(tc.outputs["Object"], zsep.inputs[0])
    low = nt.nodes.new("ShaderNodeMapRange"); low.location = (-850, -550)
    low.inputs[1].default_value, low.inputs[2].default_value = 0.08, 0.45
    low.inputs[3].default_value, low.inputs[4].default_value = 0.07, 0.0
    nt.links.new(zsep.outputs[2], low.inputs[0])
    chipv = _math(nt, "ADD", chipv, low.outputs[0], (-450, -100))
    spk = _noise(nt, tc, 90.0, 2, 0.5, (-600, -800), w=seed + 2.7)
    spkm = _ramp(nt, spk.outputs["Fac"], [(0.70, (0, 0, 0, 1)), (0.72, (1, 1, 1, 1))], (-300, -800))
    chip0 = _ramp(nt, chipv, [(0.628, (0, 0, 0, 1)), (0.642, (1, 1, 1, 1))], (-300, 0))
    chipmix = nt.nodes.new("ShaderNodeMix"); chipmix.data_type = "RGBA"; chipmix.blend_type = "LIGHTEN"
    chipmix.inputs[0].default_value = 1.0
    nt.links.new(chip0.outputs[0], chipmix.inputs[6]); nt.links.new(spkm.outputs[0], chipmix.inputs[7])
    class _C:  # tiny adapter so chip.outputs[0] keeps working below
        outputs = [chipmix.outputs[2]]
    chip = _C
    halo = _ramp(nt, chipv, [(0.585, (0, 0, 0, 1)), (0.628, (1, 1, 1, 1))], (-300, 250))
    rust = (0.20, 0.075, 0.03)
    col = _mix(nt, col, rust, _math(nt, "MULTIPLY", halo.outputs[0], 0.55), (-50, 300))
    ironn = _noise(nt, tc, 40.0, 8, 0.6, (-600, 600), w=seed + 7)
    iron = _mix(nt, (0.035, 0.03, 0.028), (0.14, 0.06, 0.025), ironn.outputs["Fac"], (-300, 600))
    col = _mix(nt, col, iron, chip.outputs[0], (150, 300))
    # grime near the floor
    col = _mix(nt, col, (0.05, 0.035, 0.025), _math(nt, "MULTIPLY", low.outputs[0], 5.0), (350, 300))
    nt.links.new(col, bsdf.inputs["Base Color"])
    rough = _math(nt, "ADD", _math(nt, "MULTIPLY", var.outputs["Fac"], 0.15), 0.40, (100, -150))
    rough = _mix(nt, rough, 0.78, chip.outputs[0], (300, -150))
    nt.links.new(rough, bsdf.inputs["Roughness"])
    nt.links.new(_math(nt, "MULTIPLY", chip.outputs[0], 0.45, (300, -300)), bsdf.inputs["Metallic"])
    hgt = _math(nt, "SUBTRACT", 1.0, chip.outputs[0], (100, -500))
    _bump(nt, bsdf, hgt, 0.35, 0.0015)
    return m

def mat_iron(name, seed=3.0, rustiness=0.4):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt, bsdf, tc = _nodes(m)
    n1 = _noise(nt, tc, 7.0, 10, 0.62, (-1100, 200), w=seed)
    n2 = _noise(nt, tc, 55.0, 6, 0.6, (-1100, -200), w=seed + 2)
    rm = _ramp(nt, n1.outputs["Fac"], [(0.52, (0, 0, 0, 1)), (0.7, (1, 1, 1, 1))], (-800, 200))
    rmask = _math(nt, "MULTIPLY", rm.outputs[0], rustiness, (-550, 200))
    base = _mix(nt, (0.036, 0.034, 0.033), (0.075, 0.07, 0.066), n2.outputs["Fac"], (-550, 0))
    col = _mix(nt, base, (0.13, 0.06, 0.03), rmask, (-250, 100))
    nt.links.new(col, bsdf.inputs["Base Color"])
    nt.links.new(_mix(nt, 0.55, 0.88, rmask, (-250, -150)), bsdf.inputs["Roughness"])
    nt.links.new(_mix(nt, 0.75, 0.25, rmask, (-250, -300)), bsdf.inputs["Metallic"])
    _bump(nt, bsdf, n2.outputs["Fac"], 0.25, 0.001)
    return m

def mat_metal(name, c1, c2, rough=(0.3, 0.5), seed=5.0, scale=9.0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt, bsdf, tc = _nodes(m)
    n1 = _noise(nt, tc, scale, 8, 0.6, (-1100, 0), w=seed)
    r = _ramp(nt, n1.outputs["Fac"], [(0.4, (0, 0, 0, 1)), (0.7, (1, 1, 1, 1))], (-800, 0))
    nt.links.new(_mix(nt, c1, c2, r.outputs[0], (-400, 100)), bsdf.inputs["Base Color"])
    nt.links.new(_mix(nt, rough[0], rough[1], r.outputs[0], (-400, -150)), bsdf.inputs["Roughness"])
    bsdf.inputs["Metallic"].default_value = 1.0
    return m

def mat_enamel(name, c, seed=9.0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt, bsdf, tc = _nodes(m)
    n1 = _noise(nt, tc, 20.0, 8, 0.6, (-1100, 0), w=seed)
    chip = _ramp(nt, n1.outputs["Fac"], [(0.70, (0, 0, 0, 1)), (0.72, (1, 1, 1, 1))], (-800, 0))
    col = _mix(nt, c, (c[0] * 0.8, c[1] * 0.8, c[2] * 0.8), n1.outputs["Fac"], (-500, 150))
    col = _mix(nt, col, (0.05, 0.045, 0.04), chip.outputs[0], (-250, 150))
    nt.links.new(col, bsdf.inputs["Base Color"])
    nt.links.new(_mix(nt, 0.28, 0.7, chip.outputs[0], (-250, -150)), bsdf.inputs["Roughness"])
    nt.links.new(_mix(nt, 0.0, 0.6, chip.outputs[0], (-250, -300)), bsdf.inputs["Metallic"])
    return m

def mat_dial(name):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt, bsdf, tc = _nodes(m)
    img = bpy.data.images.load(DIAL_TEX, check_existing=True)
    tex = nt.nodes.new("ShaderNodeTexImage"); tex.image = img; tex.location = (-400, 0)
    uv = nt.nodes.new("ShaderNodeUVMap"); uv.uv_map = "UV_Src"; uv.location = (-700, 0)
    nt.links.new(uv.outputs[0], tex.inputs[0])
    nt.links.new(tex.outputs[0], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.35
    bsdf.inputs["Metallic"].default_value = 0.0
    return m

# ---------------------------------------------------------------- build
def build():
    global PARTS
    root = coll("Boiler02_Source")
    PARTS = coll("Boiler02_Parts", root)
    purge(PARTS)

    P = mat_paint("SRC_Boiler02_Paint")
    I = mat_iron("SRC_Boiler02_Iron")
    ID = mat_iron("SRC_Boiler02_IronDoor", seed=11.0, rustiness=0.45)
    DS = mat_metal("SRC_Boiler02_DarkSteel", (0.09, 0.087, 0.085), (0.035, 0.033, 0.032), (0.35, 0.6), 8.0, 10.0)
    B = mat_metal("SRC_Boiler02_Brass", (0.62, 0.42, 0.15), (0.28, 0.18, 0.07), (0.28, 0.55), 5.0, 12.0)
    S = mat_metal("SRC_Boiler02_Steel", (0.30, 0.29, 0.28), (0.10, 0.09, 0.085), (0.32, 0.6), 6.0, 10.0)
    R = mat_enamel("SRC_Boiler02_RedEnamel", (0.52, 0.02, 0.018))
    BL = mat_enamel("SRC_Boiler02_BlueEnamel", (0.02, 0.075, 0.42), seed=13.0)
    DL = mat_dial("SRC_Boiler02_Dial")

    # --- shell with front/back seam grooves
    gy = HY - SEAM
    part("Body", P, lambda bm: rbox(
        bm, (HX, HY, HZ), rp=(0.085, 0.065, 0.10), rn=(0.085, 0.065, 0.06), rim=7, step=0.1,
        extra=((), (-gy - 0.004, -gy, -gy + 0.004, gy - 0.004, gy, gy + 0.004), ()),
        grooves=((1, -gy, 0.005), (1, gy, 0.005)), mat=T(0, 0, ZC)))

    # --- base plate + legs
    part("BasePlate", I, lambda bm: rbox(bm, (0.27, 0.172, 0.007), rp=(0.035, 0.035, 0.004), rim=3, step=0.1,
                                         mat=T(0, 0, Z0 - 0.002)))
    def legs(bm):
        prof = [(0, 0.0), (0.033, 0.0), (0.0355, 0.003), (0.0355, 0.012), (0.031, 0.017), (0.026, 0.062),
                (0.028, 0.068), (0.043, 0.072), (0.046, 0.076), (0.046, 0.084)]
        for sx in (-1, 1):
            for sy in (-1, 1):
                lathe(bm, prof, 24, T(sx * 0.25, sy * 0.14, 0))
    part("Legs", I, legs)

    # --- chimney
    def chimney(bm):
        prof = [(0.105, -0.004), (0.107, 0.006), (0.104, 0.012), (0.094, 0.013), (0.094, 0.028), (0.0885, 0.033),
                (0.0875, 0.036), (0.0875, 0.128), (0.091, 0.131), (0.092, 0.139), (0.089, 0.143), (0.080, 0.143),
                (0.078, 0.139), (0.078, 0.07), (0, 0.07)]
        lathe(bm, prof, 40, T(0, CH_Y, Z1))
        bolt = [(0.0075, 0.0), (0.0075, 0.004), (0.005, 0.0065), (0, 0.007)]
        for k in range(8):
            a = 2 * math.pi * k / 8 + math.pi / 8
            lathe(bm, bolt, 6, T(0.0995 * math.cos(a), CH_Y + 0.0995 * math.sin(a), Z1 + 0.011))
    part("Chimney", I, chimney)

    # --- back service panel + corner bolts
    part("BackPanel", P, lambda bm: rbox(bm, (0.255, 0.005, 0.42), rp=(0.03, 0.003, 0.03), rim=4, step=0.08,
                                         mat=T(0, HY + 0.001, 0.64)))
    part("BackPanelGasket", I, lambda bm: rbox(bm, (0.263, 0.004, 0.428), rp=(0.035, 0.002, 0.035), rim=4,
                                               step=0.2, mat=T(0, HY - 0.0005, 0.64)))
    def back_bolts(bm):
        prof = [(0.0115, -0.002), (0.0115, 0.002), (0.009, 0.0055), (0.005, 0.0072), (0, 0.0075)]
        for sx in (-1, 1):
            for sz in (-1, 1):
                lathe(bm, prof, 16, M((sx * 0.222, HY + 0.005, 0.64 + sz * 0.385), "+Y"))
    part("BackBolts", S, back_bolts)

    # --- pressure gauge
    GZ = 0.977
    part("GaugeBezel", DS, lambda bm: lathe(bm, [
        (0.128, -0.005), (0.128, 0.006), (0.124, 0.010), (0.1205, 0.011), (0.1205, 0.026), (0.117, 0.032),
        (0.110, 0.0335), (0.105, 0.030), (0.104, 0.013), (0, 0.013)], 48, M((0, FY, GZ), "-Y")))
    dial = part("GaugeDial", DL, lambda bm: lathe(bm, [(0.1045, 0.014), (0.1045, 0.017), (0, 0.017)], 48,
                                                  M((0, FY, GZ), "-Y")))
    part("GaugeHub", S, lambda bm: lathe(bm, [(0.0095, 0.0165), (0.0095, 0.0195), (0.007, 0.0225), (0, 0.0235)],
                                         16, M((0, FY, GZ), "-Y")))
    uvl = dial.data.uv_layers["UV_Src"].data
    for poly in dial.data.polygons:
        for li in poly.loop_indices:
            co = dial.data.vertices[dial.data.loops[li].vertex_index].co
            uvl[li].uv = (0.5 + co.x / 0.209, 0.5 + (co.z - GZ) / 0.209)

    # --- name plate
    def plate(bm):
        rbox(bm, (0.08, 0.004, 0.025), rp=(0.006, 0.0025, 0.006), rim=3, step=0.2, mat=T(0, FY - 0.002, 0.74))
        for sx in (-1, 1):
            lathe(bm, [(0.004, 0.0), (0.004, 0.001), (0.0025, 0.0025), (0, 0.003)], 8,
                  M((sx * 0.068, FY - 0.006, 0.74), "-Y"))
    part("NamePlate", I, plate)

    # --- firebox door
    DZ = 0.50
    part("DoorFrame", I, lambda bm: rbox(bm, (0.22, 0.012, 0.178), rp=(0.022, 0.008, 0.022), rim=4, step=0.06,
                                         mat=T(0, FY - 0.008, DZ)))
    door = part("DoorPlate", ID, lambda bm: rbox(bm, (0.185, 0.0105, 0.147), rp=(0.014, 0.006, 0.014), rim=4,
                                                 step=0.03, mat=T(0.004, FY - 0.028, DZ)))
    # vent slots via boolean
    cut = part("_VentCutter", I, lambda bm: [rbox(bm, (0.0095, 0.02, 0.036), rp=(0.009, 0.002, 0.009), rim=4,
                                                  step=0.2, mat=T(0.025 + (k - 1.5) * 0.043, FY - 0.047, 0.41))
                                             for k in range(4)])
    mod = door.modifiers.new("Vents", "BOOLEAN")
    mod.operation, mod.solver, mod.object = "DIFFERENCE", "EXACT", cut
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    new = bpy.data.meshes.new_from_object(door.evaluated_get(dg))
    old = door.data
    door.modifiers.clear()
    door.data = new
    bpy.data.meshes.remove(old)
    new.name = "DoorPlate"
    me_c = cut.data
    bpy.data.objects.remove(cut, do_unlink=True)
    bpy.data.meshes.remove(me_c)
    new.set_sharp_from_angle(angle=math.radians(35))
    if "UV_Src" not in new.uv_layers:
        new.uv_layers.new(name="UV_Src")

    def hinges(bm):
        for sz in (-1, 1):
            z = DZ + sz * 0.09
            lathe(bm, [(0, -0.03), (0.011, -0.03), (0.0125, -0.027), (0.0125, 0.027), (0.011, 0.03), (0, 0.03)],
                  16, T(-0.2, FY - 0.03, z))
            lathe(bm, [(0.0045, 0.03), (0.0045, 0.036), (0.003, 0.038), (0, 0.038)], 8, T(-0.2, FY - 0.03, z))
            rbox(bm, (0.028, 0.003, 0.022), rp=(0.003, 0.0015, 0.003), rim=2, step=0.2, mat=T(-0.175, FY - 0.0405, z))
            rbox(bm, (0.012, 0.0045, 0.022), rp=(0.003, 0.002, 0.003), rim=2, step=0.2, mat=T(-0.212, FY - 0.017, z))
    part("DoorHinges", I, hinges)

    def handle(bm):
        rbox(bm, (0.013, 0.003, 0.034), rp=(0.004, 0.0015, 0.004), rim=2, step=0.2, mat=T(0.155, FY - 0.041, DZ + 0.01))
        for sz in (-1, 1):
            lathe(bm, [(0.0055, 0.0), (0.0055, 0.012), (0, 0.012)], 10, M((0.155, FY - 0.043, DZ + 0.01 + sz * 0.024), "-Y"))
        rbox(bm, (0.0075, 0.0065, 0.04), rp=(0.0055, 0.004, 0.0065), rim=3, step=0.2, mat=T(0.155, FY - 0.058, DZ + 0.01))
        rbox(bm, (0.012, 0.009, 0.014), rp=(0.003, 0.003, 0.003), rim=2, step=0.2, mat=T(0.2, FY - 0.022, DZ + 0.01))
    part("DoorHandle", I, handle)

    # --- burner knob + marker
    KZ = 0.223
    part("KnobBase", DS, lambda bm: lathe(bm, [(0.047, -0.004), (0.047, 0.004), (0.044, 0.008), (0, 0.008)], 32,
                                         M((0, FY, KZ), "-Y")))
    def knob(bm):
        lathe(bm, [(0.037, 0.006), (0.0375, 0.025), (0.035, 0.032), (0.029, 0.0355), (0, 0.0355)], 32,
              M((0, FY, KZ), "-Y"))
        rbox(bm, (0.0065, 0.006, 0.03), rp=(0.004, 0.004, 0.005), rim=3, step=0.2, mat=T(0, FY - 0.038, KZ))
    part("Knob", R, knob)
    part("KnobMarker", I, lambda bm: prism(bm, [(-0.009, 0.297), (0, 0.282), (0.009, 0.297)], FY + 0.001, FY - 0.0025))

    # --- valves on the right side (+X)
    VY = 0.03
    for label, z, wmat in (("Top", 0.988, R), ("Bottom", 0.684, BL)):
        mv = M((HX, VY, z), "+X")
        def brass(bm, mv=mv):
            lathe(bm, [(0.03, -0.005), (0.03, 0.014), (0.027, 0.018), (0, 0.018)], 6, mv, spin=math.pi / 6)
            lathe(bm, [(0.016, 0.017), (0.016, 0.04), (0, 0.04)], 16, mv)
            lathe(bm, [(0.026, 0.038), (0.026, 0.055), (0, 0.055)], 6, mv, spin=math.pi / 6)
            lathe(bm, [(0.02, 0.054), (0.027, 0.061), (0.028, 0.08), (0.021, 0.089), (0, 0.089)], 20, mv)
            lathe(bm, [(0.016, 0.087), (0.016, 0.099), (0, 0.099)], 6, mv, spin=math.pi / 6)
            lathe(bm, [(0.0055, 0.098), (0.0055, 0.112), (0, 0.112)], 10, mv)
        part("Valve%s_Brass" % label, B, brass)
        def wheel(bm, mv=mv):
            torus(bm, 0.0355, 0.0065, 32, 10, mv @ T(0, 0, 0.115))
            lathe(bm, [(0.0115, 0.106), (0.0115, 0.12), (0.0085, 0.1235), (0, 0.124)], 16, mv)
            for k in range(5):
                a = 2 * math.pi * k / 5 + math.pi / 2
                spoke = mv @ T(0, 0, 0.115) @ Matrix.Rotation(a, 4, "Z") @ Matrix.Rotation(math.radians(90), 4, "Y")
                lathe(bm, [(0, 0.008), (0.0042, 0.008), (0.0042, 0.033), (0, 0.033)], 8, spoke)
        part("Valve%s_Wheel" % label, wmat, wheel)

    return {o.name: len(o.data.polygons) for o in PARTS.objects}

if __name__ == "__main__" or True:
    result = build()
