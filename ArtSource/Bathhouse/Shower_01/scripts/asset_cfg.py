# Shower_01 pipeline config (UE cm, BP_Shower SceneRoot local; front = UE +X)
ASSET = "Shower_01"
ROOT = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Shower_01"
RES = 4096
PIVOTS = {"Body": (0, 0, 0)}
def _box(c0, c1):
    return [(x, y, z) for x in (c0[0], c1[0]) for y in (c0[1], c1[1]) for z in (c0[2], c1[2])]
HULLS = {
    "Body": [
        _box((-40, -60, 0), (17.6, 60, 56)),               # counter block + top
        _box((-41, -60, 56), (-29, 60, 188)),              # tiled back panel + cap
        _box((-30, -60, 60), (-6, -34, 180)),              # station (-Y): mixer, spout, riser, head
        _box((-30, 34, 60), (-6, 60, 180)),                # station (+Y)
        _box((14, -51.5, 40.5), (21.4, 51.5, 43.5)),       # grab rail
    ],
}
FRONT = "+X"
TARGET = (0, 0, 0.92)
ORTHO = 2.1
DETAIL_TARGET = (0.0, -0.3, 1.0)
DETAIL_DIST = 1.9
