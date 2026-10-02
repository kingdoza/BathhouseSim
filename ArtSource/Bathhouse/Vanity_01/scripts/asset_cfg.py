# Vanity_01 pipeline config (UE cm, BP_Vanity SceneRoot local; front = UE +X)
ASSET = "Vanity_01"
ROOT = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Vanity_01"
RES = 2048
PIVOTS = {"Body": (0, 0, 0)}
def _box(c0, c1):
    return [(x, y, z) for x in (c0[0], c1[0]) for y in (c0[1], c1[1]) for z in (c0[2], c1[2])]
HULLS = {
    "Body": [
        _box((-31, -60.6, 0), (31.6, 60.6, 100)),          # cabinet + legs + counter
        _box((-30, -56, 100), (-22.8, 56, 180)),           # backsplash + mirror
        _box((-29.5, -67.5, 142), (-14, -53, 162.5)),      # sconce -Y
        _box((-29.5, 53, 142), (-14, 67.5, 162.5)),        # sconce +Y
    ],
}
FRONT = "+X"
TARGET = (0, 0, 0.92)
ORTHO = 2.0
DETAIL_TARGET = (0.0, -0.25, 1.0)
DETAIL_DIST = 1.9
