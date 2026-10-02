"""Circulator_01 - bathwater circulation pump for BP_Circulator (retro enamel + cast iron).

Coordinates: BP_Circulator SceneRoot-local Unreal cm (front = UE -Y, bottom pivot). Footprint X+-60, Y+-40, Z0-120.
Export groups:
  Body   -> SM_Circulator_01         (VisualMesh: skid, control cabinet, gauge housing, lever quadrant, motor, pump, pipes)
  Lever  -> SM_Circulator_01_Lever   (LeverMesh, pivot = LeverPivot (30,-44,30), arm points +Z at rest;
                                      LeverLabor swings it about local Y to -60 deg = toward UE -X / viewer's right)
  Needle -> SM_Circulator_01_Needle  (GaugeNeedleMesh, pivot = GaugeNeedlePivot (34.49,-44,109.92), points +X at rest)
Viewer stands at UE -Y; UE +X is on the viewer's LEFT.
"""
import os
exec(open(r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\_Pipeline\bh_lib.py", encoding="utf-8").read())
apply_asset_palette("Circulator_01")          # theme + overrides: _Pipeline/palettes.json

ROOT = globals().get("ROOT_OVERRIDE", r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Circulator_01")
TEXSRC = os.path.join(ROOT, "textures", "src")
os.makedirs(TEXSRC, exist_ok=True)

reset_scene()
begin_parts("Circulator_01")
M = standard_mats("CIR")
glass_g = mat_plastic("CIR_LampGreen", PAL["lamp_a"], rough=0.15, edge_light=0.5)
glass_r = mat_plastic("CIR_LampAmber", PAL["lamp_b"], rough=0.15, edge_light=0.5)
dial_png = make_dial(os.path.join(TEXSRC, "T_Circulator_01_Dial_Src.png"), labels=("0", "20", "40"),
                     unit="L/min", red_from=0.85, sweep=(150.0, 30.0), title="FLOW")
plate_png = make_label(os.path.join(TEXSRC, "T_Circulator_01_Plate_Src.png"), ["CIRCULATOR"],
                       size=(1024, 256), sizes=[0.62])
dial_m = mat_image("CIR_Dial", dial_png, rough=0.35)
plate_m = mat_image("CIR_Plate", plate_png, rough=0.4)

FY = -40.0                       # cabinet front face
LP = (30.0, -44.0, 30.0)         # LeverPivot
GP = (34.49, -44.0, 109.92)      # GaugeNeedlePivot

# ---------------------------------------------------------------- skid base
box("Skid", M["iron"], (0, -1, 5), (120, 76, 6), bevel=0.012, segs=2)
for sx in (-1, 1):
    for sy in (-1, 1):
        lathe("Foot%d%d" % (sx, sy), M["iron"], (sx * 53, sy * 31, 0),
              [(4.2, 0), (4.2, 1.2), (3.4, 2.0), (3.2, 2.2), (0, 2.2)], verts=20)
for i, x in enumerate((-50, -30, -10, 10, 30, 50)):
    for sy in (-1, 1):
        lathe("SkidBolt%d%d" % (i, sy), M["steel"], (x, sy * 34, 8), [(1.1, 0), (1.1, 0.5), (0.7, 0.9), (0, 1.0)],
              verts=6)
# ---------------------------------------------------------------- control cabinet (viewer's left, +X)
CX0, CX1 = 12.0, 58.0
box("Cabinet", M["body"], ((CX0 + CX1) / 2, -2, 64), (CX1 - CX0, 76, 112), bevel=0.035, segs=4)
box("CabinetBand", M["chrome"], ((CX0 + CX1) / 2, FY - 0.35, 86), (CX1 - CX0 - 7, 1.4, 2.0), bevel=0.004, segs=2)
box("CabinetKick", M["light"], ((CX0 + CX1) / 2, FY - 0.3, 13.5), (CX1 - CX0 - 6, 1.2, 7), bevel=0.004, segs=2)
# side vents (+X face)
for i in range(4):
    box("SideVent%d" % i, M["body_dk"], (CX1 + 0.3, 0, 50 + i * 5), (1.0, 28, 2.0), bevel=0.006, segs=2)
box("SideVentFrame", M["chrome"], (CX1 + 0.1, 0, 57.5), (0.8, 32, 23), bevel=0.004, segs=2)
# nameplate + indicator lamps
box("PlateFrame", M["brass"], (35, FY - 0.5, 77), (32, 1.0, 7.5), bevel=0.005, segs=2)
pm = box("PlateFace", plate_m, (35, FY - 1.05, 77), (29.6, 0.3, 5.8), bevel=0.0)
plate_src_uv((35, FY - 1.2, 77), 29.6, 5.8, "-Y")(pm.data)
for x, gm in ((47.0, glass_g), (23.0, glass_r)):
    lathe("LampBezel%d" % x, M["chrome"], (x, FY, 94), [(2.6, 0), (2.6, 0.6), (2.1, 1.0), (0, 1.0)], axis="-Y",
          verts=24)
    lathe("LampLens%d" % x, gm, (x, FY - 1.0, 94), [(1.9, 0), (1.8, 0.6), (1.2, 1.2), (0, 1.4)], axis="-Y", verts=24)
# gauge housing on the cabinet top front
lathe("GaugeBack", M["body_dk"], (GP[0], FY, GP[2]), [(10.0, 0), (10.0, 0.6), (9.4, 1.0), (0, 1.0)], axis="-Y",
      verts=48)
lathe("GaugeBezel", M["chrome"], (GP[0], FY - 1.0, GP[2]),
      [(9.4, 0), (9.4, 1.0), (9.1, 1.9), (8.6, 2.3), (8.1, 2.2), (7.9, 1.6), (7.9, 1.4), (0, 1.4)],
      axis="-Y", verts=64)
disc("GaugeDial", dial_m, (GP[0], -42.45, GP[2]), 7.9, 0.1, "-Y", verts=64, src_uv=True)
# lever quadrant: arc guide from straight up (0 deg) to 60 deg toward viewer's right (UE -X)
qy = FY - 0.6
pts = []
for k in range(13):
    a = math.radians(90 + 70 * k / 12 - 72)          # screen angle 18..88 deg (viewer's right = UE -X)
    pts.append((LP[0] - 25.5 * math.cos(a), LP[2] + 25.5 * math.sin(a)))
for k in range(12, -1, -1):
    a = math.radians(90 + 70 * k / 12 - 72)
    pts.append((LP[0] - 21.0 * math.cos(a), LP[2] + 21.0 * math.sin(a)))
# build quadrant as strips (non-convex polygon -> segment boxes)
for k in range(12):
    a0 = math.radians(26 + 62 * k / 12); a1 = math.radians(26 + 62 * (k + 1) / 12)
    am = (a0 + a1) / 2
    cx, cz = LP[0] - 18.0 * math.cos(am), LP[2] + 18.0 * math.sin(am)
    box("Quadrant%d" % k, M["chrome"], (cx, qy, cz), (2 * 18.0 * math.sin((a1 - a0) / 2) + 0.3, 1.2, 3.6),
        bevel=0.0, rot=[("Y", -(90 - math.degrees(am)))])
for k, ang in enumerate((90, 60, 30)):
    a = math.radians(ang)
    box("QuadNotch%d" % k, M["iron"], (LP[0] - 18.0 * math.cos(a), qy - 0.2, LP[2] + 18.0 * math.sin(a)),
        (1.0, 1.4, 4.4), bevel=0.002, rot=[("Y", -(90 - ang))])
lathe("LeverBoss", M["iron"], (LP[0], FY, LP[2]), [(4.6, 0), (4.6, 0.8), (3.6, 1.4), (3.6, 2.6), (0, 2.6)],
      axis="-Y", verts=28)
# ---------------------------------------------------------------- pump train (viewer's right, -X)
MZ = 36.0
lathe("MotorBody", M["body_dk"], (-42, 2, MZ), [(0, -14.5), (15.5, -14.5), (17.0, -13.0), (17.0, 13.0), (15.5, 14.5),
                                                (0, 14.5)], axis="+X", verts=40)
for i in range(6):
    torus("MotorFin%d" % i, M["body_dk"], (-52 + i * 4.0, 2, MZ), 17.0, 0.9, axis="+X", verts=40, rverts=8)
lathe("MotorEndCap", M["iron"], (-56.5, 2, MZ), [(0, 0), (14.0, 0), (14.0, 1.4), (9.0, 2.6), (0, 3.0)],
      axis="-X", verts=36)
lathe("MotorFrontCap", M["iron"], (-27.5, 2, MZ), [(0, 0), (14.0, 0), (14.0, 1.0), (10.0, 2.0), (0, 2.0)],
      axis="+X", verts=36)
box("TerminalBox", M["body"], (-42, 2, MZ + 19.5), (14, 12, 7), bevel=0.012, segs=3)
box("TerminalLid", M["chrome"], (-42, 2, MZ + 23.4), (12, 10, 1.0), bevel=0.004, segs=2)
tube("Conduit", M["iron"], [(-42, 7, MZ + 21), (-42, 24, MZ + 21), (-42, 24, 8.5)], 1.1, verts=10, bend=6)
for sy in (-1, 1):
    box("MotorFoot%d" % sy, M["iron"], (-42, 2 + sy * 10, 13.5), (26, 4, 11), bevel=0.006, segs=2)
cyl("Shaft", M["steel"], (-24.5, 2, MZ), 2.6, 5.5, axis="+X", verts=20, bevel=0.002)
lathe("Coupling", M["accent"], (-22.5, 2, MZ), [(0, 0), (4.2, 0), (4.6, 0.6), (4.6, 2.8), (4.2, 3.4), (0, 3.4)],
      axis="+X", verts=24)
# volute (pump casing), axis X, outlet up
VX = -12.0
lathe("Volute", M["iron"], (VX - 6.0, 2, MZ), [(0, 0), (16.0, 0), (19.5, 1.6), (21.0, 6.0), (19.5, 10.4), (16.0, 12.0),
                                             (0, 12.0)], axis="+X", verts=44)
for k in range(8):
    a = math.radians(22.5 + 45 * k)
    lathe("VoluteBolt%d" % k, M["steel"], (VX + 6.0, 2 + 17.0 * math.cos(a), MZ + 17.0 * math.sin(a)),
          [(1.0, 0), (1.0, 0.5), (0.6, 0.9), (0, 1.0)], axis="+X", verts=6)
box("VoluteOutlet", M["iron"], (VX, -6, MZ + 21), (11, 11, 8), bevel=0.012, segs=2)
box("PumpFoot", M["iron"], (VX, 2, 13.5), (11, 22, 11), bevel=0.008, segs=2)
# suction: axial inlet -> back to wall (stays behind the lever swing plane)
lathe("InletFlange", M["iron"], (VX + 6.0, 2, MZ), [(0, 0), (8.0, 0), (8.0, 1.6), (0, 1.6)], axis="+X", verts=28)
tube("SuctionPipe", M["copper"], [(VX + 7.0, 2, MZ), (2.0, 2, MZ), (2.0, 36.5, MZ)], 5.0, verts=24, bend=7)
lathe("SuctionWallFlange", M["iron"], (2.0, 36.0, MZ), [(0, 0), (8.0, 0), (8.0, 2.0), (0, 2.0)], axis="+Y", verts=28)
# discharge: up from volute outlet, gate valve, bend back to wall
CU = M["copper"]
tube("DischargePipe", CU, [(VX, -6, MZ + 25), (VX, -6, 98), (VX, 36.5, 98)], 4.6, verts=24, bend=9)
lathe("DischargeWallFlange", M["iron"], (VX, 36.0, 98), [(0, 0), (7.6, 0), (7.6, 2.0), (0, 2.0)], axis="+Y", verts=28)
lathe("GateValveBody", M["brass"], (VX, -6, 74), [(0, -6), (6.4, -6), (7.2, -4.6), (7.2, 4.6), (6.4, 6), (0, 6)],
      axis="+Z", verts=24)
box("GateBonnet", M["brass"], (VX, -10.5, 74), (6, 6, 8), bevel=0.01, segs=2)
cyl("GateStem", M["steel"], (VX, -15.5, 74), 0.7, 6, axis="-Y", verts=10, bevel=0.0)
torus("GateWheel", M["accent"], (VX, -18.0, 74), 5.2, 0.8, axis="-Y", verts=32)
for k in range(3):
    box("GateSpoke%d" % k, M["accent"], (VX, -18.0, 74), (10.4, 0.9, 0.9), bevel=0.0, rot=[("Y", 60 * k)])
lathe("GateHub", M["accent"], (VX, -18.0, 74), [(0, -0.9), (1.6, -0.9), (1.6, 0.9), (0, 0.9)], axis="-Y", verts=16)
for z in (MZ + 29, 90.5):
    lathe("PipeCollar%d" % z, M["iron"], (VX, -6, z), [(0, -1.2), (6.2, -1.2), (6.2, 1.2), (0, 1.2)], verts=24)

# ================================================================= Lever (pivot = LeverPivot, points +Z)
L = "Lever"
lathe("LeverHub", M["iron"], (LP[0], FY - 2.6, LP[2]), [(3.4, 0), (3.4, 2.8), (2.8, 3.4), (0, 3.4)], axis="-Y",
      group=L, verts=24)
box("LeverArm", M["iron"], (LP[0], LP[1], LP[2] + 17.0), (2.6, 1.8, 30.0), group=L, bevel=0.006, segs=2,
    taper=(0.75, 0.9))
box("LeverLatchRod", M["steel"], (LP[0] - 1.8, LP[1] - 0.4, LP[2] + 19.0), (0.7, 0.7, 22.0), group=L, bevel=0.0)
box("LeverLatchGrip", M["steel"], (LP[0] - 1.6, LP[1] - 0.4, LP[2] + 30.5), (1.8, 1.4, 3.2), group=L, bevel=0.003)
lathe("LeverKnob", M["accent"], (LP[0], LP[1], LP[2] + 31.0),
      [(0.01, 0), (1.6, 0.2), (3.1, 1.6), (3.5, 3.6), (3.1, 5.6), (1.8, 6.8), (0, 7.1)], group=L, verts=24)

# ================================================================= Needle (pivot = GaugeNeedlePivot, points +X)
N = "Needle"
GX, GZ = GP[0], GP[2]
cyl("NeedleHub", M["iron"], (GX, -43.2, GZ), 1.1, 1.7, axis="-Y", group=N, verts=18, bevel=0.0015)
prism("NeedleBlade", M["accent"], [(GX - 2.4, GZ - 0.7), (GX, GZ - 0.55), (GX + 7.2, GZ - 0.14), (GX + 7.6, GZ),
                               (GX + 7.2, GZ + 0.14), (GX, GZ + 0.55), (GX - 2.4, GZ + 0.7)],
      "XZ", -43.8, -44.2, group=N, bevel=0.0)
lathe("NeedleCap", M["chrome"], (GX, -44.2, GZ), [(0.75, 0), (0.7, 0.3), (0.4, 0.65), (0, 0.75)], axis="-Y",
      group=N, verts=14)

aged = age_all_materials()      # ageing strength: palettes.json age_default / assets.<asset>.age
result = {"parts": len(PARTS["coll"].objects), "aged_materials": aged}
