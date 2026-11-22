"""Compare Soft Drop in matched pp and 0–10% Pb–Pb at 5.02 TeV."""

import argparse
from bisect import bisect_right
import json
import math
from pathlib import Path

from jet_raa import require, spectrum, validate_pair


RG_EDGES = (0., .02, .1, .2, .4)
ZG_EDGES = (.2, .25, .3, .35, .4, .45, .5)


def ratio(numerator, denominator, error_key, value_key):
    den = denominator[value_key]
    num = numerator[value_key]
    if den is None or num is None or den <= 0:
        return None, None
    value = num / den
    error = (math.hypot(numerator[error_key], value * denominator[error_key]) / den
             if num > 0 else None)
    return value, error


def tagged_distribution(sample, pt_low, pt_high, key, edges):
    weighted_events = []
    selected = tagged = outside = 0
    selected_weight = tagged_weight = 0.
    for event in sample["events"]:
        weight = event["weight"]
        counts = [0] * (len(edges) - 1)
        for jet in event["jets"]:
            if not pt_low <= jet["pt"] < pt_high:
                continue
            selected += 1
            selected_weight += weight
            if not jet["soft_drop_tagged"]:
                continue
            tagged += 1
            tagged_weight += weight
            if jet["rg"] >= RG_EDGES[-1]:
                outside += 1
                continue
            # Include z_g=0.5, the endpoint for a symmetric splitting.
            index = min(bisect_right(edges, jet[key]) - 1, len(counts) - 1)
            require(index >= 0, f"{key} below the first bin edge")
            counts[index] += 1
        if weight > 0:
            weighted_events.append((weight, sum(counts), counts))
    total = math.fsum(w * n for w, n, _ in weighted_events)
    bins = []
    for i, (lo, hi) in enumerate(zip(edges, edges[1:])):
        width = hi - lo
        fraction = math.fsum(w * counts[i] for w, _, counts in weighted_events) / total if total else None
        error = None
        if fraction is not None:
            size = len(weighted_events)
            variance = math.fsum((w * (counts[i] - fraction * n) / total)**2
                                 for w, n, counts in weighted_events)
            error = math.sqrt(variance * size / (size - 1)) / width
        bins.append(dict(low=lo, high=hi,
                         raw_jets=sum(counts[i] for _, _, counts in weighted_events),
                         density=fraction / width if fraction is not None else None,
                         standard_error=error))
    return dict(raw_selected=selected, raw_tagged=tagged, raw_tagged_rg_overflow=outside,
                weighted_tagged_fraction=tagged_weight / selected_weight if selected_weight else None,
                bins=bins)


def rg_spectrum(sample, pt_low, pt_high, edges):
    def bin_index(jet):
        if not pt_low <= jet["pt"] < pt_high:
            return None
        if not jet["soft_drop_tagged"]:
            return 0
        return bisect_right(edges, jet["rg"])

    # The untagged category is integrated over r_g, with unit bin width.
    _, bins = spectrum(sample, (-1., *edges), bin_index)
    return bins


