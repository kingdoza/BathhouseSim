#!/bin/sh
# usage: sh run_all.sh <asset_dir>   -> build, bake, finalize, export, validate, preview, save
cd "$(dirname "$0")"
B="$1"
for st in build prep bake_base bake_rough bake_metal bake_normal bake_ao finalize export validate; do
  python run_bridge.py "$B" $st > "$B/_last_$st.log" 2>&1 || { echo "FAIL $st"; head -40 "$B/_last_$st.log"; exit 1; }
done
python run_bridge.py "$B" preview "VIEWS=['front34','front','front34l','right','left','back34','top','detail']" "VIEW_RES=(700,900)" > "$B/_last_preview.log" 2>&1 || { echo FAIL preview; exit 1; }
python run_bridge.py "$B" save > "$B/_last_save.log" 2>&1 || { echo FAIL save; cat "$B/_last_save.log"; exit 1; }
rm -f "$B"/_last_*.log "$B/$(basename "$B").blend1"
python -c "import json,sys;r=json.load(open(sys.argv[1]+'/validation_report.json'));print('validate',r['passed'],'/',r['total']);[print(k,v['size_ue_cm'],v['triangles'],v['hulls']) for k,v in r['meshes'].items()]" "$B"
