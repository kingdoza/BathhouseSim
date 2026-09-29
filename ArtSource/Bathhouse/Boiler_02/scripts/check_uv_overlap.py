"""Rasterises UV triangles (dumped by finalize_boiler_02.py) and reports coverage/overlap."""
import numpy as np, sys, os, json
N = 1024
d = sys.argv[1] if len(sys.argv) > 1 else "."
out = {}
for nm in ["UV0", "UV1_Lightmap"]:
    T = np.load(os.path.join(d, nm + "_tris.npy")).astype(np.float64) * N
    cnt = np.zeros((N, N), np.int32)
    area = 0.5 * ((T[:, 1, 0] - T[:, 0, 0]) * (T[:, 2, 1] - T[:, 0, 1]) - (T[:, 2, 0] - T[:, 0, 0]) * (T[:, 1, 1] - T[:, 0, 1]))
    for t, a in zip(T, area):
        if abs(a) < 1e-9:
            continue
        x0, y0 = np.floor(t.min(0)).astype(int); x1, y1 = np.ceil(t.max(0)).astype(int)
        x0, y0, x1, y1 = max(x0, 0), max(y0, 0), min(x1, N), min(y1, N)
        if x1 <= x0 or y1 <= y0:
            continue
        xs, ys = np.meshgrid(np.arange(x0, x1) + 0.5, np.arange(y0, y1) + 0.5)
        s = np.sign(a); ok = np.ones_like(xs, bool)
        for i in range(3):
            p, q = t[i], t[(i + 1) % 3]
            ok &= ((q[0] - p[0]) * (ys - p[1]) - (q[1] - p[1]) * (xs - p[0])) * s > 1e-9
        cnt[y0:y1, x0:x1] += ok
    cov, ov = int((cnt > 0).sum()), int((cnt > 1).sum())
    out[nm] = {"triangles": int(len(T)), "zero_area": int((np.abs(area) < 1e-9).sum()),
               "coverage_pct": round(100 * cov / N / N, 1), "overlap_px_1024": ov}
print(json.dumps(out))
