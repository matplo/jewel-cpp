"""Compare recoil-subtracted JEWEL jets with ATLAS HEPData tables 14 and 18."""

import argparse
from html import escape
from io import StringIO
import json
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator, FuncFormatter, NullFormatter

import jet_raa
import jet_soft_drop


PT_EDGES = (158, 178, 200, 224, 251, 282, 316)
FINE_PT_EDGES = (158, 200, 315)
HIGH_PT_EDGES = (316, 355, 398, 501)
HIGH_FINE_PT_EDGES = (315, 501)
RG_EDGES = (0, .02, .11, .26, .4)
CATEGORIES = ("Inclusive", "Untagged", "0–0.02", "0.02–0.11", "0.11–0.26", "0.26–0.40")
ATLAS = "#202833"
MEDIUM = "#216584"
CONTROL = "#ae5428"


def read_json(path):
    return json.loads(path.read_text())


def atlas_points(table, group):
    for row in table["values"]:
        entry = next(point for point in row["y"] if point["group"] == group)
        errors = {e["label"]: float(e["symerror"]) for e in entry["errors"]}
        yield float(row["x"][0]["value"]), float(entry["value"]), errors["stat"], errors["syst"]


def validate_table(table, number):
    if (table["doi"] != f"10.17182/hepdata.132904.v2/t{number}"
            or table["qualifiers"]["Centrality"][0]["value"] != "0-10%"
            or table["qualifiers"]["SQRT(S)"][0]["value"] != "5020.0 GeV"):
        raise ValueError(f"expected HEPData submission 132904 v2, table {number}")


def model_results(analysis, pt_edges, rg_edges):
    aa = analysis["samples"][1]
    glauber = aa["run"]["glauber"]
    inclusive = jet_raa.calculate(analysis, glauber["taa_mb_inverse"],
                                  glauber["taa_fixed_bin_se_mb_inverse"], glauber["source"], pt_edges)
    substructure = jet_soft_drop.calculate(analysis, pt_edges, rg_edges)["pt_intervals"]
    return [(inc, *sub["rg_raa"]) for inc, sub in zip(inclusive["bins"], substructure)]


def plot_atlas(ax, points):
    x, y, stat, syst = zip(*points)
    ax.errorbar(x, y, yerr=syst, fmt="none", color=ATLAS, alpha=.35, lw=2.5, capsize=5)
    ax.errorbar(x, y, yerr=stat, fmt="o", color=ATLAS, markersize=4, capsize=2, label="ATLAS")


def plot_model(ax, x, bins, color, label, fmt):
    keep = [(coord, b) for coord, b in zip(x, bins)
            if b["raa"] is not None and b["raa_standard_error"] is not None]
    ax.errorbar([x for x, _ in keep], [b["raa"] for _, b in keep],
                yerr=[b["raa_standard_error"] for _, b in keep],
                fmt=fmt, color=color, markersize=5, capsize=3, label=label)


def axes_style(ax):
    ax.set_axisbelow(True)
    ax.grid(axis="y", alpha=.25)
    ax.axhline(1, color="#687883", linestyle="--", lw=.9)
    ax.spines[["top", "right"]].set_visible(False)


def inline_svg(fig, title):
    output = StringIO()
    fig.savefig(output, format="svg", metadata={"Title": title, "Date": None})
    plt.close(fig)
    svg = "<svg" + output.getvalue().split("<svg", 1)[1]
    return "\n".join(line.rstrip() for line in svg.splitlines())


def html_table(headings, rows):
    labels = "".join(f'<th scope="col">{escape(h)}</th>' for h in headings)
    body = "".join("<tr>" + "".join(f"<td>{escape(str(cell))}</td>" for cell in row) + "</tr>" for row in rows)
    return f'<div class="scroll"><table><thead><tr>{labels}</tr></thead><tbody>{body}</tbody></table></div>'


def model_value(b):
    raa, err = b["raa"], b["raa_standard_error"]
    if raa is None:
        return "—"
    return f"{raa:.3f} ± {err:.3f}" if err is not None else f"{raa:.3f} (sparse)"


