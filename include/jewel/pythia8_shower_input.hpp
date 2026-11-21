// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include "jewel/pythia8_isr.hpp"
#include "jewel/hard_initialization.hpp"
#include <map>
#include <functional>
namespace jewel {
struct PreparedDijet {
 ShowerState state{-1e6};
 ParticleId scratch{},hard_process{};
 std::array<ParticleId,2> hard{};
 std::map<ParticleId,ParticleId> input_sources;
 HardInitializationResult initialization;
 double weight{};
};
// Reduced ISR/MPI-off adapter: only final hard copies evolve; remnants remain
// in the record with status 4. An inert process anchor distinguishes the hard
// scattering from a shower splitting while permitting coherence of later daughters.
// pt_hat comes from the snapshot. First-angle ordering needs a full mapped
// hard ancestry and is rejected here; it remains supported by the core routine.
PreparedDijet prepare_pythia_dijet(const HardEventSnapshot&,HardInitializationConfig,
 const BranchingSampler&,Random&);
struct IsrTimingAttachment {ParticleId native{},source{};std::size_t subsystem{},side{};};
// Attach candidate ISR timing before any final-record kinematic preparation.
// Requires a complete one-to-one map of source final particles to live native
// records. Only production and decay_time change; all other metadata survive.
// Records are transactional; random draws are not rewound on a timing failure.
std::vector<IsrTimingAttachment> attach_pythia_isr_times(ShowerState&,
 const std::map<ParticleId,ParticleId>& input_sources,const HardEventSnapshot&,
 const std::vector<std::array<IsrTimingChain,2>>&,const IsrTimeConfig&,Random&);

enum class ImportedRole {primary_hard,mpi_hard,isr_emission,remnant};
struct ImportedSubsystem {
 int index{};
 std::array<ParticleId,2> incoming{},hard_origins{};
 std::array<ParticleId,2> hard{},source_hard{};
 double pt_hat{};
};
struct ImportedPythiaRecord {
 ShowerState state{-1e6};ParticleId scratch{};
 std::array<ParticleId,2> beams{},source_beams{};
 std::vector<ImportedSubsystem> subsystems;
 std::map<ParticleId,ParticleId> input_sources;
 std::map<ParticleId,ImportedRole> roles; // Native IDs.
 std::vector<GeneratorJunction> junctions;
};
// Raw final-record import, not an evolution-ready event. All finals start at
// status 4. Distinct original hard records, incoming records and beam records
// preserve ancestry; MPI hard legs separately have scheduling parent zero.
// Beam records occupy slots 1/2 and slot 1 is scratch.
// Selection, virtuality assignment and evolution still remain separate stages.
// Optional order must be a permutation of all final source IDs.
ImportedPythiaRecord import_pythia_final_record(const HardEventSnapshot&,
 double x,double y,double lifetime,const std::vector<ParticleId>& order={});

// Apply GENEVENT survivor preparation to all imported finals in native slot
// order, using the primary hard pair for ordered MAKEMASSLESS recoil.
// Call after ISR timing and before primary/MPI virtuality initialization.
// Nominal masses come from the generator. Entire state is transactional.
void prepare_imported_partons(ImportedPythiaRecord&,
 const std::function<double(int)>& nominal_mass,double max_pseudorapidity);

struct SubsystemInitialization {
 std::array<Channel,2> primary_sampling_channels;
 std::vector<HardInitializationResult> pairs;
};
// Initialize the prepared primary pair, then MPI pairs in subsystem order.
// MPI calls inherit primary TYPE1/TYPE2, not their own outgoing flavours.
// Currently all pairs must be quarks/gluons. Unsupported pairs fail before
// drawing; later failures roll back the whole state but not the random stream.
SubsystemInitialization initialize_imported_subsystems(ImportedPythiaRecord&,
 HardInitializationConfig,const BranchingSampler&,Random&);

}
