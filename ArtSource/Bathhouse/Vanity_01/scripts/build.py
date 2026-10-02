"""Vanity_01 - retro dressing table with arched mirror for BP_Vanity.

Coordinates: BP_Vanity SceneRoot-local Unreal cm. Front = UE +X (customer at (70,0,0) facing -X).
Footprint X+-40, Y+-70, Z0-180. Counter top surface Z=100 (display slots: X -9..9, Y -58..58, Z 100..114).
BP MirrorVisual plane sits at X=-25 (normal -X); the modelled glass is at X=-24.6 and covers it.
Single export mesh: SM_Vanity_01 (VanityBody).
"""
import os
exec(open(r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\_Pipeline\bh_lib.py", encoding="utf-8").read())
apply_asset_palette("Vanity_01")          # theme + overrides: _Pipeline/palettes.json

ROOT = globals().get("ROOT_OVERRIDE", r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Vanity_01")
reset_scene()
begin_parts("Vanity_01")
M = standard_mats("VAN")
mirror = mat_mirror("VAN_Mirror")
shade = mat_plastic("VAN_Shade", PAL["shade"], rough=0.3, edge_light=0.2)

TOP = 100.0
# ---------------------------------------------------------------- legs (tapered, brass shoes)
for sx in (-1, 1):
    for sy in (-1, 1):
        lathe("Leg%d%d" % (sx, sy), M["wood"], (sx * 21 - 1, sy * 51, 0),
              [(1.6, 2.4), (2.2, 10.5), (0, 10.5)], verts=16)
        lathe("LegShoe%d%d" % (sx, sy), M["brass"], (sx * 21 - 1, sy * 51, 0),
              [(1.9, 0), (1.9, 2.6), (1.5, 2.8), (0, 2.8)], verts=16)
# ---------------------------------------------------------------- cabinet
box("Cabinet", M["body"], (-1.5, 0, 53), (57, 116, 86), bevel=0.025, segs=4)
box("CabinetPlinthTrim", M["chrome"], (27.3, 0, 11.2), (0.9, 108, 1.2), bevel=0.003)
# front: two doors (sides) + two drawers (centre); panels in darker teal with cream inner field
FX = 27.0                                    # cabinet front face x
for sy in (-1, 1):
    yc = sy * 39.0
    box("Door%d" % sy, M["body_dk"], (FX + 0.6, yc, 52), (1.4, 36, 76), bevel=0.012, segs=3)
    box("DoorField%d" % sy, M["light"], (FX + 1.3, yc, 52), (0.6, 28, 66), bevel=0.006, segs=2)
    box("DoorPull%d" % sy, M["chrome"], (FX + 3.4, sy * 23.5, 66), (1.4, 1.6, 14), bevel=0.006, segs=2)
    for dz in (-1, 1):
        box("DoorPullPost%d%d" % (sy, dz), M["chrome"], (FX + 2.3, sy * 23.5, 66 + dz * 5.6), (2.0, 1.2, 1.2),
            bevel=0.002)
for i, (zc, h) in enumerate(((73.0, 30.0), (36.0, 40.0))):
    box("Drawer%d" % i, M["body_dk"], (FX + 0.6, 0, zc), (1.4, 38, h), bevel=0.012, segs=3)
    box("DrawerField%d" % i, M["light"], (FX + 1.3, 0, zc), (0.6, 31, h - 8), bevel=0.006, segs=2)
    box("DrawerPull%d" % i, M["chrome"], (FX + 3.4, 0, zc + h / 2 - 8), (1.4, 14, 1.6), bevel=0.006, segs=2)
    for dy in (-1, 1):
        box("DrawerPullPost%d%d" % (i, dy), M["chrome"], (FX + 2.3, dy * 5.6, zc + h / 2 - 8), (2.0, 1.2, 1.2),
            bevel=0.002)
    lathe("Keyhole%d" % i, M["brass"], (FX + 1.6, 0, zc - 2), [(1.1, 0), (1.1, 0.25), (0, 0.3)], axis="+X", verts=16)
# side towel bar (-Y side)
tube("TowelBar", M["chrome"], [(16, -58, 80), (16, -64, 80), (-14, -64, 80), (-14, -58, 80)], 0.9, verts=12,
     bend=2.5)
# ---------------------------------------------------------------- counter (top surface Z=100)
box("Counter", M["light"], (0, 0, TOP - 2.5), (62, 120, 5), bevel=0.01, segs=3)
box("CounterEdgeF", M["chrome"], (31.15, 0, TOP - 2.6), (0.9, 118, 2.6), bevel=0.003, segs=2)
for sy in (-1, 1):
    box("CounterEdgeS%d" % sy, M["chrome"], (1.0, sy * 60.15, TOP - 2.6), (58, 0.9, 2.6), bevel=0.003, segs=2)
# ---------------------------------------------------------------- backsplash + arched mirror
box("Backsplash", M["body_dk"], (-27.5, 0, TOP + 5.0), (5, 118, 10), bevel=0.01, segs=3)
def arch(y_half, z0, z1, r, n=10):
    """Rounded-top rectangle outline (UE y,z), CCW seen from +X."""
    pts = [(-y_half, z0), (y_half, z0)]
    for k in range(n + 1):
        a = math.radians(0 + 90 * k / n)
        pts.append((y_half - r + r * math.cos(a), z1 - r + r * math.sin(a)))
    for k in range(n + 1):
        a = math.radians(90 + 90 * k / n)
        pts.append((-y_half + r + r * math.cos(a), z1 - r + r * math.sin(a)))
    return pts
prism("MirrorBoard", M["body_dk"], arch(56.0, 104.0, 179.0, 22.0), "YZ", -29.5, -25.6, bevel=0.006)
prism("MirrorGlass", mirror, arch(51.0, 108.0, 174.5, 18.0), "YZ", -25.6, -24.6, bevel=0.0)
out = arch(53.5, 106.0, 176.8, 20.0)
loop = out[1:] + [out[0], out[1]]
tube("MirrorFrame", M["light"], [(-24.4, y, z) for y, z in loop], 1.6, verts=16, sharp=70)
# little crest on top
lathe("Crest", M["brass"], (-25.0, 0, 176.8), [(3.0, 0), (3.0, 0.8), (2.2, 1.4), (0, 1.6)], axis="+X", verts=24)
# ---------------------------------------------------------------- wall sconces at the mirror sides
for sy in (-1, 1):
    yc = sy * 62.0
    lathe("SconcePlate%d" % sy, M["chrome"], (-28.0, yc, 148), [(3.2, 0), (3.2, 0.6), (2.6, 1.2), (0, 1.2)],
          axis="+X", verts=24)
    tube("SconceArm%d" % sy, M["chrome"], [(-27.0, yc, 148), (-21.0, yc, 148), (-19.0, yc, 152)], 0.7, verts=10,
         bend=2.0)
    lathe("SconceCup%d" % sy, M["chrome"], (-19.0, yc, 151.5), [(0, 0), (2.0, 0), (2.4, 1.2), (0, 1.2)], verts=20)
    lathe("SconceShade%d" % sy, shade, (-19.0, yc, 152.5),
          [(0.01, 0), (2.4, 0.2), (4.6, 2.6), (5.0, 5.4), (4.2, 8.2), (2.8, 9.4), (0, 9.6)], verts=28)
    # backing strip connecting sconce to mirror board
    box("SconceMount%d" % sy, M["body_dk"], (-28.6, sy * 58.0, 148), (1.6, 10, 9), bevel=0.006, segs=2)

aged = age_all_materials()      # ageing strength: palettes.json age_default / assets.<asset>.age
result = {"parts": len(PARTS["coll"].objects), "aged_materials": aged}