def calculate(analysis, pt_edges, rg_edges=RG_EDGES):
    require(len(analysis["samples"]) == 2, "supply pp first and PbPb second")
    pp, aa = analysis["samples"]
    validate_pair(pp, aa)
    require(aa["run"]["centrality_percent"] == [0, 10]
            and "Beams:eCM = 5020" in pp["run"]["pythia_settings"],
            "this example requires 0–10% PbPb and 5.02 TeV")
    glauber = aa["run"].get("glauber")
    require(glauber and glauber["sampling"] == "ncoll-proportional",
            "PbPb must use the Glauber-weighted impact parameter sampler")
    taa = glauber["taa_mb_inverse"]
    require(math.isfinite(taa) and taa > 0, "invalid Glauber T_AA")
    selection = analysis["selection"]
    grooming = selection["soft_drop"]
    require(selection["R"] == .4 and grooming["R0"] == .4
            and grooming["zcut"] == .2 and grooming["beta"] == 0,
            "use anti-kt R=0.4 and analyze with --soft-drop-zcut 0.2")
    require(len(pt_edges) >= 2 and all(map(math.isfinite, pt_edges))
            and pt_edges[0] >= selection["min_pt_GeV"]
            and all(lo < hi for lo, hi in zip(pt_edges, pt_edges[1:])),
            "pT edges must increase and lie above the jet threshold")
    require(len(rg_edges) >= 2 and rg_edges[0] == 0 and rg_edges[-1] == .4
            and all(lo < hi for lo, hi in zip(rg_edges, rg_edges[1:])),
            "r_g edges must increase from 0 to 0.4")
    theta_edges = tuple(edge / .4 for edge in rg_edges)
    intervals = []
    for lo, hi in zip(pt_edges, pt_edges[1:]):
        row = dict(pt_low_GeV=lo, pt_high_GeV=hi)
        rg_rows = []
        pp_rg = rg_spectrum(pp, lo, hi, rg_edges)
        aa_rg = rg_spectrum(aa, lo, hi, rg_edges)
        for index, (vacuum, medium) in enumerate(zip(pp_rg, aa_rg)):
            pp_xsec = vacuum["dsigma_mb_per_GeV"]
            aa_xsec = medium["dsigma_mb_per_GeV"]
            raa = aa_xsec / pp_xsec if pp_xsec > 0 else None
            stat = sigma = None
            if pp_xsec > 0 and aa_xsec > 0:
                stat = math.hypot(medium["spectrum_standard_error_mb_per_GeV"],
                                  raa * vacuum["spectrum_standard_error_mb_per_GeV"]) / pp_xsec
                sigma = math.hypot(medium["generator_sigma_error_mb_per_GeV"],
                                   raa * vacuum["generator_sigma_error_mb_per_GeV"]) / pp_xsec
            rg_rows.append(dict(
                untagged=index == 0,
                low=rg_edges[index - 1] if index else None,
                high=rg_edges[index] if index else None,
                pp_raw_jets=vacuum["raw_jets"], pbpb_raw_jets=medium["raw_jets"],
                pp_cross_section_density_mb_per_GeV=pp_xsec / (hi - lo),
                pbpb_yield_density_per_event_per_GeV=taa * aa_xsec / (hi - lo),
                raa=raa, raa_spectrum_standard_error=stat, raa_generator_sigma_error=sigma,
                raa_standard_error=math.hypot(stat, sigma) if stat is not None else None,
            ))
        row["rg_raa"] = rg_rows
        for key, edges in (("zg", ZG_EDGES), ("theta_g", theta_edges)):
            vacuum = tagged_distribution(pp, lo, hi, key, edges)
            medium = tagged_distribution(aa, lo, hi, key, edges)
            ratios = []
            for vac_bin, med_bin in zip(vacuum["bins"], medium["bins"]):
                value, error = ratio(med_bin, vac_bin, "standard_error", "density")
                ratios.append(dict(low=vac_bin["low"], high=vac_bin["high"],
                                   medium_over_vacuum=value, standard_error=error))
            row[key] = dict(pp=vacuum, pbpb=medium, ratios=ratios)
        intervals.append(row)
    return dict(
        ecm_GeV=5020, centrality_percent=[0, 10], glauber=glauber, selection=selection,
        analysis_versions=analysis["analysis_versions"], generator=analysis.get("generator"),
        samples=[{key: sample[key] for key in ("input_sha256", "run", "summary", "selected_jets")}
                 for sample in (pp, aa)],
        shape_normalization="Each sample has unit density over tagged jets with r_g < 0.4 within each pT interval. "
                            "The weighted tagged fraction includes all tagged jets in the pT interval.",
        rg_normalization="Cross section and PbPb yield are differential in pT and r_g for tagged jets. "
                         "The untagged category is integrated over r_g and differential in pT. "
                         "Glauber sampling applies N_coll once, and the class mean T_AA cancels in R_AA.",
        uncertainty="Event-level weighted errors with shared numerator and denominator within each sample. "
                    "PbPb and pp errors are propagated as independent. Generator cross-section errors are "
                    "separate for R_AA, with covariance neglected. Empty denominator bins have no ratio.",
        pt_intervals=intervals,
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("analysis", type=Path, help="retained-event JSON from analyze_native_events.py")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--pt-edges", nargs="+", type=float, default=(40, 60, 80, 100))
    parser.add_argument("--rg-edges", nargs="+", type=float, default=RG_EDGES)
    args = parser.parse_args()
    try:
        report = calculate(json.loads(args.analysis.read_text()), args.pt_edges, args.rg_edges)
    except (KeyError, TypeError, ValueError) as error:
        parser.error(str(error))
    with args.output.open("x") as stream:
        json.dump(report, stream, indent=2, allow_nan=False)
        stream.write("\n")


if __name__ == "__main__":
    main()
