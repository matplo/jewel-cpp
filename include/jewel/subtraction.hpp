// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
namespace jewel {
// Input must already select the particles eligible for subtraction. The broken
// process-offset logic of the upstream Fortran wrapper is not part of this API.
struct SubtractionParticle {
 ParticleId source{};
 int pdg{};
 Momentum momentum;
 double mass{};
 bool thermal_residual{};
};
// SignedMass encodes negative invariant mass squared as -sqrt(-m2); no
// projection or clamping is applied to momenta. Reference rejects non-real mass.
enum class SubtractionMassPolicy {Reference,SignedMass};
struct SubtractionResult {
 std::vector<SubtractionParticle> particles;
 std::size_t skipped_particles{},skipped_thermals{},spacelike_mass_fields{};
 bool capacity_exceeded{};
};
// JEWEL 2.6.0 constituent subtraction in rapidity/phi. Residual thermal entries
// are emitted with POSITIVE momentum and random pion IDs, exactly as upstream.
// Random is used only for their IDs. Inputs are never modified. Equal-distance
// ordering follows the platform qsort used by the original implementation.
SubtractionResult subtract_thermal_momenta(const std::vector<SubtractionParticle>&,
 const std::vector<Momentum>& thermals,double maximum_distance,Random&,
 SubtractionMassPolicy=SubtractionMassPolicy::Reference);
}
