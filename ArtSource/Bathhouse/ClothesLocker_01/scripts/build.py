"""ClothesLocker_01 - single locker cell for BP_ClothesLocker (module for the 4/8-cell lockers).

Coordinates: BP_ClothesLocker SceneRoot-local Unreal cm. Front = UE +X (customer at (120,0,0) facing -X).
Cell: X -30..30 (+door hardware to ~33), Y -17.5..17.5 exactly (4/8-cell BPs repeat cells every 35 cm in Y,
so nothing may cross Y +-17.5), Z 0..100. Viewer facing the door: UE -Y is on the viewer's right (handle side),
hinges on UE +Y. Single export mesh: SM_ClothesLocker_01.
"""
import os
exec(open(r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\_Pipeline\bh_lib.py", encoding="utf-8").read())
apply_asset_palette("ClothesLocker_01")          # theme + overrides: _Pipeline/palettes.json

ROOT = globals().get("ROOT_OVERRIDE", r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\ClothesLocker_01")
TEXSRC = os.path.join(ROOT, "textures", "src")
os.makedirs(TEXSRC, exist_ok=True)
reset_scene()
begin_parts("ClothesLocker_01")
M = standard_mats("LKR")
tag_png = make_label(os.path.join(TEXSRC, "T_ClothesLocker_01_Tag_Src.png"), ["No."], size=(512, 512), sizes=[0.62],
                     bg=PAL["label"], ink=PAL["ink"], border=False)
tag_m = mat_image("LKR_Tag", tag_png, rough=0.35)

HY = 17.5
FX = 29.0                     # body front face
# ---------------------------------------------------------------- carcass
box("Plinth", M["iron"], (-1.0, 0, 3.0), (56, 33.6, 6), bevel=0.008, segs=2)
box("Body", M["body_dk"], (-0.5, 0, 52.0), (59, 2 * HY, 92), bevel=0.012, segs=3)
box("TopCap", M["light"], (0.3, 0, 98.25), (60.6, 2 * HY, 3.5), bevel=0.01, segs=3)
box("TopLip", M["chrome"], (30.7, 0, 96.3), (0.8, 2 * HY - 1.6, 1.0), bevel=0.003)
# ---------------------------------------------------------------- door
DX0, DX1 = FX, FX + 1.6
dxc = (DX0 + DX1) / 2
DZ0, DZ1 = 9.0, 94.0
box("Door", M["body"], (dxc, 0, (DZ0 + DZ1) / 2), (DX1 - DX0, 31.6, DZ1 - DZ0), bevel=0.008, segs=3)
box("DoorRib", M["body"], (DX1 + 0.35, 0, 50.5), (0.7, 25.0, 28.0), bevel=0.006, segs=2)
# vent louvres (raised slats) top and bottom
for zbase in (79.0, 14.0):
    for i in range(5):
        box("Louvre%d_%d" % (zbase, i), M["body_dk"], (DX1 + 0.45, 0, zbase + i * 2.6), (0.9, 20.0, 1.3),
            bevel=0.004, segs=2)
    box("LouvreFrame%d" % zbase, M["chrome"], (DX1 + 0.15, 0, zbase + 5.2), (0.4, 22.6, 14.0), bevel=0.002)
# number plate (cream enamel tag in brass oval)
lathe("TagRim", M["brass"], (DX1, 0, 71.0), [(5.2, 0), (5.2, 0.5), (4.6, 0.9), (0, 0.9)], axis="+X", verts=40)
tg = disc("TagFace", tag_m, (DX1 + 0.95, 0, 71.0), 4.5, 0.1, "+X", verts=40)
plate_src_uv((DX1 + 1.0, 0, 71.0), 9.0, 9.0, "+X")(tg.data)
# handle (viewer's right = UE -Y) + cylinder lock
HYL = -10.5
lathe("HandleRose", M["chrome"], (DX1, HYL, 55.0), [(2.0, 0), (2.0, 0.5), (1.5, 0.9), (0, 0.9)], axis="+X", verts=24)
box("HandleNeck", M["chrome"], (DX1 + 1.6, HYL, 55.0), (2.4, 1.4, 1.4), bevel=0.004, segs=2)
box("HandleLever", M["chrome"], (DX1 + 2.5, HYL + 3.2, 55.0), (1.4, 7.6, 1.6), bevel=0.006, segs=2)
lathe("Lock", M["brass"], (DX1, HYL, 47.5), [(1.5, 0), (1.5, 1.1), (1.2, 1.4), (0, 1.4)], axis="+X", verts=24)
box("KeySlot", M["iron"], (DX1 + 1.42, HYL, 47.5), (0.1, 0.45, 1.6), bevel=0.0)
# hinges (UE +Y side)
for z in (22.0, 82.0):
    cyl("Hinge%d" % z, M["chrome"], (DX1 + 0.3, 15.9, z), 0.9, 7.0, verts=14, bevel=0.002)
# side seam strip + rivets (kept inside +-17.5)
for sy in (-1, 1):
    for z in (12.0, 50.0, 88.0):
        lathe("SideRivet%d_%d" % (sy, z), M["steel"], (24.0, sy * (HY - 0.05), z),
              [(0.6, -0.2), (0.6, 0.0), (0, 0.0)], axis="+Y" if sy > 0 else "-Y", verts=8)
# coat hook inside is invisible; add a small brass card holder above the louvres instead
box("CardHolder", M["brass"], (DX1 + 0.3, 0, 89.8), (0.5, 9.0, 3.0), bevel=0.002)

aged = age_all_materials()      # ageing strength: palettes.json age_default / assets.<asset>.age
result = {"parts": len(PARTS["coll"].objects), "aged_materials": aged}
