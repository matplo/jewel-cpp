"""Generate hadronized JEWEL events in Python and cluster anti-kt jets."""

import argparse

import hepyy

hepyy.load(["jewel_cpp", "fastjet"])

import fastjet
import jewel_cpp


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--events", type=int, default=10, help="number of generation attempts")
    parser.add_argument("--seed", type=int, default=2601301)
    args = parser.parse_args()
    if args.events < 1:
        parser.error("--events must be positive")

    config = jewel_cpp.EventGeneratorConfig()
    config.input.seed = args.seed
    config.input.initial_state_radiation = True
    generator = jewel_cpp.EventGenerator(config)
    jet_definition = fastjet.JetDefinition(fastjet.antikt_algorithm, 0.4)
    particle_vector = fastjet.gbl.std.vector[fastjet.PseudoJet]

    for _ in range(args.events):
        event = generator.next()
        if not event.accepted:
            print(f"Attempt {event.attempt} failed in {event.stage}: {event.error}")
            continue

        particles = particle_vector()
        for particle in event.particles:
            if abs(particle.pdg) in (12, 14, 16):
                continue
            p = particle.momentum
            particles.push_back(fastjet.PseudoJet(p.px, p.py, p.pz, p.energy))

        # Keep the ClusterSequence alive while inspecting its jets.
        sequence = fastjet.ClusterSequence(particles, jet_definition)
        jets = fastjet.sorted_by_pt(sequence.inclusive_jets(20.0))
        pts = [round(jet.pt(), 2) for jet in jets if abs(jet.eta()) < 2.0]
        print(f"Event {event.attempt}, weight {event.weight:.6g}, jet pT [GeV] {pts}")

    stats = generator.hard_statistics()
    print(f"Hard input: {stats.returned}/{stats.attempts} events, sigma = {stats.sigma_mb:.6g} mb")


if __name__ == "__main__":
    main()
