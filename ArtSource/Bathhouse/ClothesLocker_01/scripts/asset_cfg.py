# ClothesLocker_01 pipeline config (UE cm, BP_ClothesLocker SceneRoot local; front = UE +X)
ASSET = "ClothesLocker_01"
ROOT = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\ClothesLocker_01"
RES = 2048
PIVOTS = {"Body": (0, 0, 0)}
def _box(c0, c1):
    return [(x, y, z) for x in (c0[0], c1[0]) for y in (c0[1], c1[1]) for z in (c0[2], c1[2])]
HULLS = {"Body": [_box((-30, -17.5, 0), (31.6, 17.5, 100))]}
FRONT = "+X"
TARGET = (0, 0, 0.5)
ORTHO = 1.3
DETAIL_TARGET = (0.3, 0.0, 0.6)
DETAIL_DIST = 1.0
