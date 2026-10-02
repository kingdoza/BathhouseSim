# Boiler_03 pipeline config (UE cm, BP_Boiler SceneRoot local)
ASSET = "Boiler_03"
ROOT = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_03"
RES = 2048
PIVOTS = {"Body": (0, 0, 0), "Door": (12.5, -37, 42), "Needle": (0, -34, 90)}
def _box(c0, c1):
    return [(x, y, z) for x in (c0[0], c1[0]) for y in (c0[1], c1[1]) for z in (c0[2], c1[2])]
def _cyl(cx, cy, r, z0, z1, n=12):
    import math
    return [(cx + r * math.cos(2 * math.pi * i / n), cy + r * math.sin(2 * math.pi * i / n), z)
            for i in range(n) for z in (z0, z1)]
HULLS = {
    "Body": [
        _box((-49, -30.5, 0), (49, 29.5, 112)),          # shell + plinth + cap
        _cyl(0, 12, 13, 112, 120),                        # flue
        _box((-18.5, -35.4, 14), (18.5, -29, 56)),        # firebox frame + ash drawer
        _box((-16.2, -33.3, 73.8), (16.2, -29, 106.2)),   # gauge housing
    ],
    "Door": [_box((-12.5, -38.2, 33), (12.5, -36, 51))],
}
FRONT = "+Y"          # Blender axis of the facility front (UE -Y)
TARGET = (0, 0, 0.6)
ORTHO = 1.45
DETAIL_TARGET = (0, 0.3, 0.62)
DETAIL_DIST = 1.5
