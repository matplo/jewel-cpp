"""Exercise the installed wheel and confirm that it shares the bundle libraries."""

import ctypes
from pathlib import Path
import os
import subprocess
import sys

from nbclient import NotebookClient
import nbformat

import hepyy
from hepyy.config import get_registry_path
from hepyy.native import wheel_record


def mapped_libraries():
    if sys.platform == "darwin":
        dyld = ctypes.CDLL(None)
        dyld._dyld_image_count.restype = ctypes.c_uint32
        dyld._dyld_get_image_name.argtypes = [ctypes.c_uint32]
        dyld._dyld_get_image_name.restype = ctypes.c_char_p
        return {Path(os.fsdecode(dyld._dyld_get_image_name(i))).resolve()
                for i in range(dyld._dyld_image_count())}
    mappings = (line.split(maxsplit=5) for line in Path("/proc/self/maps").read_text().splitlines())
    return {Path(fields[5]).resolve() for fields in mappings
            if len(fields) == 6 and fields[5].startswith("/")}


def event_rows(text):
    assert "Hard input: 10/10 events" in text, text
    rows = [line for line in text.splitlines() if line.startswith("Event ")]
    assert len(rows) == 10, text
    assert any(not row.endswith("[]") for row in rows), text
    return rows


def main():
    registry = get_registry_path()
    assert not registry.exists()
    hepyy.load(["jewel_cpp", "fastjet"])
    import jewel_cpp

    assert jewel_cpp.EventGenerator(jewel_cpp.EventGeneratorConfig()).next().accepted
    lib_paths = mapped_libraries()
    for package, library in (("pythia8", "pythia8"), ("lhapdf", "LHAPDF"), ("fastjet", "fastjet")):
        record = wheel_record(package)
        filename = record["library_files"][library]
        expected = (Path(record["lib_dir"]) / filename).resolve()
        loaded = {path for path in lib_paths if path.name == filename}
        assert loaded == {expected}, (expected, loaded)
        print(f"{package} loaded from {expected}")
    pythia_xml = Path(os.environ["PYTHIA8DATA"], "Index.xml").resolve()
    assert pythia_xml.is_file() and pythia_xml.is_relative_to(Path(wheel_record("pythia8")["prefix"]))
    assert not registry.exists()

    examples = Path(wheel_record("jewel_cpp")["prefix"]) / "share/jewel-cpp/examples"
    script_output = subprocess.check_output(
        [sys.executable, str(examples / "jewel_fastjet.py"), "--events", "10"], text=True,
    )
    notebook = nbformat.read(examples / "jewel_fastjet.ipynb", as_version=4)
    executed = NotebookClient(notebook, kernel_name="jewel-wheel-test", timeout=180).execute()
    notebook_output = "".join(
        output.text for cell in executed.cells if cell.cell_type == "code"
        for output in cell.outputs if output.output_type == "stream"
    )
    assert event_rows(script_output) == event_rows(notebook_output)
    print("FastJet script and notebook agree for all 10 events.")


if __name__ == "__main__":
    main()
