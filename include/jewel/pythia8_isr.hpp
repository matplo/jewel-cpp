// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
namespace jewel {
struct IsrBranchMapping {
 ParticleId mother{},daughter{},emitted{},emitted_final{};
 // Mother minus emitted momentum in this branching's recorded recoil frame.
 // This is NOT the on-shell bookkeeping daughter's stored momentum.
 // A spacelike vector can have negative energy in this frame; retain its sign.
 Momentum spacelike_daughter;
 double virtuality_squared{};
};
struct IsrIncomingChain {
 ParticleId hard_incoming{},beam{};
 std::vector<ParticleId> backward_lines; // hard root through last status-61 line
 std::vector<IsrBranchMapping> branches; // hard-to-beam order, skipping recoil copies
};
// Preserve stage-local frames. Connecting these stages to one formation-time
// chain requires further recoil/frame mapping; do not feed the result directly
// into assign_isr_times as if all momenta belonged to one shower frame.
std::array<IsrIncomingChain,2> map_pythia_isr_ancestry(const HardEventSnapshot&);
struct IsrRecoilFrame {
 ParticleId emitted{},mother{},recoiler{},old_recoiler{};
 // Row-major Lorentz matrix ordered (energy,px,py,pz), or (dt,dx,dy,dz).
 std::array<double,16> matrix{};
 std::vector<std::array<ParticleId,2>> copies; // before, after
 double maximum_copy_error{};
};
// Source-prescription reconstruction for ordinary initial-initial recoil only.
// Frames are chronological. Primordial kT and IF/secondary-scattering recoil
// require separate mappings and are not silently represented by these matrices.
std::vector<IsrRecoilFrame> reconstruct_isr_recoil_frames(const HardEventSnapshot&);
Momentum apply_isr_frame(const IsrRecoilFrame&,Momentum);
// Apply to a displacement relative to an independently chosen event origin.
// Does not choose that origin or assign a production time.
SpaceTime apply_isr_frame_displacement(const IsrRecoilFrame&,SpaceTime);
struct PrimordialRecoilFrame {
 std::array<ParticleId,2> incoming_before{},incoming_after{};
 std::array<double,16> matrix{}; // Same (energy,px,py,pz) convention.
 std::vector<std::array<ParticleId,2>> copies; // Includes incoming 61 and outgoing 62.
 double maximum_copy_error{};
};
// Single pp hard system only; reconstructs BeamRemnants' toCM/fromCM mapping.
// Additional rescattering mismatch corrections are unsupported and fail closure.
PrimordialRecoilFrame reconstruct_primordial_recoil_frame(const HardEventSnapshot&);
Momentum apply_primordial_frame(const PrimordialRecoilFrame&,Momentum);
SpaceTime apply_primordial_frame_displacement(const PrimordialRecoilFrame&,SpaceTime);
}
