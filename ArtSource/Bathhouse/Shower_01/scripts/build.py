"""Shower_01 - two-person tiled shower station for BP_Shower.

Coordinates: BP_Shower SceneRoot-local Unreal cm. Front = UE +X (customers stand at (120,+-55) facing -X),
back = UE -X (wall side). Footprint X+-40, Y+-60, Z0-56. Bottle display surface Z=56, bottle centres X=0,
Shampoo Y -32/-18, BodyWash Y 18/32 (ShampooGroup / BodyWashGroup slots).
Single export mesh: SM_Shower_01 (FacilityVisual). The tall tiled back panel exceeds the 56cm footprint height on purpose.
"""
import os
exec(open(r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\_Pipeline\bh_lib.py", encoding="utf-8").read())
apply_asset_palette("Shower_01")          # theme + overrides: _Pipeline/palettes.json

ROOT = globals().get("ROOT_OVERRIDE", r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Shower_01")
TEXSRC = os.path.join(ROOT, "textures", "src")
os.makedirs(TEXSRC, exist_ok=True)

reset_scene()
begin_parts("Shower_01")
M = standard_mats("SHW")
tile = mat_tile_box("SHW_TileMain", PAL["tile_a"], grout=PAL["tile_grout"], tile_cm=7.5, seed=31,
                    color2=PAL["tile_b"])
tile_band = mat_tile_box("SHW_TileBand", PAL["tile_band"], grout=PAL["tile_grout"], tile_cm=7.5, seed=32)
mirror = mat_mirror("SHW_Mirror")
blue = M["cold"]
sign_png = make_label(os.path.join(TEXSRC, "T_Shower_01_Sign_Src.png"), ["SHOWER"], size=(1024, 256), sizes=[0.7],
                      bg=PAL["label"], ink=PAL["sign_ink"])
sign_m = mat_image("SHW_Sign", sign_png, rough=0.3)

BX0, BX1 = -40.0, 14.0           # tiled counter block (back..front)
TOP = 56.0                       # display surface (slot Z)
PX0, PX1 = -40.0, -30.0          # back panel
PTOP = 184.0

# ---------------------------------------------------------------- counter block + top
box("CounterBlock", tile, ((BX0 + BX1) / 2, 0, (TOP - 4.5) / 2), (BX1 - BX0, 119, TOP - 4.5),
    bevel=0.008, segs=2)
box("CounterKick", tile_band, ((BX0 + BX1) / 2 + 0.6, 0, 3.0), (BX1 - BX0 + 0.4, 119.4, 6.0), bevel=0.006, segs=2)
box("CounterTop", M["light"], (-12.5, 0, TOP - 2.25), (57, 120, 4.5), bevel=0.014, segs=3)
box("CounterTrim", M["chrome"], (17.1, 0, TOP - 2.3), (0.9, 118, 1.2), bevel=0.003, segs=2)
# bottle niche hints: shallow rounded mats under each bottle group
# ---------------------------------------------------------------- back panel
box("BackPanel", tile, ((PX0 + PX1) / 2, 0, (TOP + PTOP) / 2), (PX1 - PX0, 119, PTOP - TOP), bevel=0.008, segs=2)
box("PanelBand", tile_band, (PX1 + 0.3, 0, 100), (0.8, 119, 7.5), bevel=0.002)
box("PanelCap", M["light"], ((PX0 + PX1) / 2, 0, PTOP + 2.0), (PX1 - PX0 + 2.0, 120, 4.0), bevel=0.012, segs=3)
box("SignFrame", M["chrome"], (PX1 + 0.4, 0, 170), (0.8, 34, 9.5), bevel=0.004, segs=2)
sm = box("SignFace", sign_m, (PX1 + 0.85, 0, 170), (0.3, 31.5, 7.4), bevel=0.0)
plate_src_uv((PX1 + 1.0, 0, 170), 31.5, 7.4, "+X")(sm.data)

# ---------------------------------------------------------------- two shower stations
for sy in (-1, 1):
    S = "L" if sy < 0 else "R"
    yc = sy * 47.0
    # mixer escutcheon + body
    lathe("Escutcheon" + S, M["chrome"], (PX1, yc, 80), [(6.5, 0), (6.5, 0.4), (5.6, 1.2), (0, 1.4)], axis="+X",
          verts=40)
    cyl("MixerBody" + S, M["chrome"], (PX1 + 3.0, yc, 80), 2.0, 16.0, axis="+Y", verts=24, bevel=0.003)
    for side, cap in ((-1, blue), (1, M["hot"])):
        hy = yc + side * 8.6 * sy
        cyl("Stem%s%d" % (S, side), M["chrome"], (PX1 + 3.0, hy, 80), 1.1, 2.0, axis="+Y", verts=12, bevel=0.0)
        for k in range(4):
            box("Cross%s%d_%d" % (S, side, k), M["chrome"], (PX1 + 3.0, hy, 80), (1.2, 1.4, 7.6), bevel=0.005,
                segs=2, rot=[("Y", 45 + 90 * k)])
        lathe("CrossCap%s%d" % (S, side), cap, (PX1 + 3.0, hy + side * sy * 0.8, 80),
              [(0, 0), (1.6, 0), (1.6, 0.6), (1.0, 1.1), (0, 1.2)], axis="+Y" if side * sy > 0 else "-Y", verts=16)
    # spout
    tube("Spout" + S, M["chrome"], [(PX1 + 0.5, yc, 73), (PX1 + 13, yc, 73), (PX1 + 16, yc, 67)], 1.2, verts=16, bend=3)
    lathe("SpoutTip" + S, M["chrome"], (PX1 + 16.0, yc, 67), [(1.4, -1.5), (1.4, 0), (0, 0)], axis="+Z", verts=16)
    # riser + gooseneck + head
    tube("Riser" + S, M["chrome"], [(PX1 + 3.0, yc, 82), (PX1 + 3.0, yc, 166), (PX1 + 6.0, yc, 172),
                                    (PX1 + 22.0, yc, 172)], 1.2, verts=16, bend=6)
    lathe("RiserClamp" + S, M["chrome"], (PX1, yc, 140), [(0, 0), (2.2, 0), (2.2, 3.4), (0, 3.4)], axis="+X", verts=16)
    lathe("ShowerHead" + S, M["chrome"], (PX1 + 22.0, yc, 172),
          [(0, 1.2), (1.6, 1.2), (2.0, 0), (6.6, -4.6), (7.0, -5.6), (6.6, -6.2), (0, -6.2)], axis="+Z", verts=40)
    lathe("ShowerFace" + S, M["steel"], (PX1 + 22.0, yc, 165.75), [(0, 0), (6.4, 0), (6.4, 0.2), (0, 0.2)],
          axis="+Z", verts=40)
    # round mirror above the bottles
    lathe("MirrorRim" + S, M["chrome"], (PX1, sy * 24.0, 128), [(13.0, 0), (13.0, 0.6), (12.4, 1.4), (11.6, 1.4),
                                                               (11.6, 1.0), (0, 1.0)], axis="+X", verts=56)
    disc("MirrorGlass" + S, mirror, (PX1 + 1.05, sy * 24.0, 128), 11.6, 0.1, "+X", verts=56)
    # soap ledge / shelf under mirror
    box("Ledge" + S, M["light"], (PX1 + 4.0, sy * 24.0, 108), (8.0, 24, 1.6), bevel=0.006, segs=2)
    for k in (-1, 1):
        box("LedgeBracket%s%d" % (S, k), M["chrome"], (PX1 + 2.0, sy * 24.0 + k * 9.0, 105.5), (4.0, 1.2, 4.0),
            bevel=0.003, taper=(1.0, 1.0))
    # hand-shower hook
    tube("Hook" + S, M["chrome"], [(PX1, sy * 57.0, 120), (PX1 + 3.5, sy * 57.0, 120), (PX1 + 4.5, sy * 57.0, 124)],
         0.5, verts=8, bend=1.5)
# grab rail along the counter front
for sy in (-1, 1):
    box("RailPost%d" % sy, M["chrome"], (BX1 + 3.0, sy * 50.0, 42.0), (6.0, 2.0, 2.0), bevel=0.004, segs=2)
tube("Rail", M["chrome"], [(BX1 + 6.0, -50.0, 42.0), (BX1 + 6.0, 50.0, 42.0)], 1.3, verts=16)
# floor drain gutter in front of the block
box("Gutter", M["steel"], (BX1 + 9.0, 0, 0.4), (12, 116, 0.8), bevel=0.002)
for i in range(23):
    box("GutterSlot%d" % i, M["iron"], (BX1 + 9.0, -52.8 + i * 4.8, 0.82), (9, 1.6, 0.1), bevel=0.0)

aged = age_all_materials()      # ageing strength: palettes.json age_default / assets.<asset>.age
result = {"parts": len(PARTS["coll"].objects), "aged_materials": aged}
