"""Launch the wheel's native event runner with its dependency data paths."""

from importlib.metadata import distribution
import os
from pathlib import Path
import sys


PACKAGE_ROOT = Path(__file__).resolve().parent
NATIVE_RUNNER = PACKAGE_ROOT / "bin/jewel-run-events"
ANALYSIS_TOOLS = PACKAGE_ROOT / "share/jewel-cpp/tools"


def main():
    pythia_data = distribution("hepyy-pythia8").locate_file("hepyy_pythia8/share/Pythia8/xmldoc")
    lhapdf_data = distribution("hepyy-lhapdf").locate_file("hepyy_lhapdf/share/LHAPDF")
    env = os.environ.copy()
    env.setdefault("PYTHIA8DATA", str(pythia_data))
    pdf_paths = env.get("LHAPDF_DATA_PATH", "").split(os.pathsep)
    if str(lhapdf_data) not in pdf_paths:
        pdf_paths.append(str(lhapdf_data))
    env["LHAPDF_DATA_PATH"] = os.pathsep.join(path for path in pdf_paths if path)
    os.execve(NATIVE_RUNNER, ["jewel-run-events", *sys.argv[1:]], env)
