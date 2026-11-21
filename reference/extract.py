#!/usr/bin/env python3
"""Extract unchanged routines only after verifying the frozen stock source."""
import hashlib, json, re, sys
from pathlib import Path
source, output = map(Path, sys.argv[1:])
manifest = json.loads((Path(__file__).with_name("source.json")).read_text())
path = source / "jewel-2.6.0.f.stock"
if not path.exists(): path = source / "jewel-2.6.0.f"
data = path.read_bytes()
if hashlib.sha256(data).hexdigest() != manifest["sha256"]:
    raise SystemExit("Refusing unrecognized or patched JEWEL core source")
lines = data.decode().splitlines(keepends=True)
selected = []
for name, bounds in manifest["routines"].items():
    block = lines[bounds[0]-1:bounds[1]]
    if not re.search(r"FUNCTION\s+"+name+r"\(", block[0], re.I):
        raise SystemExit("Routine location mismatch: " + name)
    selected.extend(block)
output.write_text("".join(selected))
