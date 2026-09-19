"""High-fidelity procedural authoring script for Vintage Red Industrial Boiler asset.
Executed via Blender Lab MCP socket bridge.
"""
import bpy
import bmesh
import math
import json
from pathlib import Path
from mathutils import Vector, Matrix, Euler
import numpy as np

ROOT = Path(r'C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_01')
TEXTURE_DIR = ROOT / 'textures'
TEXTURE_DIR.mkdir(parents=True, exist_ok=True)

# 1. Scene Reset & Settings
if bpy.context.object and bpy.context.object.mode != 'OBJECT':
    bpy.ops.object.mode_set(mode='OBJECT')
for obj in list(bpy.data.objects):
    bpy.data.objects.remove(obj, do_unlink=True)
for col in list(bpy.data.collections):
    bpy.data.collections.remove(col)
bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=False, do_recursive=True)

scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0

asset_col = bpy.data.collections.new('BOILER_01 | Assets')
scene.collection.children.link(asset_col)
stage_col = bpy.data.collections.new('PRESENTATION | Lights & Cameras')
scene.collection.children.link(stage_col)
collision_col = bpy.data.collections.new('COLLISION | UCX')
scene.collection.children.link(collision_col)

groups = {'body': [], 'hardware': []}

def move_to(obj, col):
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    col.objects.link(obj)

def register(obj, name, mat=None, group='body', col=None):
    obj.name = name
    if mat:
        if isinstance(mat, list):
            for m in mat:
                obj.data.materials.append(m)
        else:
            obj.data.materials.append(mat)
    move_to(obj, col or asset_col)
    if group and group in groups:
        groups[group].append(obj)
    return obj

def apply_bevel(obj, width=0.015, segments=3):
    m = obj.modifiers.new('Bevel', 'BEVEL')
    m.width = width
    m.segments = segments
    m.limit_method = 'ANGLE'
    m.angle_limit = math.radians(25)
    m2 = obj.modifiers.new('WeightedNormal', 'WEIGHTED_NORMAL')
    m2.keep_sharp = True
    return obj

# 2. Textures & Shaders
def create_image_texture(name, width, height, rgba_data):
    img = bpy.data.images.new(name, width=width, height=height)
    img.pixels.foreach_set(rgba_data.astype(np.float32).ravel())
    filepath = str(TEXTURE_DIR / f"{name}.png")
    img.filepath_raw = filepath
    img.file_format = 'PNG'
    img.save()
    return img

def generate_dial_face_image():
    size = 1024
    y, x = np.mgrid[0:size, 0:size].astype(np.float32)
    cx, cy = size / 2.0, size / 2.0
    r = np.sqrt((x - cx)**2 + (y - cy)**2) / (size / 2.0)
    theta = np.arctan2(y - cy, x - cx) # -pi to pi
    deg = (np.degrees(theta) + 360) % 360
    
    # Ivory dial base
    rgb = np.ones((size, size, 3), dtype=np.float32) * np.array([0.94, 0.93, 0.88])[None, None, :]
    
    # Outer dark concentric bezel rings
    rgb[r > 0.96] = [0.12, 0.12, 0.12]
    rgb[(r > 0.88) & (r < 0.91)] = [0.15, 0.15, 0.15]
    rgb[(r > 0.83) & (r < 0.84)] = [0.25, 0.25, 0.25]
    
    # Dial arc from angle 135 deg (bottom-left) sweeping clockwise through top (90 deg) to 45 deg (bottom-right)
    # In math degrees: 135 deg to 90 to 0 to 360 to 45
    # Let's map angle to [0, 1] across the active gauge scale:
    # Scale starts at -135 deg (225 deg) and ends at -45 deg (315 deg) - top half
    # Top is 90 deg. 0 bar is at ~210 deg (bottom-left). 4 bar is at ~330 deg (bottom-right).
    # Active range: from 210 deg down through 90 to -30 (330)
    for i in range(41): # 0 to 4 bar with 0.1 subdivisions
        # Sweep angle from 210 down to -30
        tick_angle = 210.0 - i * (240.0 / 40.0)
        tick_angle = tick_angle % 360
        diff = np.abs((deg - tick_angle + 180) % 360 - 180)
        
        is_major = (i % 10 == 0)
        is_med = (i % 5 == 0)
        
        tick_len = 0.14 if is_major else (0.09 if is_med else 0.05)
        tick_thick = 0.9 if is_major else (0.6 if is_med else 0.35)
        
        mask = (diff < tick_thick) & (r > (0.83 - tick_len)) & (r < 0.83)
        rgb[mask] = [0.1, 0.1, 0.1]
        
    # Center mounting dot
    rgb[r < 0.08] = [0.15, 0.15, 0.15]
    
    # Text block imitation: "bar" and pressure scale markers
    # 0, 1, 2, 3, 4 digit spots
    digit_angles = [210, 150, 90, 30, 330]
    for val, d_ang in enumerate(digit_angles):
        rad_val = 0.62
        d_rad = math.radians(d_ang)
        dx_c = cx + rad_val * (size/2.0) * math.cos(d_rad)
        dy_c = cy + rad_val * (size/2.0) * math.sin(d_rad)
        d_dist = np.sqrt((x - dx_c)**2 + (y - dy_c)**2)
        # Small filled block / digit mark
        rgb[d_dist < 14] = [0.12, 0.12, 0.12]
        
    # "bar" label below center
    bar_mask = (np.abs(x - cx) < 26) & (np.abs(y - (cy - 70)) < 9)
    rgb[bar_mask] = [0.15, 0.15, 0.15]
    
    rgba = np.concatenate([rgb, np.ones((size, size, 1), dtype=np.float32)], axis=2)
    return create_image_texture("T_Boiler_Gauge_Dial", size, size, rgba)

