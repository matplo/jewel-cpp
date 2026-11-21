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
 int seed{260000},pdf_set{13},pp_tune{14};
 // External set/member, e.g. CT14nlo/0. Requires an explicit hard coupling policy.
 std::string external_pdf;
 std::optional<double> hard_alpha_s;
 std::optional<int> hard_alpha_s_order;
};
struct GeneratorParticle {
 ParticleId id{};
 int source_index{},pdg{},status{},colour{},anticolour{};
 bool final{};
 Momentum momentum;
 double mass{},scale{};
 std::array<double,4> vertex_mm{}; // Pythia units, NOT JEWEL production coordinates in fm.
 std::vector<ParticleId> mothers,daughters;
};
struct GeneratorJunction {int kind{};std::array<int,3> colours{};};
struct HardEventSnapshot {
 std::uint64_t attempt{};
 int process_code{},incoming_id1{},incoming_id2{};
 double weight{},pt_hat{},x1{},x2{},q_factorization{},q_renormalization{};
 std::array<ParticleId,2> hard_incoming{},hard_outgoing{},hard_final{};
 std::vector<GeneratorParticle> particles;
 std::vector<GeneratorJunction> junctions;
};
struct GeneratorStatistics {
 std::uint64_t attempts{},returned{},failed{};
 long pythia_accepted{};
 double sigma_mb{},sigma_error_mb{},weight_sum{};
};
// Reduced hard-dijet input only. Does not initialize JEWEL virtualities or run
// its shower. ISR/MPI histories need a separately validated interpretation.
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
};
void validate_hard_snapshot(const HardEventSnapshot&);
}
