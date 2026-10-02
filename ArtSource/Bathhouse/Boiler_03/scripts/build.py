"""Boiler_03 - functional bathhouse boiler for BP_Boiler (retro enamel stove style).

All coordinates are BP_Boiler SceneRoot-local Unreal cm (front = UE -Y, bottom pivot).
Export groups:
  Body   -> SM_Boiler_03          (VisualMesh: shell, firebox frame, gauge housing, nameplate)
  Door   -> SM_Boiler_03_Door     (FuelDoorMesh, pivot = hinge axis (12.5,-37,42), plate extends -X)
  Needle -> SM_Boiler_03_Needle   (GaugeNeedleMesh, pivot = hub (0,-34,90), needle points +X at rest)
Gauge arc follows GaugePresentation: Zero -30 deg (10 o'clock) .. Max -150 deg (2 o'clock) about local Y.
"""
import os
exec(open(r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\_Pipeline\bh_lib.py", encoding="utf-8").read())
apply_asset_palette("Boiler_03")          # theme + overrides: _Pipeline/palettes.json

ROOT = globals().get("ROOT_OVERRIDE", r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_03")
TEXSRC = os.path.join(ROOT, "textures", "src")
os.makedirs(TEXSRC, exist_ok=True)

reset_scene()
begin_parts("Boiler_03")
M = standard_mats("BLR")
soot = mat_paint("BLR_Soot", PAL["soot"], under=PAL["soot_under"], under_metal=0.0, rough=0.85, wear=0.6, seed=21)
dial_png = make_dial(os.path.join(TEXSRC, "T_Boiler_03_Dial_Src.png"), labels=("0", "1", "2", "3", "4"),
                     unit="bar", red_from=0.75, sweep=(150.0, 30.0), title="BOILER")
plate_png = make_label(os.path.join(TEXSRC, "T_Boiler_03_Plate_Src.png"), ["BATHHOUSE", "BOILER  No.3"],
                       size=(1024, 288), sizes=[0.42, 0.36])
dial_m = mat_image("BLR_Dial", dial_png, rough=0.35)
plate_m = mat_image("BLR_Plate", plate_png, rough=0.4)

FY = -29.0            # front face of the enamel shell (UE y)
# ---------------------------------------------------------------- base
for sx in (-1, 1):
    for sy in (-1, 1):
        lathe("Foot_%d%d" % (sx, sy), M["iron"], (sx * 41, sy * 21, 0),
              [(4.0, 0), (4.0, 1.2), (3.2, 2.0), (3.0, 5.0), (0, 5.0)], verts=20)
box("Plinth", M["iron"], (0, -0.5, 9), (96, 55, 9), bevel=0.012, segs=2)
# ---------------------------------------------------------------- shell
box("Shell", M["body"], (0, -0.5, 60), (94, 57, 96), bevel=0.045, segs=5)
box("TopCap", M["light"], (0, -0.5, 109.5), (98, 61, 5), bevel=0.022, segs=3)
# chrome band across front and sides
box("BandFront", M["chrome"], (0, FY - 0.35, 72), (78, 1.4, 2.2), bevel=0.005, segs=2)
for sx in (-1, 1):
    box("BandSide%d" % sx, M["chrome"], (sx * 47.35, 0, 72), (1.4, 42, 2.2), bevel=0.005, segs=2)
# rivets on the cap front
for i, x in enumerate((-42, -28, -14, 14, 28, 42)):
    lathe("CapRivet%d" % i, M["steel"], (x, -31.0, 109.5), [(0.9, 0), (0.85, 0.35), (0.5, 0.75), (0, 0.85)],
          axis="-Y", verts=12)
# ---------------------------------------------------------------- top: flue + safety valve
lathe("Flue", M["iron"], (0, 12, 112),
      [(13.0, 0), (13.0, 1.6), (10.0, 2.2), (10.0, 6.0), (11.6, 6.4), (11.6, 8.0), (8.6, 8.0), (8.6, 4.0), (0, 4.0)],
      verts=40)
lathe("SafetyValve", M["brass"], (32, -14, 112),
      [(3.2, 0), (3.2, 1.0), (1.6, 1.4), (1.6, 4.0), (2.4, 4.6), (2.4, 6.4), (1.0, 6.8), (1.0, 7.6), (0, 7.6)], verts=20)
torus("SafetyWheel", M["accent"], (32, -14, 120 - 1.2), 2.8, 0.55, verts=24)
for k in range(4):
    a = math.radians(45 + 90 * k)
    box("SafetySpoke%d" % k, M["accent"], (32 + 1.4 * math.cos(a), -14 + 1.4 * math.sin(a), 118.8), (2.8, 0.7, 0.6),
        bevel=0.001, rot=[("Z", math.degrees(a))])
# ---------------------------------------------------------------- right side (-X) valves
for name, z, wm in (("Hot", 82, M["hot"]), ("Cold", 46, M["cold"])):
    lathe("Valve%s" % name, M["brass"], (-47.0, 8, z),
          [(2.6, 0), (2.6, 0.7), (1.5, 0.9), (1.5, 1.7), (2.0, 1.9), (2.0, 2.5), (0.7, 2.6), (0.7, 3.0), (0, 3.0)],
          axis="-X", verts=20)
    torus("Wheel%s" % name, wm,
          (-47.0 - 2.55, 8, z), 2.4, 0.45, axis="-X", verts=24)
    for k in range(3):
        box("Wheel%sSpoke%d" % (name, k), M["chrome"], (-49.55, 8, z), (0.5, 4.6, 0.5), bevel=0.0,
            rot=[("X", 60 * k)])
# ---------------------------------------------------------------- left side (+X) louvres
for i in range(5):
    box("Louvre%d" % i, M["body_dk"], (47.2, 4, 28 + i * 5), (1.0, 26, 2.0), bevel=0.006, segs=2,
        rot=[("Y", 0)])
box("LouvreFrame", M["chrome"], (47.0, 4, 38), (0.8, 30, 26), bevel=0.004, segs=2)
# ---------------------------------------------------------------- back service panel
box("BackPanel", M["body_dk"], (0, 28.4, 58), (50, 1.2, 60), bevel=0.008, segs=2)
for sx in (-1, 1):
    for sz in (-1, 1):
        lathe("BackBolt%d%d" % (sx, sz), M["steel"], (sx * 22, 29.0, 58 + sz * 27),
              [(1.0, 0), (1.0, 0.4), (0.6, 0.8), (0, 0.9)], axis="+Y", verts=6)
# ---------------------------------------------------------------- front: ash drawer
box("AshDrawer", M["iron"], (0, FY - 1.0, 20), (28, 2.4, 9), bevel=0.006, segs=2)
box("AshHandle", M["chrome"], (0, FY - 3.6, 20.5), (12, 1.2, 1.4), bevel=0.004, segs=2)
for sx in (-1, 1):
    box("AshHandlePost%d" % sx, M["chrome"], (sx * 5.2, FY - 2.8, 20.5), (1.2, 1.8, 1.2), bevel=0.002)
# ---------------------------------------------------------------- front: firebox frame (FuelIntake area)
FZ0, FZ1, FX = 28.0, 56.0, 18.5      # outer frame
OZ0, OZ1, OX = 33.0, 51.0, 12.5      # opening = FuelIntake 25 x 18
Y0, Y1 = FY, FY - 6.4                # frame protrudes to y -35.4
yc, yd = (Y0 + Y1) / 2, abs(Y1 - Y0)
box("FrameTop", M["iron"], (0, yc, (OZ1 + FZ1) / 2), (2 * FX, yd, FZ1 - OZ1), bevel=0.01, segs=2)
box("FrameBot", M["iron"], (0, yc, (OZ0 + FZ0) / 2), (2 * FX, yd, OZ0 - FZ0), bevel=0.01, segs=2)
box("FrameL", M["iron"], ((OX + FX) / 2, yc, (OZ0 + OZ1) / 2), (FX - OX, yd, OZ1 - OZ0), bevel=0.006, segs=2)
box("FrameR", M["iron"], (-(OX + FX) / 2, yc, (OZ0 + OZ1) / 2), (FX - OX, yd, OZ1 - OZ0), bevel=0.006, segs=2)
box("FireMouth", soot, (0, FY - 0.6, 42), (2 * OX + 0.4, 1.2, OZ1 - OZ0 + 0.4), bevel=0.0)
for sx in (-1, 1):
    for sz in (-1, 1):
        lathe("FrameRivet%d%d" % (sx, sz), M["steel"], (sx * 15.5, Y1, 42 + sz * 11.5),
              [(0.9, 0), (0.85, 0.3), (0.5, 0.7), (0, 0.8)], axis="-Y", verts=12)
# hinge leaves + fixed knuckles on the frame (+X side); hinge axis x=12.5, y=-37
HX, HY = 12.5, -37.0
for z0 in (30.0, 51.5):
    box("HingeLeaf%d" % z0, M["iron"], (HX + 1.6, (Y1 + HY) / 2, z0 + 1.75), (3.2, abs(HY - Y1) + 0.6, 3.5),
        bevel=0.003)
    cyl("HingeKnuckle%d" % z0, M["steel"], (HX, HY, z0 + 1.75), 1.3, 3.5, verts=16, bevel=0.002)
cyl("HingePin", M["steel"], (HX, HY, 42), 0.55, 27, verts=10, bevel=0.0)
# ---------------------------------------------------------------- front: nameplate
box("PlateFrame", M["brass"], (0, FY - 0.5, 64), (30, 1.0, 9.2), bevel=0.006, segs=2)
pm = box("PlateFace", plate_m, (0, FY - 1.05, 64), (27.2, 0.3, 7.6), bevel=0.0)
plate_src_uv((0, FY - 1.2, 64), 27.2, 7.6, "-Y")(pm.data)
for sx in (-1, 1):
    lathe("PlateRivet%d" % sx, M["brass"], (sx * 14.0, FY - 1.0, 64), [(0.7, 0), (0.65, 0.3), (0.35, 0.6), (0, 0.65)],
          axis="-Y", verts=10)
# ---------------------------------------------------------------- front: gauge housing (GaugeFacePlate area)
GZ, GY = 90.0, FY
lathe("GaugeBack", M["body_dk"], (0, GY, GZ), [(16.2, 0), (16.2, 0.8), (15.0, 1.2), (0, 1.2)], axis="-Y", verts=48)
lathe("GaugeBezel", M["chrome"], (0, GY - 1.2, GZ),
      [(15.0, 0), (15.0, 1.4), (14.6, 2.6), (13.9, 3.0), (13.0, 2.9), (12.7, 2.2), (12.7, 2.0), (0, 2.0)],
      axis="-Y", verts=64)
disc("GaugeDial", dial_m, (0, -32.25, GZ), 12.7, 0.1, "-Y", verts=64, src_uv=True)
# little bolts around the bezel
for k in range(6):
    a = math.radians(30 + 60 * k)
    lathe("BezelBolt%d" % k, M["steel"], (15.6 * math.cos(a), GY - 1.0, GZ + 15.6 * math.sin(a)),
          [(0.55, 0), (0.5, 0.25), (0.3, 0.5), (0, 0.55)], axis="-Y", verts=6)

# ================================================================= Door (pivot = hinge axis)
D = "Door"
DY0, DY1 = -36.0, -38.2
dyc = (DY0 + DY1) / 2
box("DoorPlate", M["iron"], (0, dyc, 42), (25.0, abs(DY1 - DY0), 18.0), group=D, bevel=0.006, segs=2)
box("DoorPanel", M["iron"], (-0.5, DY1 - 0.35, 42), (19.0, 0.8, 12.6), group=D, bevel=0.004, segs=2)
for i, z in enumerate((38.0, 42.0, 46.0)):
    box("DoorLouvre%d" % i, M["iron"], (-1.5, DY1 - 0.95, z), (12.0, 0.8, 1.4), group=D, bevel=0.003, segs=2)
for z0 in (34.5, 46.0):
    cyl("DoorKnuckle%d" % z0, M["steel"], (HX, HY, z0 + 1.75), 1.3, 3.5, group=D, verts=16, bevel=0.002)
# latch handle at the free (-X) end
lathe("LatchBoss", M["chrome"], (-9.5, DY1, 42), [(1.6, 0), (1.6, 0.8), (1.1, 1.2), (0, 1.2)], axis="-Y",
      group=D, verts=20)
box("LatchArm", M["chrome"], (-9.5, DY1 - 1.9, 39.0), (1.2, 1.2, 7.0), group=D, bevel=0.004, segs=2)
lathe("LatchGrip", M["accent"], (-9.5, DY1 - 1.9, 35.0), [(0.01, -2.4), (1.1, -2.2), (1.25, -0.8), (1.1, 0.4), (0, 0.6)],
      axis="+Z", group=D, verts=16)
for sx in (-1, 1):
    for sz in (-1, 1):
        lathe("DoorRivet%d%d" % (sx, sz), M["steel"], (-0.5 + sx * 10.6, DY1, 42 + sz * 7.6),
              [(0.6, 0), (0.55, 0.25), (0.3, 0.5), (0, 0.55)], axis="-Y", group=D, verts=8)

# ================================================================= Needle (pivot = hub, points +X at rest)
N = "Needle"
NY = -34.0
cyl("NeedleHub", M["iron"], (0, -33.3, GZ), 1.5, 2.1, axis="-Y", group=N, verts=20, bevel=0.002)
prism("NeedleBlade", M["accent"], [(-3.6, GZ - 1.0), (0.0, GZ - 0.75), (11.4, GZ - 0.18), (11.9, GZ), (11.4, GZ + 0.18),
                               (0.0, GZ + 0.75), (-3.6, GZ + 1.0)], "XZ", NY + 0.25, NY - 0.25, group=N, bevel=0.0)
lathe("NeedleCap", M["chrome"], (0, NY - 0.25, GZ), [(1.0, 0), (0.95, 0.4), (0.55, 0.85), (0, 1.0)], axis="-Y",
      group=N, verts=16)

aged = age_all_materials()      # ageing strength: palettes.json age_default / assets.<asset>.age
result = {"parts": len(PARTS["coll"].objects), "aged_materials": aged}
