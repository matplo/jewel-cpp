// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
#include <array>
#include <unordered_map>
namespace jewel {
struct ShowerParticle {
 Particle particle;
 double record_mass{}; // Pythia's signed P(:,5), distinct from computed p^2.
 ParticleId parent{};
 ParticleId collision_source{}; // Recoil origin; stock K(:,3) remains zero.
 std::array<ParticleId,2> daughters{};
 SpaceTime generator_vertex; // Auxiliary V record, distinct from MV production point.
 double decay_time{}, evolution_time{}, fraction{-1}, angle{};
 bool quark_pair{}, angular_required{}, medium_induced{};
};
struct ShowerSnapshot {
 std::vector<ShowerParticle> particles;
 int colour_max{};
 double weight{1};
 bool discarded{};
};
class ShowerState {
 std::vector<ShowerParticle> particles_;
 std::unordered_map<ParticleId,std::size_t> slots_;
 ParticleId next_{1};
public:
 int colour_max{};
 double weight{1};
 bool discarded{};
 ParticleId append(ShowerParticle);
 const ShowerParticle& at(ParticleId) const;
 ShowerParticle& at(ParticleId);
 const auto& particles() const{return particles_;}
 void connect(ParticleId parent,ParticleId first,ParticleId second);
 void connect_collision(ParticleId incoming,ParticleId recoil,ParticleId outgoing);
 void validate() const;
 ShowerSnapshot snapshot() const;
 void restore(const ShowerSnapshot&);
};
}
