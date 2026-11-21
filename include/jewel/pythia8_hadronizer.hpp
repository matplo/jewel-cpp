// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
namespace jewel {
enum class FragmentationProfile {Monash14,Jewel260Mapped};
struct HadronizerConfig {
 std::string xml_directory;int seed{260001};
 FragmentationProfile fragmentation{FragmentationProfile::Monash14};
};
// Mapped explicit INITPYTHIA overrides; not an equivalence claim for the two engines.
std::vector<std::string> jewel260_fragmentation_settings();
struct HadronizedSnapshot {
 std::vector<GeneratorParticle> particles;
 // Entry i+1 in the Pythia record was imported from input_sources[i].
 std::vector<ParticleId> input_sources;
};
struct HadronizationStatistics {std::uint64_t attempts{},returned{},failed{};};
class Pythia8Hadronizer {
 struct Impl;
 std::unique_ptr<Impl> impl_;
public:
 explicit Pythia8Hadronizer(HadronizerConfig);
 ~Pythia8Hadronizer();
 Pythia8Hadronizer(const Pythia8Hadronizer&)=delete;
 Pythia8Hadronizer& operator=(const Pythia8Hadronizer&)=delete;
 // Inputs must be a complete colour-connected state, including needed remnants.
 // Momentum units are GeV and vertex units are mm. No JEWEL unit conversion,
 // string repair, reconnection, hard generation or showering is done here.
 std::optional<HadronizedSnapshot> hadronize(const std::vector<GeneratorParticle>&,
  const std::vector<GeneratorJunction>& = {});
 // Nominal ParticleData mass, without a random resonance-mass draw.
 double nominal_mass(int pdg) const;
 HadronizationStatistics statistics() const;
 const std::vector<std::string>& settings() const;
};
}
