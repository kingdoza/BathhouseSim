"""Compose 8-view turnaround collage matching the user reference sheet."""
import bpy
import numpy as np
from pathlib import Path

ROOT = Path(r'C:\UnrealProjects\BathhouseSim\ArtSource\Bathhouse\Boiler_01')

view_names = [
    ['FRONT', 'BACK', 'LEFT', 'RIGHT'],
    ['TOP', 'BOTTOM', 'FRONT_3_4', 'BACK_3_4']
]

# Load and composite
grid_h = 2
grid_w = 4
tile_w = 800
tile_h = 800
composite = np.zeros((grid_h * tile_h, grid_w * tile_w, 4), dtype=np.float32)

for r_idx, row in enumerate(view_names):
    for c_idx, name in enumerate(row):
        img_path = str(ROOT / f'Boiler_01_View_{name}.png')
        b_img = bpy.data.images.load(img_path)
        pixels = np.empty((tile_w * tile_h * 4,), dtype=np.float32)
        b_img.pixels.foreach_get(pixels)
        tile = pixels.reshape((tile_h, tile_w, 4))
        # Blender pixel array has (0,0) at bottom-left
        # Place into composite grid
        y_start = (grid_h - 1 - r_idx) * tile_h
        y_end = y_start + tile_h
        x_start = c_idx * tile_w
        x_end = x_start + tile_w
        composite[y_start:y_end, x_start:x_end, :] = tile
        bpy.data.images.remove(b_img)

# Save composite image
comp_img = bpy.data.images.new('Boiler_01_Turnaround', width=grid_w * tile_w, height=grid_h * tile_h)
comp_img.pixels.foreach_set(composite.ravel())
comp_img.filepath_raw = str(ROOT / 'Boiler_01_Turnaround.png')
comp_img.file_format = 'PNG'
comp_img.save()
bpy.data.images.remove(comp_img)
print("Turnaround composite saved successfully!")
