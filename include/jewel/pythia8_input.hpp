// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
#include <array>
#include <memory>
#include <optional>
#include <string>
namespace jewel {
struct PythiaInputConfig {
 std::string xml_directory;
 double ecm{5020},pt_min{20},pt_max{100};
 std::optional<double> pthat_bias_power;
 int seed{260000},pdf_set{13},pp_tune{14};
 bool multiple_parton_interactions{false};
 bool initial_state_radiation{false}; // Adapter-only until ISR transport is mapped.
 // External set/member, e.g. CT14nlo/0. Requires an explicit hard coupling policy.
 std::string external_pdf;
 std::optional<double> hard_alpha_s;
 std::optional<int> hard_alpha_s_order;
 std::optional<double> isr_alpha_s;
 std::optional<int> isr_alpha_s_order,alpha_s_nfmax;
 std::optional<bool> isr_alpha_s_cmw;
 std::optional<double> mpi_alpha_s;
 std::optional<int> mpi_alpha_s_order;
 // Power-law pT0Ref [GeV], ecmRef [GeV], energy exponent.
 std::optional<std::array<double,3>> mpi_screening;
 // Double-Gaussian matter core fraction and core/outer radius ratio.
 std::optional<std::array<double,2>> mpi_double_gaussian;
 // Hard process only, ordered as beams A and B. Empty entries inherit the base PDF.
 std::array<std::string,2> hard_external_pdfs{};
};
struct GeneratorParticle {
 ParticleId id{};
 int source_index{},pdg{},status{},colour{},anticolour{};
 bool final{};
 Momentum momentum;
 std::optional<double> energy_before_beam_repair;
 double mass{},scale{};
 std::array<double,4> vertex_mm{}; // Pythia units, NOT JEWEL production coordinates in fm.
 std::vector<ParticleId> mothers,daughters;
};
struct GeneratorJunction {int kind{};std::array<int,3> colours{};};
struct GeneratorSubsystem {
 int index{};double shat{},pt_hat{};
 // BeamRemnants target momenta before applying Msys to event-record copies.
 // They differ from the rounded status-61 outputs; retain source inputs.
 std::optional<std::array<Momentum,2>> primordial_targets;
 std::array<ParticleId,2> incoming{}; // Current Pythia subsystem incoming records.
 std::vector<ParticleId> outgoing,outgoing_final,process_outgoing;
};
struct GeneratorIsrFractions {
 double collision_s{};
 int subsystem{};
 ParticleId emitted{},mother{},recoiler{};
 std::array<double,2> before{},after{}; // Beam A/B fractions around accepted ISR.
};
struct HardPdfSample {int flavour{};double x{},xf{};};
struct HardEventSnapshot {
 std::vector<GeneratorIsrFractions> isr_fractions;
 std::uint64_t attempt{};
 bool multiple_parton_interactions{false};
 std::vector<GeneratorSubsystem> subsystems;
 bool initial_state_radiation{false};
 std::vector<ParticleId> isr_emissions,isr_final;
 int process_code{},incoming_id1{},incoming_id2{};
 double weight{},pt_hat{},x1{},x2{},q_factorization{},q_renormalization{};
 std::array<ParticleId,2> hard_incoming{},hard_outgoing{},hard_final{};
 std::vector<GeneratorParticle> particles;
 std::vector<GeneratorJunction> junctions;
 std::array<HardPdfSample,2> hard_pdfs{}; // Pythia hard-process PDF values, beam A then B.
};
struct GeneratorStatistics {
 std::uint64_t attempts{},returned{},failed{};
 long pythia_accepted{};
 double sigma_mb{},sigma_error_mb{},weight_sum{};
};
// Reduced hard-dijet input only. Does not initialize JEWEL virtualities or run
// its shower. ISR snapshots include emission/final-copy associations; their
// JEWEL transport interpretation remains separate. Optional MPI snapshots
// preserve Pythia subsystem membership; JEWEL MPI evolution is not connected.
class Pythia8Input {
 struct Impl;
 std::unique_ptr<Impl> impl_;
public:
 explicit Pythia8Input(PythiaInputConfig);
 ~Pythia8Input();
 Pythia8Input(const Pythia8Input&)=delete;
 Pythia8Input& operator=(const Pythia8Input&)=delete;
 std::optional<HardEventSnapshot> next();
 GeneratorStatistics statistics() const;
 const std::vector<std::string>& settings() const;
 static int version();
 double nominal_mass(int pdg) const;
};
void validate_hard_snapshot(const HardEventSnapshot&);
// Final copies of the two process outgoing lines in each Pythia subsystem.
// Uses reciprocal copy ancestry, never momentum ranking.
std::vector<std::array<ParticleId,2>> subsystem_hard_final_pairs(const HardEventSnapshot&);
}
