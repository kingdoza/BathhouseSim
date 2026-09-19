"""Client for executing Blender Python scripts via Blender Lab MCP socket bridge."""
import json
import pathlib
import socket
import sys

def run_script(script_path: str):
    code = pathlib.Path(script_path).read_text(encoding="utf-8")
    with socket.create_connection(("127.0.0.1", 9876), timeout=10) as conn:
        conn.settimeout(1800)  # 30 mins timeout for complex renders
        payload = {"type": "execute", "code": code, "strict_json": True}
        conn.sendall(json.dumps(payload).encode("utf-8") + b"\0")
        chunks = bytearray()
        while not chunks.endswith(b"\0"):
            chunk = conn.recv(65536)
            if not chunk:
                raise RuntimeError("Blender disconnected before returning a result")
            chunks.extend(chunk)
        response = json.loads(chunks[:-1].decode("utf-8"))
        print(json.dumps(response, ensure_ascii=False, indent=2))
        if response.get("status") != "ok":
            sys.exit(1)
        return response.get("result")

if __name__ == "__main__":
    script = sys.argv[1] if len(sys.argv) > 1 else "test.py"
    run_script(script)
