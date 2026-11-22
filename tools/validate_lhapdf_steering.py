"""Check hard-process PDF steering against LHAPDF and compare the free-proton limit."""

import argparse
import json
import math
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory


PROTON = "CT18ANLO"
NUCLEUS = "EPPS21nlo_CT18Anlo_Pb208"
CENTRAL = f"{PROTON}/0"
CASES = (
    ("free-proton", CENTRAL, None, None),
    ("free-proton-hard-override", CENTRAL, CENTRAL, CENTRAL),
    ("proton-member", f"{PROTON}/1", None, None),
    ("proton-set", "CT14nlo/0", None, None),
    ("lead-a", CENTRAL, f"{NUCLEUS}/0", None),
    ("lead-b", CENTRAL, None, f"{NUCLEUS}/0"),
    ("lead-central", CENTRAL, f"{NUCLEUS}/0", f"{NUCLEUS}/0"),
    ("lead-member", CENTRAL, f"{NUCLEUS}/1", f"{NUCLEUS}/1"),
)


def simulate(runner, filename, attempts, base, hard_a, hard_b):
    args = [str(runner), str(filename), str(attempts), "2601101",
            "--ecm", "5020", "--pt-min", "50", "--pt-max", "150",
            "--isr", "on", "--lhapdf", base, "--hard-alpha-s", ".118",
            "--hard-alpha-s-order", "2", "--isr-alpha-s", ".118", "--isr-alpha-s-order", "2"]
    for side, pdf in (("a", hard_a), ("b", hard_b)):
        if pdf:
            args.extend((f"--lhapdf-hard-{side}", pdf))
    run = subprocess.run(args, capture_output=True, text=True)
    if run.returncode:
        raise RuntimeError(f"{filename.stem} failed\n{run.stdout}{run.stderr}")
    with filename.open() as source:
        header = json.loads(next(source))
        records = [json.loads(line) for line in source]
    events, summary = records[:-1], records[-1]
    if len(events) != attempts or summary["type"] != "summary":
        raise ValueError(f"incomplete attempt ledger from {filename.stem}")
    return header, events, summary


def compare_densities(events, expected):
    maximum = 0.
    checked = 0
    for event in events:
        if event["type"] != "event":
            continue
        if len(event["hard_pdfs"]) != len(expected):
            raise ValueError("missing beam PDF samples")
        for observed, pdf in zip(event["hard_pdfs"], expected):
            value = pdf.xfxQ(observed["flavour"], observed["x"], event["q_factorization"])
            if not math.isclose(observed["xf"], value, rel_tol=2e-11, abs_tol=1e-12):
                raise ValueError(f"hard PDF mismatch at attempt {event['attempt']}: {observed['xf']} vs {value}")
            maximum = max(maximum, abs(observed["xf"] - value))
            checked += 1
    if not checked:
        raise ValueError("no generated PDF samples to validate")
    return checked, maximum


def validate(runner, attempts):
    import hepyy
    hepyy.load("lhapdf")
    import lhapdf

    lhapdf.setVerbosity(0)
    names = {name for _, *pdfs in CASES for name in pdfs if name}
    grids = {}
    for name in sorted(names):
        set_name, member = name.split("/")
        grids[name] = lhapdf.mkPDF(set_name, int(member))
    baseline = None
    rows = []
    with TemporaryDirectory(prefix="jewel-lhapdf-") as directory:
        for label, base, hard_a, hard_b in CASES:
            hard = [hard_a or base, hard_b or base]
            header, events, summary = simulate(runner, Path(directory) / f"{label}.jsonl",
                                               attempts, base, hard_a, hard_b)
            checked, max_diff = compare_densities(events, [grids[name] for name in hard])
            if label == "free-proton":
                baseline = events, summary
            if label == "free-proton-hard-override" and (events, summary) != baseline:
                raise ValueError("explicit free-proton hard PDFs changed the generated events or cross section")
            rows.append(dict(case=label, base_pdf=base, hard_pdf_a=hard[0], hard_pdf_b=hard[1],
                             compared_pdf_values=checked, maximum_absolute_xf_difference=max_diff,
                             summary=summary))
            print(f"{label}: {checked} PDF values verified")
    return dict(pythia_version=header["pythia_version"], lhapdf_version=str(lhapdf.version()), attempts_per_case=attempts,
                ecm_GeV=5020, pthat_GeV=[50, 150], alpha_s_mz=.118, alpha_s_order=2,
                seed=2601101, isr=True, mpi=False, free_proton_limit_exact=True, cases=rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runner", type=Path, help="jewel-pythia8-input executable")
    parser.add_argument("--attempts", type=int, default=1000)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.attempts < 1:
        parser.error("--attempts must be positive")
    if args.output.exists():
        parser.error("output already exists")
    report = validate(args.runner.resolve(), args.attempts)
    with args.output.open("x") as output:
        json.dump(report, output, indent=2, allow_nan=False)
        output.write("\n")


if __name__ == "__main__":
    main()
