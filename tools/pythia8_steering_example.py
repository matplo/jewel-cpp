"""Compare the JEWEL vacuum cascade with standalone Pythia8 for matched HardQCD."""

import argparse
import hashlib
import json
import math
from pathlib import Path

from jet_raa import spectrum


JEWEL_SCHEMA = "jewel-hadron-development-v1"
PYTHIA_SCHEMA = "pythia8-standalone-hadron-v1"
PT_EDGES = (40, 50, 60, 80, 100, 125, 150, 200)
HADRON_SETTINGS = ("ColourReconnection:", "String", "ParticleDecays:", "111:mayDecay =", "HadronLevel:Decay =")


def initialize(pythia8, settings):
    engine = pythia8.Pythia()
    for setting in settings:
        if not engine.readString(setting):
            raise ValueError(f"Pythia8 rejected {setting}")
    if not engine.init():
        raise RuntimeError("Pythia8 initialization failed")
    return engine


def summary(engine, attempts, accepted):
    info = engine.info
    return dict(type="summary", attempts=attempts, accepted=accepted, evolution_failures=0,
                generation_failures=attempts-accepted, accepted_weight=info.weightSum(),
                generator_weight_sum=info.weightSum(), generator_sigma_mb=info.sigmaGen(),
                generator_sigma_error_mb=info.sigmaErr())


def check_hard_input(pythia8, source, run):
    engine = initialize(pythia8, run["pythia_settings"])
    version = round(engine.settings.parm("Pythia:versionNumber") * 1000)
    if version != run["pythia_version"]:
        raise ValueError("standalone Pythia8 and the JEWEL runner must use the same Pythia8 version")
    accepted = comparisons = additional_mpi = 0
    for attempt in range(1, run["attempts_requested"]+1):
        row = json.loads(next(source))
        success = engine.next()
        if row["attempt"] != attempt or success != (row["type"] != "generation_failure"):
            raise ValueError(f"hard-input acceptance differs at attempt {attempt}")
        if success:
            accepted += 1
            additional_mpi += engine.info.nMPI() > 1
            if row["weight"] != engine.info.weight():
                raise ValueError(f"hard-input event weight differs at attempt {attempt}")
            if row["type"] == "event":
                if row["pt_hat"] != engine.info.pTHat():
                    raise ValueError(f"hard-input pThat differs at attempt {attempt}")
                comparisons += 1
    jewel = json.loads(next(source))
    check = summary(engine, run["attempts_requested"], accepted)
    keys = ("generator_sigma_mb", "generator_sigma_error_mb", "generator_weight_sum", "generation_failures")
    if any(jewel[k] != check[k] for k in keys):
        raise ValueError("hard-input cross section, weight sum or failure count differs")
    return dict(exact=True, weight_comparisons=accepted, pthat_comparisons=comparisons,
                events_with_additional_mpi=additional_mpi, summary=check)


def generate(jewel_file, output, seed):
    if output.exists():
        raise FileExistsError(output)
    import hepyy
    hepyy.load("pythia8")
    import pythia8

    digest = hashlib.sha256()
    with jewel_file.open("rb") as source:
        def hashed_lines():
            for line in source:
                digest.update(line)
                yield line
        lines = hashed_lines()
        run = json.loads(next(lines))
        if (run["schema"] != JEWEL_SCHEMA or run["mode"] != "vacuum" or run.get("native_mpi")
                or run.get("subtraction") or seed == run["seed"]):
            raise ValueError("use a JEWEL vacuum run with Pythia8 hard input and an independent jet-sample seed")
        check = check_hard_input(pythia8, lines, run)
        if next(lines, None) is not None:
            raise ValueError("unexpected data after the JEWEL summary")

    replacements = {"PartonLevel:FSR": "on", "HadronLevel:all": "on", "Random:seed": str(seed)}
    settings = [f"{key} = {replacements.get(key, value)}"
                for key, value in (line.split(" = ", 1) for line in run["pythia_settings"])]
    settings.extend(line for line in run["hadronizer_settings"] if line.startswith(HADRON_SETTINGS))
    engine = initialize(pythia8, settings)
    attempts = run["attempts_requested"]
    accepted = additional_mpi = 0
    with output.open("x") as stream:
        def write(record):
            stream.write(json.dumps(record, allow_nan=False, separators=(",", ":")) + "\n")

        write(dict(type="run", schema=PYTHIA_SCHEMA, mode="vacuum", pythia_version=run["pythia_version"],
                   attempts_requested=attempts, seed=seed, pythia_settings=settings,
                   hard_input_check=check, matched_jewel_sha256=digest.hexdigest()))
        for attempt in range(1, attempts+1):
            if not engine.next():
                write(dict(type="generation_failure", attempt=attempt, reason="Pythia8 next() failed"))
                continue
            accepted += 1
            additional_mpi += engine.info.nMPI() > 1
            particles = []
            for i in range(engine.event.size()):
                particle = engine.event[i]
                if particle.isFinal():
                    particles.append(dict(pdg=particle.id(), final=True,
                                          momentum=[particle.px(), particle.py(), particle.pz(), particle.e()]))
            write(dict(type="event", attempt=attempt, weight=engine.info.weight(),
                       pt_hat=engine.info.pTHat(), hadronized_particles=particles))
        write(dict(summary(engine, attempts, accepted), events_with_additional_mpi=additional_mpi))
    print(f"{output}: exact hard-input check passed, standalone accepted {accepted}/{attempts}")


