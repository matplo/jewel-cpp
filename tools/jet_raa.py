"""Normalize two native jet samples from analyze_native_events.py to jet R_AA."""

import argparse
from bisect import bisect_right
import json
import math
from pathlib import Path


PT_EDGES = (10, 20, 30, 40, 60, 80, 100, 150, 250)
CLOSURE_MIN_JETS = 50
CLOSURE_MAX_PULL = 3


def require(condition, message):
    if not condition:
        raise ValueError(message)


def matched_settings(run, key):
    return [setting for setting in run[key] if not setting.startswith("Random:seed = ")]


def validate_pair(pp, aa):
    pp_run, aa_run = (sample["run"] for sample in (pp, aa))
    for sample in (pp, aa):
        require(
            sample["run"]["schema"] == "jewel-hadron-development-v1"
            and sample["particle_level"] == "hadron"
            and sample["denominator_scope"] == "attempted-events",
            "R_AA requires native hadron events with a complete attempt ledger",
        )
    require(pp_run["mode"] == "vacuum" and pp_run["centrality_percent"] is None,
            "the first input sample must be pp vacuum")
    centrality = aa_run["centrality_percent"]
    require(aa_run["mode"] in {"medium", "zero-interaction"}
            and centrality is not None and len(centrality) == 2
            and 0 <= centrality[0] < centrality[1] <= 100,
            "the second input sample must have explicit PbPb centrality")
    require(pp_run["seed"] != aa_run["seed"], "use independent sample seeds for the error estimate")
    for key in ("pythia_settings", "hadronizer_settings"):
        require(matched_settings(pp_run, key) == matched_settings(aa_run, key), f"unmatched {key}")
    for key in ("pythia_version", "hadrotype", "native_mpi", "mpi_remnant_policy", "mpi_production_pt0_gev",
                "table_energy_gev", "angular_ordering", "first_angle_ordering", "coherence",
                "compression", "keep_recoils", "recoil_rescattering", "recoil_soft_cut_3T",
                "recoil_hard_cut_3T", "longitudinal_scatterer_boost", "subtraction"):
        require(pp_run.get(key) == aa_run.get(key), f"unmatched {key}")


def spectrum(sample, edges):
    summary = sample["summary"]
    records = sample["events"]
    require(len(records) == summary["attempts"] == sample["run"]["attempts_requested"],
            "analysis must retain all attempt records")
    sigma = summary["generator_sigma_mb"]
    sigma_error = summary["generator_sigma_error_mb"]
    require(math.isfinite(sigma) and sigma > 0 and math.isfinite(sigma_error) and sigma_error >= 0,
            "invalid Pythia generator cross section or uncertainty")
    weights, accepted_weights, counts_by_event = [], [], []
    accepted_count = 0
    for attempt, event in enumerate(records, 1):
        require(event["attempt"] == attempt, "incomplete or out-of-order attempt ledger")
        weight = event["weight"]
        require(math.isfinite(weight) and weight >= 0, "invalid attempt weight")
        counts = [0] * (len(edges) - 1)
        if event["accepted"]:
            require(weight > 0, "accepted event has zero weight")
            accepted_count += 1
            accepted_weights.append(weight)
            for jet in event["jets"]:
                pt = jet["pt"]
                require(math.isfinite(pt), "nonfinite jet pT")
                index = bisect_right(edges, pt) - 1
                if 0 <= index < len(counts):
                    counts[index] += 1
        else:
            require(not event["jets"], "failed attempt has selected jets")
        if weight > 0:
            weights.append(weight)
            counts_by_event.append(counts)
    total_weight = math.fsum(weights)
    accepted_weight = math.fsum(accepted_weights)
    require(accepted_count == summary["accepted"] and len(weights) > 1
            and len(weights) - accepted_count == summary["evolution_failures"]
            and len(records) - len(weights) == summary["generation_failures"]
            and math.isclose(total_weight, summary["generator_weight_sum"], rel_tol=1e-12)
            and math.isclose(accepted_weight, summary["accepted_weight"], rel_tol=1e-12),
            "attempt weights or counts disagree with the generator summary")
    bins = []
    for index, (lo, hi) in enumerate(zip(edges, edges[1:])):
        pairs = list(zip(weights, (counts[index] for counts in counts_by_event)))
        weighted_count = math.fsum(w * n for w, n in pairs)
        jets_per_weight = weighted_count / total_weight
        variance = math.fsum((w * (n - jets_per_weight) / total_weight)**2 for w, n in pairs)
        stat = math.sqrt(variance * len(pairs) / (len(pairs) - 1))
        width = hi - lo
        bins.append(dict(
            raw_jets=sum(n for _, n in pairs),
            weighted_jets=weighted_count,
            dsigma_mb_per_GeV=sigma * jets_per_weight / width,
            spectrum_standard_error_mb_per_GeV=sigma * stat / width,
            generator_sigma_error_mb_per_GeV=sigma_error * jets_per_weight / width,
        ))
    metadata = dict(input_sha256=sample["input_sha256"], run=sample["run"], summary=summary,
                    selected_weight_fraction=accepted_weight / total_weight)
    return metadata, bins


