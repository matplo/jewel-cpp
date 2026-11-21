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
// Signed nonzero energies reproduce the source algebra, including reversed
// time intervals. This does not assert that such a history is physical.
IsrTimeResult assign_isr_times(const IsrKinematics& hard,const std::vector<IsrTimeStep>&,
 const IsrTimeConfig&,Random&);
// Final-record projection of ISR timing. Incoming bookkeeping lines are not
// retained by interpretpythiaevent. An empty chain has no emitted lines but
// still consumes its hard-line draw, including at zero recorded virtuality.
// No nonfinite intermediate incoming coordinates are constructed in that case.
std::vector<IsrTimedLine> assign_isr_emission_times(const IsrKinematics& hard,
 const std::vector<IsrTimeStep>&,const IsrTimeConfig&,Random&);
}
