"""Build a local hepyy-jewel-cpp wheel against the active hepbundle wheels."""

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import sysconfig
import tempfile

from hepyy.native import wheel_record
from wheel.wheelfile import WheelFile


ROOT = Path(__file__).resolve().parents[1]
PROJECT_VERSION = "0.2.0"
WHEEL_VERSION = f"{PROJECT_VERSION}.1"
DIST_NAME = "hepyy-jewel-cpp"
PAYLOAD = "hepyy_jewel_cpp"
DEPENDENCIES = ("lhapdf", "pythia8")


def dependency_records():
    records = {}
    for name in DEPENDENCIES:
        record = wheel_record(name)
        if record is None:
            raise SystemExit(f"Install the hepyy-{name} wheel in the build environment first.")
        records[name] = record
    return records


def build_payload(work, prefix, deps, jobs):
    env = os.environ.copy()
    build = work / "build"
    flags = [f"-DJEWEL_{name.upper()}_ROOT={rec['prefix']}" for name, rec in deps.items()]
    platform_tag = sysconfig.get_platform().replace("-", "_").replace(".", "_")
    if sys.platform == "darwin":
        for key in ("CXXFLAGS", "CPPFLAGS", "LDFLAGS"):
            env.pop(key, None)
        clang = subprocess.check_output(["xcrun", "--find", "clang++"], text=True).strip()
        target = "11.0"
        flags += [f"-DCMAKE_CXX_COMPILER={clang}", f"-DCMAKE_OSX_DEPLOYMENT_TARGET={target}"]
        platform_tag = f"macosx_{target.replace('.', '_')}_{platform.machine()}"
    elif sys.platform != "linux":
        raise SystemExit("Local wheel builds are supported on macOS and Linux.")

    subprocess.run([
        "cmake", "-S", str(ROOT), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_INSTALL_PREFIX={prefix}", "-DBUILD_TESTING=OFF",
        "-DJEWEL_WITH_PYTHIA8=ON", "-DJEWEL_WITH_CPPYY=ON",
        "-DJEWEL_CPPYY_WHEEL=ON", *flags,
    ], env=env, check=True)
    subprocess.run([
        "cmake", "--build", str(build), "--target", "jewel_generator", "--parallel", str(jobs),
    ], env=env, check=True)
    subprocess.run(["cmake", "--install", str(build)], env=env, check=True)

    examples = prefix / "share/jewel-cpp/examples"
    examples.mkdir(parents=True)
    for filename in ("jewel_fastjet.py", "jewel_fastjet.ipynb"):
        shutil.copy2(ROOT / "examples" / filename, examples / filename)
    shutil.copy2(ROOT / "README.md", examples.parent / "README.md")
    return f"py3-none-{platform_tag}"


def write_wheel(output, prefix, tag, deps):
    manifest = dict(
        schema_version=1, name="jewel_cpp", version=PROJECT_VERSION,
        distribution_version=WHEEL_VERSION, cppyy_namespace="jewel",
        headers=["jewel/event_generator.hpp"], libraries=["jewel_generator"],
        depends_on=list(deps), abi=f"jewel-cpp-{WHEEL_VERSION}-cxx17",
        requires_native={name: rec["abi"] for name, rec in deps.items()},
    )
    info = f"{PAYLOAD}-{WHEEL_VERSION}.dist-info"
    licenses = ("LICENSE", "NOTICE", "GUIDELINES")
    metadata = (
        f"Metadata-Version: 2.4\nName: {DIST_NAME}\nVersion: {WHEEL_VERSION}\n"
        "Requires-Python: >=3.10\n"
        "Summary: Native JEWEL C++ generator for hepyy/cppyy\n"
        "License-Expression: GPL-2.0-or-later\n"
    )
    metadata += "".join(f"License-File: {name}\n" for name in licenses)
    metadata += "".join(
        f"Requires-Dist: hepyy-{name}=={rec['distribution_version']}\n"
        for name, rec in deps.items()
    )
    path = output / f"{PAYLOAD}-{WHEEL_VERSION}-{tag}.whl"
    with WheelFile(path, "w") as wheel:
        wheel.writestr(f"{info}/METADATA", metadata + "\n")
        wheel.writestr(f"{info}/WHEEL", f"Wheel-Version: 1.0\nRoot-Is-Purelib: false\nTag: {tag}\n")
        wheel.writestr(f"{PAYLOAD}/manifest.json", json.dumps(manifest, indent=2) + "\n")
        for name in licenses:
            wheel.write(ROOT / name, f"{info}/licenses/{name}")
        for item in sorted(prefix.rglob("*")):
            if item.is_file():
                wheel.write(item, f"{PAYLOAD}/{item.relative_to(prefix).as_posix()}")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    deps = dependency_records()
    with tempfile.TemporaryDirectory(prefix="jewel-wheel-") as tmp:
        work = Path(tmp)
        prefix = work / "payload"
        tag = build_payload(work, prefix, deps, args.jobs)
        print(write_wheel(args.output, prefix, tag, deps))


if __name__ == "__main__":
    main()