def jet_count(b, key):
    sample = b.get(key)
    return sample["raw_jets"] if sample is not None else b[f"{key}_raw_jets"]


def hard_window_fractions(sample, pt_bin, hard_bounds):
    lo, hi = pt_bin
    lower_tail, upper_tail = hard_bounds
    weights = [(event["pt_hat_GeV"], event["weight"])
               for event in sample["events"] for jet in event["jets"] if lo <= jet["pt"] < hi]
    total = sum(w for _, w in weights)
    if total <= 0:
        raise ValueError(f"no weighted jets in {lo}–{hi} GeV")
    return (sum(w for pt, w in weights if pt < lower_tail) / total,
            sum(w for pt, w in weights if pt >= upper_tail) / total)


def data_columns(point):
    return tuple(f"{value:.4f}" for value in point[1:])


def counts(subtracted, control):
    return " / ".join(f"{n:,}" for n in (jet_count(subtracted, "pp"),
                                         jet_count(subtracted, "pbpb"), jet_count(control, "pbpb")))


def validate_samples(medium, control, raw, hard_window):
    if (medium["selection"] != control["selection"]
            or medium["samples"][0]["input_sha256"] != control["samples"][0]["input_sha256"]
            or not medium["selection"].get("exclude_thermal_residuals")
            or medium["selection"].get("max_abs_rapidity") != 2.1
            or medium["samples"][1]["run"]["mode"] != "medium"
            or control["samples"][1]["run"]["mode"] != "zero-interaction"
            or medium["samples"][1]["run"]["glauber"] != control["samples"][1]["run"]["glauber"]):
        raise ValueError("expected matched subtracted pp, medium and zero-interaction jet samples")
    raw_selection = dict(raw["selection"], raw_hadrons=False, exclude_thermal_residuals=True)
    if (not raw["selection"].get("raw_hadrons") or raw_selection != medium["selection"]
            or any(a["input_sha256"] != b["input_sha256"] for a, b in zip(raw["samples"], medium["samples"]))):
        raise ValueError("the raw comparison must use the same pp and medium events")
    min_pt_hat, max_pt_hat = hard_window
    for sample in (*medium["samples"], control["samples"][1]):
        run = sample["run"]
        settings = set(run["pythia_settings"])
        expected = {f"PhaseSpace:pTHatMin = {min_pt_hat}", f"PhaseSpace:pTHatMax = {max_pt_hat}",
                    "PhaseSpace:bias2SelectionPow = 5", "PDF:pSet = LHAPDF6:CT14nlo/0",
                    "PartonLevel:ISR = on", "PartonLevel:MPI = off"}
        if (sample["particle_collection"] != "subtracted_particles"
                or not expected.issubset(settings) or run.get("native_mpi") or not run["keep_recoils"]
                or run["subtraction"]["radius"] != .4 or run["subtraction"]["put_on_shell"]
                or "ParticleDecays:tau0Max = 8.99377374" not in run["hadronizer_settings"]):
            raise ValueError(f"expected {min_pt_hat}–{max_pt_hat} GeV ISR-only samples with radius 0.4 subtraction")


