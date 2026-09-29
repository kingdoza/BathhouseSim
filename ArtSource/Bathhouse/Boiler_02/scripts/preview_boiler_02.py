"""Studio preview rig + multi-view renders for Boiler_02 (Eevee)."""
import bpy, math, os
from mathutils import Vector

OUT = r"C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_02\renders"
VIEWS = globals().get("VIEWS", ["front", "right", "back", "front34", "back34", "left", "top"])
RES = globals().get("RES", (900, 1100))
os.makedirs(OUT, exist_ok=True)
sc = bpy.context.scene

def coll(name):
    c = bpy.data.collections.get(name)
    if c is None:
        c = bpy.data.collections.new(name); sc.collection.children.link(c)
    return c

prev = coll("Boiler02_Preview")
for o in list(bpy.data.objects):
    if o.name in ("Camera", "Light") and o.users_collection and o.users_collection[0].name == "Collection":
        bpy.data.objects.remove(o, do_unlink=True)
old = bpy.data.collections.get("Collection")
if old and not old.objects and not old.children:
    bpy.data.collections.remove(old)

def light(name, typ, loc, energy, size=1.0, color=(1, 1, 1)):
    o = bpy.data.objects.get(name)
    if o is None:
        ld = bpy.data.lights.new(name, typ); o = bpy.data.objects.new(name, ld); prev.objects.link(o)
    o.data.energy = energy; o.data.color = color
    if typ == "AREA":
        o.data.size = size
    o.location = loc
    d = Vector((0, 0, 0.65)) - Vector(loc)
    o.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    return o

light("PV_Key", "AREA", (-1.6, -2.2, 2.2), 420, 1.6)
light("PV_Fill", "AREA", (2.4, -1.4, 1.2), 160, 2.0, (0.9, 0.95, 1.0))
light("PV_Rim", "AREA", (1.2, 2.4, 2.0), 260, 1.5)
light("PV_Back2", "AREA", (-2.0, 2.0, 1.0), 120, 1.5)

w = sc.world or bpy.data.worlds.new("World")
sc.world = w
w.use_nodes = True
bg = w.node_tree.nodes.get("Background")
bg.inputs[0].default_value = (0.62, 0.63, 0.65, 1)
bg.inputs[1].default_value = 0.6

cam = bpy.data.objects.get("PV_Camera")
if cam is None:
    cam = bpy.data.objects.new("PV_Camera", bpy.data.cameras.new("PV_Camera")); prev.objects.link(cam)
sc.camera = cam
try:
    sc.render.engine = "BLENDER_EEVEE"
except TypeError:
    sc.render.engine = "BLENDER_EEVEE_NEXT"
sc.render.resolution_x, sc.render.resolution_y = RES
sc.render.resolution_percentage = 100
sc.render.film_transparent = False
sc.view_settings.view_transform = "Standard"
sc.view_settings.look = "None"
sc.view_settings.exposure = globals().get("EXPOSURE", -0.6)
src_lc = bpy.context.view_layer.layer_collection.children.get("Boiler02_Source")
if src_lc and bpy.data.collections.get("Boiler02_Export"):
    src_lc.exclude = True
sc.render.image_settings.file_format = "PNG"

TGT = Vector((0, 0, 0.68))
VIEWDEF = {
    "front": (Vector((0, -1, 0)), True),
    "back": (Vector((0, 1, 0)), True),
    "right": (Vector((1, 0, 0)), True),
    "left": (Vector((-1, 0, 0)), True),
    "top": (Vector((0, -0.35, 1)).normalized(), False),
    "front34": (Vector((0.75, -1, 0.35)).normalized(), False),
    "back34": (Vector((-0.75, 1, 0.35)).normalized(), False),
    "detail": (Vector((0.35, -1, 0.1)).normalized(), False),
    "valves": (Vector((1, -0.6, 0.15)).normalized(), False),
    "bottom": (Vector((0.0, -0.25, -1)).normalized(), False),
}
done = []
for v in VIEWS:
    d, ortho = VIEWDEF[v]
    cam.data.type = "ORTHO" if ortho else "PERSP"
    cam.data.ortho_scale = 1.62
    cam.data.lens = 60
    dist = 4.2
    tgt = {"detail": Vector((0, -0.2, 0.75)), "valves": Vector((0.4, 0.03, 0.84))}.get(v, TGT)
    if v in ("detail", "valves"):
        dist = 1.6 if v == "detail" else 0.9
    cam.location = tgt + d * dist
    cam.rotation_euler = (tgt - cam.location).to_track_quat("-Z", "Y").to_euler()
    sc.render.filepath = os.path.join(OUT, "Boiler_02_%s.png" % v)
    bpy.ops.render.render(write_still=True)
    done.append(sc.render.filepath)
result = {"renders": done}