print("Creating textures...")
tex_gauge = generate_dial_face_image()

# Shader Creation
def create_chipped_red_paint_shader():
    mat = bpy.data.materials.new('M_Boiler_Body')
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bsdf = nodes.get('Principled BSDF')
    
    # Texture Coordinate & Mapping
    tex_coord = nodes.new('ShaderNodeTexCoord')
    
    # Noise for organic wear
    noise1 = nodes.new('ShaderNodeTexNoise')
    noise1.inputs['Scale'].default_value = 12.0
    noise1.inputs['Detail'].default_value = 10.0
    noise1.inputs['Roughness'].default_value = 0.65
    links.new(tex_coord.outputs['Object'], noise1.inputs['Vector'])
    
    noise2 = nodes.new('ShaderNodeTexNoise')
    noise2.inputs['Scale'].default_value = 35.0
    noise2.inputs['Detail'].default_value = 6.0
    links.new(tex_coord.outputs['Object'], noise2.inputs['Vector'])
    
    mix_noise = nodes.new('ShaderNodeMix')
    mix_noise.data_type = 'FLOAT'
    mix_noise.inputs['Factor'].default_value = 0.35
    links.new(noise1.outputs['Fac'], mix_noise.inputs['A'])
    links.new(noise2.outputs['Fac'], mix_noise.inputs['B'])
    
    # ColorRamp for paint chipping mask
    chip_ramp = nodes.new('ShaderNodeValToRGB')
    chip_ramp.color_ramp.elements[0].position = 0.52
    chip_ramp.color_ramp.elements[0].color = (0, 0, 0, 1) # 0 = intact paint
    chip_ramp.color_ramp.elements[1].position = 0.62
    chip_ramp.color_ramp.elements[1].color = (1, 1, 1, 1) # 1 = chipped metal
    links.new(mix_noise.outputs['Result'], chip_ramp.inputs['Fac'])
    
    # Base Color Ramp
    # Rich vintage industrial terracotta red with edge wear
    color_mix = nodes.new('ShaderNodeMix')
    color_mix.data_type = 'RGBA'
    # Paint red: deep rich vintage industrial rust-red in Linear color space
    color_mix.inputs['A'].default_value = (0.42, 0.035, 0.020, 1.0)
    # Chipped exposed dark metal: dark charcoal with touch of rust
    color_mix.inputs['B'].default_value = (0.025, 0.024, 0.024, 1.0)
    links.new(chip_ramp.outputs['Color'], color_mix.inputs['Factor'])
    links.new(color_mix.outputs['Result'], bsdf.inputs['Base Color'])
    
    # Roughness
    rough_mix = nodes.new('ShaderNodeMix')
    rough_mix.data_type = 'FLOAT'
    rough_mix.inputs['A'].default_value = 0.38 # smooth paint sheen
    rough_mix.inputs['B'].default_value = 0.75 # rough oxidized iron
    links.new(chip_ramp.outputs['Color'], rough_mix.inputs['Factor'])
    links.new(rough_mix.outputs['Result'], bsdf.inputs['Roughness'])
    
    # Metallic
    metal_mix = nodes.new('ShaderNodeMix')
    metal_mix.data_type = 'FLOAT'
    metal_mix.inputs['A'].default_value = 0.04
    metal_mix.inputs['B'].default_value = 0.85
    links.new(chip_ramp.outputs['Color'], metal_mix.inputs['Factor'])
    links.new(metal_mix.outputs['Result'], bsdf.inputs['Metallic'])
    
    # Bump
    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.06
    bump.inputs['Distance'].default_value = 0.015
    links.new(chip_ramp.outputs['Color'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    
    return mat

def create_cast_iron_shader():
    mat = bpy.data.materials.new('M_Boiler_CastIron')
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    # Dark charcoal cast iron
    bsdf.inputs['Base Color'].default_value = (0.075, 0.07, 0.07, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.68
    bsdf.inputs['Metallic'].default_value = 0.82
    
    # Subtle iron grain bump
    tex_coord = mat.node_tree.nodes.new('ShaderNodeTexCoord')
    noise = mat.node_tree.nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = 50.0
    noise.inputs['Detail'].default_value = 8.0
    mat.node_tree.links.new(tex_coord.outputs['Object'], noise.inputs['Vector'])
    bump = mat.node_tree.nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.03
    bump.inputs['Distance'].default_value = 0.01
    mat.node_tree.links.new(noise.outputs['Fac'], bump.inputs['Height'])
    mat.node_tree.links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    return mat

def create_brass_shader():
    mat = bpy.data.materials.new('M_Boiler_Brass')
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    # Warm golden aged brass
    bsdf.inputs['Base Color'].default_value = (0.78, 0.56, 0.22, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.28
    bsdf.inputs['Metallic'].default_value = 0.94
    return mat

def create_valve_enamel_shader(name, color_rgb):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (*color_rgb, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.35
    bsdf.inputs['Metallic'].default_value = 0.20
    return mat

def create_gauge_shader():
    mat = bpy.data.materials.new('M_Boiler_Gauge')
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    tex_node = mat.node_tree.nodes.new('ShaderNodeTexImage')
    tex_node.image = tex_gauge
    mat.node_tree.links.new(tex_node.outputs['Color'], bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 0.30
    bsdf.inputs['Metallic'].default_value = 0.0
    return mat

mat_body = create_chipped_red_paint_shader()
mat_iron = create_cast_iron_shader()
mat_brass = create_brass_shader()
mat_valve_red = create_valve_enamel_shader('M_Boiler_ValveRed', (0.52, 0.02, 0.015))
mat_valve_blue = create_valve_enamel_shader('M_Boiler_ValveBlue', (0.015, 0.08, 0.58))
mat_gauge = create_gauge_shader()

# 3. Geometry Helpers
def create_box(name, loc, size, mat, bevel_w=0.015, bevel_seg=3, group='body', col=None):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
    obj = bpy.context.object
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    register(obj, name, mat, group, col)
    if bevel_w > 0:
        apply_bevel(obj, bevel_w, bevel_seg)
    return obj

def create_cylinder(name, loc, radius, depth, mat, rot=(0,0,0), vertices=32, bevel_w=0.005, group='body', col=None):
    bpy.ops.mesh.primitive_cylinder_add(radius=radius, depth=depth, vertices=vertices, location=loc, rotation=rot)
    obj = bpy.context.object
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    register(obj, name, mat, group, col)
    if bevel_w > 0:
        apply_bevel(obj, bevel_w, 2)
    return obj

# 4. Modeling Components

# A. Main Boiler Chassis (Rounded Cuboid)
# Size: 0.80m W x 0.50m D x 1.20m H, base at Z=0.12, top at Z=1.32m
print("Modeling main chassis with generous rounded corners...")
# Large soft bevel (0.085m, 6 segments) matches reference perfectly
body = create_box('Boiler_Chassis', (0, 0, 0.72), (0.80, 0.50, 1.20), mat_body, bevel_w=0.085, bevel_seg=6, group='body')

# B. Top Flue Pipe / Chimney (Shorter, chunkier matching reference)
# Base flange collar: Z=1.32m to 1.345m, radius 0.13m
flue_flange = create_cylinder('Flue_Flange', (0, 0, 1.3325), radius=0.13, depth=0.025, mat=mat_iron, bevel_w=0.004, group='body')
# Flue main pipe: Z=1.345 to 1.485m (height 0.14m, radius 0.102m)
flue_pipe = create_cylinder('Flue_Pipe', (0, 0, 1.415), radius=0.102, depth=0.14, mat=mat_iron, bevel_w=0.004, group='body')
# Flue top lip collar: Z=1.485 to 1.505m, radius 0.108m
flue_lip = create_cylinder('Flue_Lip', (0, 0, 1.495), radius=0.108, depth=0.02, mat=mat_iron, bevel_w=0.003, group='body')
# Dark interior hole
flue_bore = create_cylinder('Flue_Bore', (0, 0, 1.46), radius=0.086, depth=0.10, mat=mat_iron, bevel_w=0.0, group='body')

# 6 Perimeter hex bolts around flue flange
for i in range(6):
    ang = i * (math.pi / 3.0)
    rx = 0.118 * math.cos(ang)
    ry = 0.118 * math.sin(ang)
    create_cylinder(f'Flue_Bolt_{i}', (rx, ry, 1.348), radius=0.0075, depth=0.012, mat=mat_iron, vertices=6, bevel_w=0.002, group='body')

# C. Bottom Cast Iron Base Plate & 4 Support Legs
base_plate = create_box('Base_Plate', (0, 0, 0.115), (0.76, 0.46, 0.02), mat_iron, bevel_w=0.005, group='body')
# Embossed cross ribs
rib1 = create_box('Base_Rib_1', (0, 0, 0.103), (0.82, 0.028, 0.012), mat_iron, bevel_w=0.002, group='body')
rib1.rotation_euler.z = math.atan2(0.46, 0.76)
rib2 = create_box('Base_Rib_2', (0, 0, 0.103), (0.82, 0.028, 0.012), mat_iron, bevel_w=0.002, group='body')
rib2.rotation_euler.z = -math.atan2(0.46, 0.76)

# 4 Tapered Legs (from Z=0.0 to 0.11m)
leg_positions = [
    (-0.31, -0.17, 'FL'),
    (0.31, -0.17, 'FR'),
    (-0.31, 0.17, 'BL'),
    (0.31, 0.17, 'BR')
]
for lx, ly, suffix in leg_positions:
    leg = create_cylinder(f'Leg_Strut_{suffix}', (lx, ly, 0.055), radius=0.038, depth=0.10, mat=mat_iron, vertices=16, bevel_w=0.004, group='body')
    foot = create_cylinder(f'Leg_Foot_{suffix}', (lx, ly, 0.008), radius=0.046, depth=0.016, mat=mat_iron, vertices=20, bevel_w=0.003, group='body')
    bracket = create_cylinder(f'Leg_Bracket_{suffix}', (lx, ly, 0.11), radius=0.046, depth=0.014, mat=mat_iron, vertices=20, bevel_w=0.003, group='body')

# D. Front - Pressure Gauge (Manometer)
print("Modeling front pressure gauge...")
# Bezel ring
gauge_bezel = create_cylinder('Gauge_Bezel', (0, -0.258, 1.02), radius=0.108, depth=0.035, mat=mat_iron, rot=(math.pi/2, 0, 0), vertices=32, bevel_w=0.006, group='hardware')

# Dial disc with mathematical planar UV coordinates
bpy.ops.mesh.primitive_cylinder_add(radius=0.096, depth=0.004, vertices=32, location=(0, -0.274, 1.02), rotation=(math.pi/2, 0, 0))
gauge_dial = bpy.context.object
gauge_dial.name = 'Gauge_Dial'
gauge_dial.data.materials.append(mat_gauge)

# Planar unwrap front face using bmesh
bm = bmesh.new()
bm.from_mesh(gauge_dial.data)
uv_layer = bm.loops.layers.uv.verify()
for face in bm.faces:
    for loop in face.loops:
        v = loop.vert.co
        # Cylinder was created in local coordinates where circle is in X-Y plane (before rotation)
        # In local space, radius is 0.096
        u = (v.x / (2.0 * 0.096)) + 0.5
        w = (v.y / (2.0 * 0.096)) + 0.5
        loop[uv_layer].uv = (u, w)
bm.to_mesh(gauge_dial.data)
bm.free()
gauge_dial.data.update()

register(gauge_dial, 'Gauge_Dial', mat_gauge, group='hardware')

# 3D Gauge needle (pointing up-right to ~1.8 bar)
needle_hub = create_cylinder('Needle_Hub', (0, -0.278, 1.02), radius=0.011, depth=0.008, mat=mat_iron, rot=(math.pi/2, 0, 0), vertices=16, bevel_w=0.002, group='hardware')
needle = create_box('Needle_Pointer', (0.024, -0.278, 1.045), (0.005, 0.003, 0.055), mat_valve_red, bevel_w=0.001, group='hardware')
needle.rotation_euler.y = math.radians(48)

# Nameplate plaque beneath gauge
plaque = create_box('Gauge_Plaque', (0, -0.254, 0.87), (0.16, 0.010, 0.042), mat_brass, bevel_w=0.003, group='hardware')
create_cylinder('Plaque_Screw_L', (-0.065, -0.260, 0.87), radius=0.0035, depth=0.008, mat=mat_iron, rot=(math.pi/2, 0, 0), group='hardware')
create_cylinder('Plaque_Screw_R', (0.065, -0.260, 0.87), radius=0.0035, depth=0.008, mat=mat_iron, rot=(math.pi/2, 0, 0), group='hardware')

# E. Front - Cast Iron Furnace / Burner Door
print("Modeling burner door...")
# Raised outer frame at Z=0.56m
door_frame = create_box('Door_Frame', (0, -0.260, 0.56), (0.42, 0.022, 0.34), mat_iron, bevel_w=0.010, bevel_seg=3, group='hardware')
# Inner door panel recessed
door_leaf = create_box('Door_Leaf', (0, -0.269, 0.56), (0.37, 0.018, 0.29), mat_iron, bevel_w=0.006, bevel_seg=3, group='hardware')

# 4 Vertical Ventilation Slots
for idx in range(4):
    sx = -0.075 + idx * 0.050
    create_box(f'Door_Vent_Slot_{idx}', (sx, -0.279, 0.485), (0.016, 0.010, 0.065), mat_iron, bevel_w=0.002, group='hardware')

# 2 Left Hinges
for idx, hz in enumerate([0.65, 0.47]):
    create_cylinder(f'Door_Hinge_Barrel_{idx}', (-0.198, -0.270, hz), radius=0.012, depth=0.052, mat=mat_iron, bevel_w=0.002, group='hardware')
    create_box(f'Door_Hinge_F_{idx}', (-0.182, -0.260, hz), (0.032, 0.014, 0.024), mat_iron, bevel_w=0.002, group='hardware')
    create_box(f'Door_Hinge_D_{idx}', (-0.174, -0.270, hz), (0.032, 0.014, 0.024), mat_iron, bevel_w=0.002, group='hardware')

# Right Pull Handle / Latch
create_cylinder('Handle_Post_Top', (0.165, -0.282, 0.61), radius=0.006, depth=0.024, mat=mat_iron, rot=(math.pi/2, 0, 0), group='hardware')
create_cylinder('Handle_Post_Bot', (0.165, -0.282, 0.51), radius=0.006, depth=0.024, mat=mat_iron, rot=(math.pi/2, 0, 0), group='hardware')
create_cylinder('Handle_Bar', (0.165, -0.294, 0.56), radius=0.008, depth=0.13, mat=mat_iron, bevel_w=0.002, group='hardware')

# F. Front Lower Red Knob
print("Modeling lower knob...")
knob_bezel = create_cylinder('Knob_Bezel', (0, -0.258, 0.27), radius=0.050, depth=0.020, mat=mat_iron, rot=(math.pi/2, 0, 0), vertices=32, bevel_w=0.004, group='hardware')
knob_body = create_cylinder('Knob_Body', (0, -0.272, 0.27), radius=0.038, depth=0.026, mat=mat_valve_red, rot=(math.pi/2, 0, 0), vertices=32, bevel_w=0.006, group='hardware')
# Small downward arrow mark at 6 o'clock
create_box('Knob_Indicator', (0, -0.267, 0.222), (0.006, 0.006, 0.010), mat_brass, bevel_w=0.001, group='hardware')

# G. Right Side Valves (Red upper, Blue lower)
print("Modeling side valves...")
def create_valve_assembly(z_pos, handle_mat, name_prefix):
    # Brass pipe nipple from X=0.40m
    create_cylinder(f'{name_prefix}_Pipe', (0.445, 0.01, z_pos), radius=0.024, depth=0.09, mat=mat_brass, rot=(0, math.pi/2, 0), vertices=24, bevel_w=0.003, group='hardware')
    create_cylinder(f'{name_prefix}_HexNut', (0.472, 0.01, z_pos), radius=0.033, depth=0.022, mat=mat_brass, rot=(0, math.pi/2, 0), vertices=6, bevel_w=0.002, group='hardware')
    create_cylinder(f'{name_prefix}_Neck', (0.505, 0.01, z_pos), radius=0.018, depth=0.05, mat=mat_brass, rot=(0, math.pi/2, 0), vertices=20, bevel_w=0.002, group='hardware')
    
    # 6-lobe flower wheel at X=0.535m
    create_cylinder(f'{name_prefix}_WheelHub', (0.535, 0.01, z_pos), radius=0.018, depth=0.016, mat=handle_mat, rot=(0, math.pi/2, 0), vertices=20, bevel_w=0.002, group='hardware')
    create_cylinder(f'{name_prefix}_RetainingNut', (0.546, 0.01, z_pos), radius=0.008, depth=0.010, mat=mat_brass, rot=(0, math.pi/2, 0), vertices=6, bevel_w=0.001, group='hardware')
    create_cylinder(f'{name_prefix}_OuterRim', (0.535, 0.01, z_pos), radius=0.065, depth=0.014, mat=handle_mat, rot=(0, math.pi/2, 0), vertices=36, bevel_w=0.003, group='hardware')
    
    for i in range(6):
        ang = i * (math.pi / 3.0)
        sx = 0.035 * math.cos(ang)
        sz = 0.035 * math.sin(ang)
        spoke = create_box(f'{name_prefix}_Spoke_{i}', (0.535, 0.01 + sx, z_pos + sz), (0.010, 0.038, 0.008), handle_mat, bevel_w=0.002, group='hardware')
        spoke.rotation_euler.x = -ang
        
        px = 0.068 * math.cos(ang)
        pz = 0.068 * math.sin(ang)
        create_cylinder(f'{name_prefix}_Lobe_{i}', (0.535, 0.01 + px, z_pos + pz), radius=0.016, depth=0.016, mat=handle_mat, rot=(0, math.pi/2, 0), vertices=16, bevel_w=0.003, group='hardware')

create_valve_assembly(z_pos=0.96, handle_mat=mat_valve_red, name_prefix='Valve_Upper_Red')
create_valve_assembly(z_pos=0.60, handle_mat=mat_valve_blue, name_prefix='Valve_Lower_Blue')

# H. Back Maintenance Hatch
print("Modeling back hatch...")
hatch_panel = create_box('Back_Hatch_Panel', (0, 0.256, 0.72), (0.56, 0.014, 0.86), mat_body, bevel_w=0.025, bevel_seg=4, group='hardware')
hatch_bolt_coords = [
    (-0.23, 0.34),
    (0.23, 0.34),
    (-0.23, 1.10),
    (0.23, 1.10)
]
for idx, (bx, bz) in enumerate(hatch_bolt_coords):
    create_cylinder(f'Hatch_Bolt_{idx}', (bx, 0.265, bz), radius=0.012, depth=0.012, mat=mat_iron, rot=(math.pi/2, 0, 0), vertices=6, bevel_w=0.002, group='hardware')

# 5. Collision Meshes (UCX)
print("Creating UCX collision hulls...")
collisions = []
def add_ucx_box(name, loc, size):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.display_type = 'WIRE'
    obj.hide_render = True
    move_to(obj, collision_col)
    collisions.append(obj)
    return obj

add_ucx_box('UCX_SM_Boiler_01_01', (0, 0, 0.72), (0.84, 0.54, 1.22))
add_ucx_box('UCX_SM_Boiler_01_02', (0, 0, 1.42), (0.24, 0.24, 0.20))
for idx, (lx, ly, _) in enumerate(leg_positions):
    add_ucx_box(f'UCX_SM_Boiler_01_0{3+idx}', (lx, ly, 0.06), (0.11, 0.11, 0.14))
add_ucx_box('UCX_SM_Boiler_01_07', (0.47, 0.01, 0.78), (0.18, 0.20, 0.60))
add_ucx_box('UCX_SM_Boiler_01_08', (0, -0.27, 0.56), (0.44, 0.06, 0.36))

# 6. UV Unwrap & Mesh Finalization
print("Finalizing meshes and UV mapping...")
def select_only(objects):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:
        o.hide_set(False)
        o.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]

def finish_group(key, final_name):
    objs = groups[key]
    if not objs:
        return None
    select_only(objs)
    bpy.ops.object.convert(target='MESH')
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = final_name
    
    scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.008)
    bpy.ops.object.mode_set(mode='OBJECT')
    
    obj.data.uv_layers.active.name = 'UV0'
    obj.data.uv_layers.new(name='UV1_Lightmap', do_init=True)
    obj.data.uv_layers.active_index = 0
    return obj

sm_body = finish_group('body', 'SM_Boiler_01_Body')
sm_hardware = finish_group('hardware', 'SM_Boiler_01_Hardware')
export_objects = [sm_body, sm_hardware]

for obj in export_objects:
    obj['asset_authoring'] = 'Vintage Red Industrial Boiler (Authored via Antigravity Blender MCP)'
    obj['units'] = 'metres'
    obj['fbx_export_axes'] = 'forward -Y, up Z'

# 7. FBX & GLB Export
print("Exporting FBX & GLB...")
def export_fbx(filename, mesh_objs, coll_objs):
    all_objs = mesh_objs + coll_objs
    select_only(all_objs)
    bpy.ops.export_scene.fbx(
        filepath=str(ROOT / filename),
        use_selection=True,
        object_types={'MESH'},
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y',
        axis_up='Z',
        bake_space_transform=False,
        use_mesh_modifiers=True,
        mesh_smooth_type='FACE',
        path_mode='COPY',
        embed_textures=True,
        add_leaf_bones=False
    )

export_fbx('SM_Boiler_01.fbx', export_objects, collisions)

for obj in collisions:
    obj.hide_set(True)

select_only(export_objects)
bpy.ops.export_scene.gltf(
    filepath=str(ROOT / 'Boiler_01.glb'),
    export_format='GLB',
    use_selection=True,
    export_apply=True,
    export_texcoords=True,
    export_normals=True,
    export_materials='EXPORT'
)

# 8. Studio Presentation & Turnaround Renders
print("Setting up lighting & camera...")
def point_at(obj, target):
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()

def add_area_light(name, loc, energy, color, size, target):
    data = bpy.data.lights.new(name, 'AREA')
    data.energy = energy
    data.color = color
    data.shape = 'DISK'
    data.size = size
    obj = bpy.data.objects.new(name, data)
    stage_col.objects.link(obj)
    obj.location = loc
    point_at(obj, target)
    return obj

# Studio lighting
add_area_light('Light_Key', (-2.8, -3.8, 3.2), 340, (1.0, 0.96, 0.90), 3.0, (0, 0, 0.72))
add_area_light('Light_Fill', (3.5, -2.2, 2.5), 200, (0.88, 0.94, 1.0), 3.5, (0, 0, 0.72))
add_area_light('Light_Rim', (0.0, 4.0, 3.2), 420, (1.0, 0.92, 0.82), 2.8, (0, 0, 0.72))
add_area_light('Light_FrontSoft', (0.0, -3.2, 0.8), 120, (0.95, 0.95, 0.95), 2.5, (0, 0, 0.72))

# World background
scene.world.use_nodes = True
bg_node = scene.world.node_tree.nodes.get('Background')
if bg_node:
    bg_node.inputs[0].default_value = (0.84, 0.84, 0.85, 1.0) # Light clean studio grey
    bg_node.inputs[1].default_value = 0.65

# Shadow catcher floor plane (invisible ground, only renders soft contact shadow!)
bpy.ops.mesh.primitive_plane_add(size=20.0, location=(0, 0, 0.0))
floor_obj = bpy.context.object
floor_obj.name = 'Studio_Floor'
floor_obj.is_shadow_catcher = True
move_to(floor_obj, stage_col)

# Camera
cam_data = bpy.data.cameras.new('Boiler_Cam')
cam_obj = bpy.data.objects.new('Boiler_Cam', cam_data)
stage_col.objects.link(cam_obj)
scene.camera = cam_obj

scene.render.engine = 'CYCLES'
scene.cycles.samples = 64
scene.cycles.use_denoising = True
scene.view_settings.view_transform = 'AgX'
scene.render.image_settings.file_format = 'PNG'

# Render Hero view
print("Rendering Hero view...")
scene.render.resolution_x = 1600
scene.render.resolution_y = 1200
cam_data.type = 'PERSP'
cam_data.lens = 65
cam_obj.location = (2.4, -3.0, 1.75)
point_at(cam_obj, (0, 0, 0.72))
scene.render.filepath = str(ROOT / 'Boiler_01_Hero.png')
bpy.ops.render.render(write_still=True)

# Render Orthographic Turnaround Views
views = {
    'FRONT': ((0, -4.2, 0.72), (0, 0, 0.72), 1.95),
    'BACK': ((0, 4.2, 0.72), (0, 0, 0.72), 1.95),
    'LEFT': ((-4.2, 0, 0.72), (0, 0, 0.72), 1.95),
    'RIGHT': ((4.2, 0, 0.72), (0, 0, 0.72), 1.95),
    'TOP': ((0, 0, 4.2), (0, 0, 0.72), 1.65),
    'BOTTOM': ((0, 0, -4.2), (0, 0, 0.72), 1.65),
    'FRONT_3_4': ((2.4, -3.0, 1.75), (0, 0, 0.72), 2.15),
    'BACK_3_4': ((-2.4, 3.0, 1.75), (0, 0, 0.72), 2.15)
}

cam_data.type = 'ORTHO'
scene.render.resolution_x = 800
scene.render.resolution_y = 800

for view_name, (cam_pos, target_pos, ortho_scale) in views.items():
    print(f"Rendering view {view_name}...")
    if view_name == 'BOTTOM':
        floor_obj.hide_render = True
    else:
        floor_obj.hide_render = False
        
    cam_obj.location = cam_pos
    point_at(cam_obj, target_pos)
    cam_data.ortho_scale = ortho_scale
    scene.render.filepath = str(ROOT / f'Boiler_01_View_{view_name}.png')
    bpy.ops.render.render(write_still=True)

floor_obj.hide_render = False

# 9. Pack & Save
print("Saving blend file...")
for img in bpy.data.images:
    if img.source == 'FILE' or img.name.startswith('T_'):
        try:
            img.pack()
        except RuntimeError:
            pass

bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / 'Boiler_01.blend'))

# 10. Metadata & Manifest
stats = {}
for obj in export_objects:
    obj.data.calc_loop_triangles()
    stats[obj.name] = {
        'vertices': len(obj.data.vertices),
        'triangles': len(obj.data.loop_triangles),
        'materials': [m.name for m in obj.data.materials if m],
        'uv_channels': [u.name for u in obj.data.uv_layers]
    }
stats['collision_hulls'] = len(collisions)
stats['nominal_dimensions_m'] = [0.80, 0.50, 1.51]

(ROOT / 'asset_manifest.json').write_text(json.dumps(stats, indent=2), encoding='utf-8')
print("Build finished successfully!")
