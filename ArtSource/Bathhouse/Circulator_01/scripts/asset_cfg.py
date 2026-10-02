# Circulator_01 pipeline config (UE cm, BP_Circulator SceneRoot local)
ASSET = "Circulator_01"
ROOT = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Circulator_01"
RES = 2048
PIVOTS = {"Body": (0, 0, 0), "Lever": (30, -44, 30), "Needle": (34.49, -44, 109.92)}
def _box(c0, c1):
    return [(x, y, z) for x in (c0[0], c1[0]) for y in (c0[1], c1[1]) for z in (c0[2], c1[2])]
def _cylx(cy, cz, r, x0, x1, n=12):
    import math
    return [(x, cy + r * math.cos(2 * math.pi * i / n), cz + r * math.sin(2 * math.pi * i / n))
            for i in range(n) for x in (x0, x1)]
HULLS = {
    "Body": [
        _box((-60, -39, 0), (60, 37, 8)),                 # skid
        _box((12, -42.5, 8), (58.5, 36, 120)),            # cabinet + gauge + quadrant + boss
        _cylx(2, 36, 18, -59.5, -24),                     # motor
        _cylx(2, 36, 21, -19.5, -4),                      # volute + inlet
        _box((-19, -23, 52), (-5, 38, 104)),              # discharge riser + gate valve
        _box((-4, -4, 30), (8, 38, 42)),                  # suction pipe
    ],
    "Lever": [_box((26.5, -46, 26), (33.5, -42.6, 68.5))],
}
FRONT = "+Y"
TARGET = (0, 0, 0.6)
ORTHO = 1.55
DETAIL_TARGET = (0.0, 0.35, 0.65)
DETAIL_DIST = 1.7
