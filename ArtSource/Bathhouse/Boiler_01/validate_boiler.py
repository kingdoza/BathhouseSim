"""Round-trip validation script for Vintage Red Industrial Boiler asset.
Executed via Blender Lab MCP socket bridge.
"""
import bpy
import bmesh
import json
import math
from pathlib import Path
from mathutils import Vector

ROOT = Path(r'C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_01')
bpy.ops.wm.open_mainfile(filepath=str(ROOT / 'Boiler_01.blend'))
source_scene = bpy.context.scene
names = ['SM_Boiler_01_Body', 'SM_Boiler_01_Hardware']
report = {'source_reload': True, 'checks': [], 'components': {}}

def check(label, condition, detail=None):
    report['checks'].append({'check': label, 'passed': bool(condition), 'detail': detail})

def bounds(obj):
    points = [obj.matrix_world @ Vector(p) for p in obj.bound_box]
    return [[min(p[k] for p in points) for k in range(3)], [max(p[k] for p in points) for k in range(3)]]

source_bounds = {name: bounds(bpy.data.objects[name]) for name in names}

for name in names:
    obj = bpy.data.objects.get(name)
    check(f'{name} exists', obj is not None)
    if not obj:
        continue
    check(f'{name} finite vertices', all(math.isfinite(v) for p in obj.data.vertices for v in p.co))
    check(f'{name} 2 UV channels', len(obj.data.uv_layers) == 2)
    check(f'{name} applied scale is 1.0', all(abs(v - 1.0) < 1e-5 for v in obj.scale))
    check(f'{name} origin at floor center', obj.location.length < 1e-5)
    check(f'{name} valid material indices', all(p.material_index < len(obj.data.materials) for p in obj.data.polygons))
    report['components'][name] = {
        'vertices': len(obj.data.vertices),
        'triangles': len(obj.data.polygons),
        'materials': [m.name for m in obj.data.materials if m],
        'bounds_m': source_bounds[name]
    }

# Check files exist
for filename in ['Boiler_01_Hero.png', 'Boiler_01_Turnaround.png', 'Boiler_01.glb', 'SM_Boiler_01.fbx']:
    f = ROOT / filename
    check(f'{filename} exists and non-empty', f.is_file() and f.stat().st_size > 1000, f.stat().st_size if f.exists() else 0)

source_count = len(bpy.data.objects)
temp_scene = bpy.data.scenes.new('Validation_Temp')
bpy.context.window.scene = temp_scene

before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=str(ROOT / 'SM_Boiler_01.fbx'))
added = list(set(bpy.data.objects) - before)

# Expected meshes: 2 asset meshes (Body, Hardware) + 8 UCX collisions = 10 objects
check('SM_Boiler_01.fbx imported mesh count', len(added) == 10, len(added))

for obj in added:
    if obj.name.startswith('UCX_'):
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        is_manifold = all(e.is_manifold for e in bm.edges)
        vol = abs(bm.calc_volume())
        check(f'{obj.name} closed manifold collision', is_manifold and vol > 1e-5, f'vol={vol:.5f}')
        bm.free()
    else:
        matched_name = next((n for n in names if obj.name.startswith(n)), None)
        check(f'{obj.name} recognized asset mesh', matched_name is not None)
        if matched_name:
            act = bounds(obj)
            err = max(abs(act[i][k] - source_bounds[matched_name][i][k]) for i in (0, 1) for k in range(3))
            check(f'{matched_name} FBX bounds accurate (<0.2mm)', err < 0.0002, err)
            check(f'{matched_name} FBX UV channels intact', len(obj.data.uv_layers) == 2)

for obj in added:
    bpy.data.objects.remove(obj, do_unlink=True)

bpy.context.window.scene = source_scene
bpy.data.scenes.remove(temp_scene)

passed = sum(1 for c in report['checks'] if c['passed'])
total = len(report['checks'])
report['summary'] = {'passed': passed, 'total': total, 'all_passed': passed == total}

(ROOT / 'validation_report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
(ROOT / 'validation_result.json').write_text(json.dumps(report['summary'], indent=2), encoding='utf-8')

print(f"Validation finished: {passed}/{total} passed (all_passed={passed == total})")
