// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
namespace jewel {
struct ProjectionParticle {Momentum momentum;double mass{},nominal_mass{};};
struct ProjectionResult {
 std::vector<ProjectionParticle> particles;
 int massless_iterations{},massive_iterations{};
 bool insufficient_energy{},massless_converged{},massive_converged{};
};
// PUTONSHELL 0/1/2 numerical operation. Nominal masses are supplied by the
// adapter, not inferred here. first is the explicit zero-based mode-2 range;
// modes 0/1 ignore it, as in the source. No channel-specific unsafe indices.
ProjectionResult project_on_shell(const std::vector<ProjectionParticle>&,int mode,std::size_t first=0);
}
