// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
namespace jewel {
struct IsrKinematics {Momentum momentum;double record_mass{},nominal_mass{};};
// Ordered backwards from the hard incoming leg towards its beam.
struct IsrTimeStep {IsrKinematics incoming,emitted;int beam_side{};};
struct IsrTimedLine {SpaceTime production;double end_time{};};
struct IsrTimeConfig {double x{},y{},formation_factor{1},lifetime{};bool scatter{true};};
struct IsrTimeResult {IsrTimedLine hard;std::vector<IsrTimedLine> incoming,emitted;};
// Translation of interpretpythiaevent's backward timing block, not a Pythia8
// ancestry mapper. The final step must reach beam_side +1/-1; prior steps use 0.
IsrTimeResult assign_isr_times(const IsrKinematics& hard,const std::vector<IsrTimeStep>&,
 const IsrTimeConfig&,Random&);
}
