"""Send a pipeline step to the open Blender session (Blender Lab MCP add-on bridge, 127.0.0.1:9876).

    python run_bridge.py <asset_dir> build            # exec <asset_dir>/scripts/build.py
    python run_bridge.py <asset_dir> <stage> [K=V..]  # exec asset_cfg.py + _Pipeline/bh_bake.py with STAGE
    python run_bridge.py --code "<python>"            # raw snippet

<asset_dir>/scripts/asset_cfg.py defines ASSET, ROOT, RES, PIVOTS, HULLS and preview globals.
Extra K=V pairs are evaluated as Python literals and override config globals (e.g. VIEWS=['front']).
"""
import json, pathlib, socket, sys

PIPE = pathlib.Path(__file__).resolve().parent

def send(code, timeout=3000):
    with socket.create_connection(("127.0.0.1", 9876), timeout=10) as conn:
        conn.settimeout(timeout)
        conn.sendall(json.dumps({"type": "execute", "code": code, "strict_json": True}).encode("utf-8") + b"\0")
        buf = bytearray()
        while not buf.endswith(b"\0"):
            chunk = conn.recv(65536)
            if not chunk:
                raise RuntimeError("Blender disconnected before returning a result")
            buf.extend(chunk)
    return json.loads(buf[:-1].decode("utf-8"))

def main():
    a = sys.argv[1:]
    if a[0] == "--code":
        code = a[1]
    else:
        d = pathlib.Path(a[0]).resolve()
        stage = a[1]
        extra = "\n".join(a[2:])
        if stage == "build":
            code = extra + "\n" + (d / "scripts" / "build.py").read_text(encoding="utf-8")
        else:
            cfg = (d / "scripts" / "asset_cfg.py").read_text(encoding="utf-8")
            code = (cfg + "\nSTAGE = %r\n" % stage + extra + "\n"
                    + (PIPE / "bh_bake.py").read_text(encoding="utf-8"))
    r = send(code)
    s = json.dumps(r, ensure_ascii=False, indent=1)
    print(s if len(s) < 6000 else s[:6000] + "\n...[truncated]")
    if r.get("status") != "ok":
        sys.exit(1)

if __name__ == "__main__":
    main()