def report(medium, control, raw, t14, t18, high_samples=None):
    validate_table(t14, 14)
    validate_table(t18, 18)
    sets = [((medium, control, raw), PT_EDGES, FINE_PT_EDGES, (100, 700), (120, 630))]
    if high_samples is not None:
        if high_samples[0]["selection"] != medium["selection"] or (
                high_samples[0]["samples"][1]["run"]["glauber"] != medium["samples"][1]["run"]["glauber"]):
            raise ValueError("both hard windows must share the jet selection and Glauber class")
        sets.append((high_samples, HIGH_PT_EDGES, HIGH_FINE_PT_EDGES, (150, 1000), (180, 900)))

    # The table publishes arithmetic centers. Work backwards from the upper edge.
    rg_edges = [.4]
    for center, *_ in reversed(list(atlas_points(t18, 1))):
        rg_edges.insert(0, round(2 * center - rg_edges[0], 7))
    assert len(rg_edges) == 13 and rg_edges[0] == .003
    fine_edges = (0., *rg_edges)

    coarse_pt = []
    fine_pt = []
    coarse_results = [[], [], []]
    fine_results = [[], [], []]
    totals = []
    run_rows = []
    for samples, pt_edges, fine_pt_edges, hard_window, tails in sets:
        validate_samples(*samples, hard_window)
        coarse_pt.extend(zip(pt_edges, pt_edges[1:]))
        pt_bins = list(zip(fine_pt_edges, fine_pt_edges[1:]))
        fine_pt.extend(pt_bins)
        coarse = [model_results(sample, pt_edges, RG_EDGES) for sample in samples]
        fine = [model_results(sample, fine_pt_edges, fine_edges) for sample in samples]
        for dest, rows in zip(coarse_results, coarse):
            dest.extend(rows)
        for dest, rows in zip(fine_results, fine):
            dest.extend(rows)
        for (lo, hi), (sub, ctrl, raw_bin) in zip(pt_bins, zip(*fine)):
            totals.append((f"{lo}–{hi}", counts(sub[0], ctrl[0]), model_value(sub[0]),
                           model_value(ctrl[0]), model_value(raw_bin[0])))
        for sample in (*samples[0]["samples"], samples[1]["samples"][1]):
            run, summary = sample["run"], sample["summary"]
            lo_fraction, hi_fraction = hard_window_fractions(sample, pt_bins[-1], tails)
            lo, hi = pt_bins[-1]
            run_rows.append((f"{hard_window[0]}–{hard_window[1]}", run["mode"], run["seed"],
                             summary["attempts"], summary["accepted"], f"{summary['generator_sigma_mb']:.5g}",
                             sample["excluded_thermal_residuals"], f"{lo}–{hi}",
                             f"{lo_fraction:.3%} (<{tails[0]})", f"{hi_fraction:.3%} (≥{tails[1]})"))

    med, ctl, unsubtracted = coarse_results
    fine_med, fine_ctl, fine_raw = fine_results
    plt.rcParams.update({"svg.fonttype": "none", "axes.titleweight": "bold"})
    fig, axes = plt.subplots(2, 3, figsize=(11, 6.3), sharex=True, sharey=True, layout="constrained")
    coarse_rows = []
    centers = [(lo + hi) / 2 for lo, hi in coarse_pt]
    for group, (ax, category) in enumerate(zip(axes.flat, CATEGORIES)):
        axes_style(ax)
        data = list(atlas_points(t14, group))[:len(centers)]
        assert [point[0] for point in data] == centers
        plot_atlas(ax, data)
        plot_model(ax, [x-2 for x in centers], [row[group] for row in med], MEDIUM, "JEWEL subtracted", "o")
        plot_model(ax, [x+2 for x in centers], [row[group] for row in ctl], CONTROL, "JEWEL zero-interaction", "s")
        ticks = [160, 200, 250, 316, 400, 500] if high_samples else [158, 178, 200, 224, 251, 282, 316]
        ax.set(title=category, xticks=ticks)
        if high_samples:
            ax.axvline(HIGH_PT_EDGES[0], lw=.8, color="#d2dbe0")
        ax.tick_params(axis="x", labelsize=8)
        if group >= 3:
            ax.set_xlabel(r"Jet $p_T$ (GeV)")
        if group in (0, 3):
            ax.set_ylabel(r"$R_{\mathrm{AA}}$")
        for i, point in enumerate(data):
            b, c, r = med[i][group], ctl[i][group], unsubtracted[i][group]
            coarse_rows.append((category, f"{coarse_pt[i][0]}–{coarse_pt[i][1]}", *data_columns(point),
                                model_value(b), model_value(c), model_value(r), counts(b,c)))
    axes[0,0].legend(loc="best", fontsize=8)
    coarse_svg = inline_svg(fig, "ATLAS jet momentum comparison in six radius categories")

    fig, axes = plt.subplots(1, len(fine_pt), figsize=(11.8, 4.4), sharey=True, layout="constrained")
    fine_rows = []
    for index, ax in enumerate(axes):
        axes_style(ax)
        lo, hi = fine_pt[index]
        pt_label = f"{lo}–{hi}"
        data = list(atlas_points(t18, index+1))
        assert t18["qualifiers"]["jet pT"][index+1]["value"] == f"{lo} < pT < {hi} GeV"
        centers = [point[0] for point in data]
        assert centers == [point[0] for point in atlas_points(t18, 1)]
        b, c, r = (rows[index][3:] for rows in (fine_med, fine_ctl, fine_raw))
        assert len(centers) == len(b) == len(c) == len(r)
        plot_atlas(ax, data)
        plot_model(ax, [x/1.035 for x in centers], b, MEDIUM, "JEWEL subtracted", "o")
        plot_model(ax, [x*1.035 for x in centers], c, CONTROL, "JEWEL zero-interaction", "s")
        ax.set(xlabel=r"Groomed radius $r_g$", xscale="log", title=f"{pt_label} GeV jets")
        ax.xaxis.set_major_locator(FixedLocator([.003,.01,.03,.1,.3]))
        ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: f"{x:g}"))
        ax.xaxis.set_minor_formatter(NullFormatter())
        for left, right, point, sub, ctrl, raw_bin in zip(rg_edges, rg_edges[1:], data, b, c, r):
            fine_rows.append((pt_label, f"{left:.7g}–{right:.7g}", *data_columns(point),
                              model_value(sub), model_value(ctrl), model_value(raw_bin), counts(sub,ctrl)))
    axes[0].set_ylabel(r"$R_{\mathrm{AA}}$")
    axes[-1].legend(loc="best", fontsize=8)
    fine_svg = inline_svg(fig, "Fine groomed-radius comparison in matched ATLAS jet intervals")

    headers = ["ATLAS RAA", "ATLAS stat.", "ATLAS syst.", "Subtracted RAA", "Control RAA",
               "Raw RAA", "Jets pp / Pb–Pb / control"]
    window_text = ("The 158–315 GeV samples use a 100–700 GeV pThat window. "
                   "The 315–501 GeV samples use a separate 150–1000 GeV window. "
                   if high_samples else "The samples use a 100–700 GeV pThat window. ")
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>ATLAS and JEWEL C++ · Recoil-subtracted Soft Drop jets</title>
<style>
:root{{font:16px/1.55 system-ui,sans-serif;color:#172c3c;background:#f1f5f6}}
body{{max-width:1140px;margin:auto;padding:clamp(1rem,4vw,3rem)}}
header,section{{background:#fff;border:1px solid #d9e3e6;padding:clamp(1rem,3vw,2rem);border-radius:12px;margin-bottom:1rem}}
h1,h2{{line-height:1.15;margin:0 0 1rem}}h1{{font-size:clamp(1.9rem,4vw,2.8rem)}}
h2{{font-size:1.4rem}}figure{{margin:1rem 0}}svg{{display:block;width:100%;height:auto}}
figcaption,summary{{font-size:.9rem;color:#364d5b}}summary{{cursor:pointer;font-weight:650}}
a{{color:#205873}}.scroll{{overflow:auto}}table{{border-collapse:collapse;width:100%;font-variant-numeric:tabular-nums}}
th,td{{text-align:right;padding:.35rem .6rem;border-bottom:1px solid #dce4e8;white-space:nowrap}}
th:first-child,td:first-child{{text-align:left}}
</style></head><body>
<header><h1>Recoil-subtracted Soft Drop jet suppression</h1>
<p>JEWEL C++ and ATLAS · 0–10% Pb–Pb at 5.02 TeV · anti-k<sub>t</sub> R = 0.4 · |y| &lt; 2.1 · z<sub>cut</sub> = 0.2 · β = 0</p>
{html_table(["Jet pT (GeV)", "Jets pp / Pb–Pb / control", "Inclusive subtracted RAA", "Control RAA", "Inclusive raw RAA"],totals)}
<p>Raw and subtracted results use the same event ledgers. Their statistical uncertainties are correlated.</p></header>
<section><h2>Groomed radius at matched jet momentum</h2>
<figure>{fine_svg}<figcaption>ATLAS statistical errors use dark caps, and systematic errors use lighter caps.
JEWEL errors combine event-level and hard-cross-section contributions.
Simulation bin edges are reconstructed from the HEPData arithmetic centers and the upper edge 0.4.
Model points are shifted slightly along the horizontal axis for legibility.</figcaption></figure>
<details><summary>Numerical values, raw comparison and jet counts</summary>{html_table(["Jet pT (GeV)", "r_g", *headers], fine_rows)}</details>
<p><a href="https://doi.org/{t18['doi']}">HEPData Table 18 · ATLAS Figure 11</a></p></section>
<section><h2>Jet momentum in six radius categories</h2>
<figure>{coarse_svg}<figcaption>The HEPData jet momentum bins span 158–{coarse_pt[-1][1]} GeV using boundaries inferred from the published midpoint coordinates.
This figure uses a boundary at 316 GeV. The fine-radius plot uses the 315 GeV boundary quoted in Table 18.
Model markers are offset by 2 GeV for legibility. ATLAS global luminosity and overlap uncertainties are outside the plotted point uncertainties.</figcaption></figure>
<details><summary>Numerical values, raw comparison and jet counts</summary>{html_table(["r_g category", "Jet pT (GeV)", *headers], coarse_rows)}</details>
<p><a href="https://doi.org/{t14['doi']}">HEPData Table 14 · ATLAS Figure 10</a></p></section>
<section><h2>Thermal subtraction and sample definition</h2>
<p>JEWEL hadronizes the scattered recoils and stores their incoming thermal momenta separately.
After hadronization, constituent subtraction removes the incoming thermal transverse momentum and mass contribution from nearby hadrons with a maximum distance ΔR = 0.4. Leftover thermal entries are excluded before jet clustering. The corrected constituents feed both anti-k<sub>t</sub> and Soft Drop. <a href="https://doi.org/10.1140/epjc/s10052-022-10954-1">JEWEL constituent-subtraction method</a></p>
<p>{window_text}Each window has matched pp, medium and control samples with power-5 bias, ISR with MPI disabled, CT14nlo proton PDFs and a 30 ps particle lifetime selection. Jet constituents exclude muons and neutrinos.
The Pb–Pb impact parameters are sampled proportional to N<sub>coll</sub>. The minimum-bias class overlap is {medium['samples'][1]['run']['glauber']['taa_mb_inverse']:.3f} mb⁻¹ and cancels in the model R<sub>AA</sub>.
Each hard window is normalized independently. The weighted pThat tails refer to the subtracted jets in the interval shown for each sample. Failed evolutions retain their generated weight and contribute zero jets.</p>
{html_table(["pThat (GeV)", "Mode", "Seed", "Attempts", "Accepted", "σ gen (mb)", "Excluded thermal entries", "Jet pT (GeV)", "Low pThat tail", "High pThat tail"], run_rows)}
<p><a href="https://arxiv.org/abs/2211.11470">ATLAS publication</a> · <a href="glauber-soft-drop.html">Lower-pT example</a></p></section>
</body></html>
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("medium", "control", "raw", "table14", "table18", "output"):
        parser.add_argument(name, type=Path)
    parser.add_argument("--high-samples", nargs=3, type=Path, metavar=("MEDIUM", "CONTROL", "RAW"),
                        help="matched 315–501 GeV analyses using the 150–1000 GeV hard window")
    args = parser.parse_args()
    inputs = [read_json(getattr(args, name)) for name in ("medium", "control", "raw", "table14", "table18")]
    high = [read_json(path) for path in args.high_samples] if args.high_samples else None
    page = report(*inputs, high_samples=high)
    args.output.write_text(page)


if __name__ == "__main__":
    main()