def closure_status(bins):
    pulls = []
    inconsistent_zero_error = False
    for row in bins:
        if min(row["pp"]["raw_jets"], row["pbpb"]["raw_jets"]) < CLOSURE_MIN_JETS:
            row["no_quenching_pull"] = None
            continue
        delta = row["raa"] - 1
        error = row["raa_standard_error"]
        row["no_quenching_pull"] = delta / error if error > 0 else None
        if error > 0:
            pulls.append(abs(row["no_quenching_pull"]))
        elif not math.isclose(delta, 0, abs_tol=1e-12):
            inconsistent_zero_error = True
    return dict(min_raw_jets_per_sample=CLOSURE_MIN_JETS, maximum_allowed_abs_pull=CLOSURE_MAX_PULL,
                bins_checked=len(pulls), max_abs_pull=max(pulls, default=None),
                passed=not inconsistent_zero_error and len(pulls) >= 3 and max(pulls) <= CLOSURE_MAX_PULL)


def calculate(analysis, taa, taa_error, taa_source, edges, check_closure=False):
    require(math.isfinite(taa) and taa > 0 and math.isfinite(taa_error) and taa_error >= 0,
            "T_AA must be positive with a nonnegative uncertainty")
    require(taa_source.strip(), "record the source of T_AA")
    require(len(analysis["samples"]) == 2, "supply exactly two samples, pp first")
    require(len(edges) >= 2 and all(map(math.isfinite, edges))
            and all(a < b for a, b in zip(edges, edges[1:]))
            and edges[0] >= analysis["selection"]["min_pt_GeV"],
            "pT edges must increase and lie above the analysis jet threshold")
    pp, aa = analysis["samples"]
    validate_pair(pp, aa)
    require(not check_closure or aa["run"]["mode"] == "zero-interaction",
            "closure requires a zero-interaction PbPb sample")
    pp_metadata, pp_bins = spectrum(pp, edges)
    aa_metadata, aa_bins = spectrum(aa, edges)
    bins = []
    for lo, hi, pp_bin, aa_bin in zip(edges, edges[1:], pp_bins, aa_bins):
        pp_xsec, aa_xsec = (b["dsigma_mb_per_GeV"] for b in (pp_bin, aa_bin))
        yield_aa = taa * aa_xsec
        ratio = yield_aa / (taa * pp_xsec) if pp_xsec > 0 else None
        components = {}
        for key in ("spectrum_standard_error_mb_per_GeV", "generator_sigma_error_mb_per_GeV"):
            components[key] = (math.hypot(aa_bin[key], ratio * pp_bin[key]) / pp_xsec
                               if ratio is not None and aa_bin["raw_jets"] > 0 else None)
        bins.append(dict(
            pt_low_GeV=lo, pt_high_GeV=hi, pp=pp_bin,
            pbpb=dict(raw_jets=aa_bin["raw_jets"], weighted_jets=aa_bin["weighted_jets"],
                      nn_equivalent_dsigma_mb_per_GeV=aa_xsec, yield_per_event_per_GeV=yield_aa,
                      spectrum_standard_error_per_event_per_GeV=taa * aa_bin["spectrum_standard_error_mb_per_GeV"],
                      generator_sigma_error_per_event_per_GeV=taa * aa_bin["generator_sigma_error_mb_per_GeV"],
                      taa_error_per_event_per_GeV=taa_error * aa_xsec),
            raa=ratio, raa_spectrum_standard_error=components["spectrum_standard_error_mb_per_GeV"],
            raa_generator_sigma_error=components["generator_sigma_error_mb_per_GeV"],
            raa_standard_error=math.hypot(*components.values()) if components["spectrum_standard_error_mb_per_GeV"] is not None else None,
        ))
    return dict(
        selection=analysis["selection"], analysis_versions=analysis["analysis_versions"],
        generator=analysis.get("generator"),
        centrality_percent=aa["run"]["centrality_percent"],
        normalization="Pythia sigmaGen times weighted jets divided by full generated weight and bin width. "
                      "The PbPb per-event yield uses binary scaling with a constant class mean T_AA.",
        uncertainty="Event-level weighted spectrum errors. Pythia cross-section errors use independent "
                    "first-order propagation with covariance neglected. The common T_AA cancels in model R_AA. Ratio errors are omitted if either sample has no jets in a bin.",
        taa=dict(value_mb_inverse=taa, uncertainty_mb_inverse=taa_error, source=taa_source),
        pp=pp_metadata, pbpb=aa_metadata, bins=bins,
        no_quenching_closure=closure_status(bins) if check_closure else None,
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("analysis", type=Path, help="jet analysis JSON with vacuum and PbPb samples, in that order")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--taa", type=float, required=True, help="class-averaged T_AA in inverse mb")
    parser.add_argument("--taa-error", type=float, default=0.0, help="T_AA uncertainty in inverse mb")
    parser.add_argument("--taa-source", required=True, help="citation or provenance for the input T_AA")
    parser.add_argument("--pt-edges", nargs="+", type=float, default=PT_EDGES)
    parser.add_argument("--validate-no-quenching", action="store_true", help="require statistical closure for zero-interaction PbPb")
    args = parser.parse_args()
    try:
        report = calculate(json.loads(args.analysis.read_text()), args.taa, args.taa_error,
                           args.taa_source, args.pt_edges, args.validate_no_quenching)
    except (KeyError, TypeError, ValueError) as error:
        parser.error(str(error))
    with args.output.open("x") as stream:
        json.dump(report, stream, indent=2, allow_nan=False)
        stream.write("\n")
    if args.validate_no_quenching:
        status = report["no_quenching_closure"]
        print(f"Closure {'passed' if status['passed'] else 'failed'}: {status['bins_checked']} bins, "
              f"largest absolute pull {status['max_abs_pull']}")
        return 0 if status["passed"] else 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