def jet_means(sample):
    events = []
    for event in sample["events"]:
        jets = [jet for jet in event["jets"] if 60 <= jet["pt"] < 100]
        events.append((event["weight"], jets))
    denominator = math.fsum(w * len(jets) for w, jets in events)
    if denominator <= 0:
        raise ValueError("no weighted jets in the 60–100 GeV comparison interval")
    result = {}
    for key in ("mass", "ptd"):
        mean = math.fsum(w * jet[key] for w, jets in events for jet in jets) / denominator
        variance = math.fsum((w * math.fsum(jet[key] - mean for jet in jets) / denominator)**2
                             for w, jets in events)
        result[key] = dict(mean=mean, standard_error=math.sqrt(variance * len(events) / (len(events)-1)))
    result["raw_jets"] = sum(len(jets) for _, jets in events)
    return result


def compare(jet_file, output):
    analysis = json.loads(jet_file.read_text())
    if len(analysis["samples"]) != 4:
        raise ValueError("supply JEWEL then standalone Pythia8 for each of two MPI settings")
    comparisons = []
    for index, mpi in enumerate((False, True)):
        jewel, pythia = analysis["samples"][2*index:2*index+2]
        run = pythia["run"]
        check = run["hard_input_check"]
        if (jewel["run"]["schema"] != JEWEL_SCHEMA or run["schema"] != PYTHIA_SCHEMA
                or run["matched_jewel_sha256"] != jewel["input_sha256"] or not check["exact"]
                or (("PartonLevel:MPI = on" in run["pythia_settings"]) != mpi)):
            raise ValueError("expected independently generated matched ISR-only then ISR+MPI pairs")
        _, j_bins = spectrum(jewel, PT_EDGES)
        _, p_bins = spectrum(pythia, PT_EDGES)
        bins = []
        for lo, hi, j, p in zip(PT_EDGES, PT_EDGES[1:], j_bins, p_bins):
            numerator, denominator = (b["dsigma_mb_per_GeV"] for b in (j, p))
            ratio = numerator / denominator if numerator > 0 and denominator > 0 else None
            errors = []
            if ratio is not None:
                for key in ("spectrum_standard_error_mb_per_GeV", "generator_sigma_error_mb_per_GeV"):
                    errors.append(math.hypot(j[key], ratio*p[key]) / denominator)
            bins.append(dict(pt_GeV=[lo,hi], jewel=j, pythia8=p, ratio=ratio,
                             ratio_standard_error=math.hypot(*errors) if errors else None))
        comparisons.append(dict(pythia8_mpi=mpi, hard_input_check=check,
                                jewel=dict(seed=jewel["run"]["seed"], input_sha256=jewel["input_sha256"],
                                           settings=jewel["run"]["pythia_settings"],
                                           hadronizer_settings=jewel["run"]["hadronizer_settings"],
                                           summary=jewel["summary"], means_60_100_GeV=jet_means(jewel)),
                                pythia8=dict(seed=run["seed"], input_sha256=pythia["input_sha256"],
                                             settings=run["pythia_settings"],
                                             summary=pythia["summary"], means_60_100_GeV=jet_means(pythia)),
                                jet_spectrum=bins))
    report = dict(pythia8_version=analysis["samples"][0]["run"]["pythia_version"],
                  selection=analysis["selection"], analysis_versions=analysis["analysis_versions"],
                  normalization="Generated hard cross section times weighted jets divided by generated weight and bin width.",
                  uncertainties="Event-level errors and independent generator cross-section errors.",
                  comparisons=comparisons)
    with output.open("x") as stream:
        json.dump(report, stream, indent=2, allow_nan=False)
        stream.write("\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    run = sub.add_parser("generate")
    run.add_argument("jewel", type=Path)
    run.add_argument("output", type=Path)
    run.add_argument("--seed", type=int, required=True)
    comparison = sub.add_parser("compare")
    comparison.add_argument("jets", type=Path)
    comparison.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.action == "generate":
        if not 1 <= args.seed <= 900000000:
            parser.error("--seed must be in [1, 900000000]")
        generate(args.jewel, args.output, args.seed)
    else:
        compare(args.jets, args.output)


if __name__ == "__main__":
    main()
