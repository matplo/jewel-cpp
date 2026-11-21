// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
#include <array>
#include <unordered_map>
#include <map>
#include <optional>
#include "jewel/compression.hpp"
namespace jewel {
namespace detail {struct CollisionWorkspace;}
struct ShowerParticle {
 // Source K(:,3:5) after compaction; ancestry below remains stable-ID history.
 std::optional<std::array<std::size_t,3>> scheduling_slots;
 Particle particle;
 double record_mass{}; // Pythia's signed P(:,5), distinct from computed p^2.
 ParticleId parent{};
 ParticleId coherent_partner{}; // Second incoming leg of a coherent merge; K(:,3) retains parent.
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
 double unused_evolution_time{};
 std::vector<double> retired_evolution; // Physical-slot metadata retained by partial rollback.
 std::vector<bool> retired_medium_induced;
 std::map<ParticleId,ShowerParticle> archived;
};
class ShowerState {
 friend struct detail::CollisionWorkspace;
 ParticleId append_impl(ShowerParticle,bool,bool,bool);
 ShowerSnapshot snapshot_truncated_impl(std::size_t,bool) const;
 std::vector<ShowerParticle> particles_;
 std::unordered_map<ParticleId,std::size_t> slots_;
 ParticleId next_{1};
 std::vector<double> retired_evolution_;
 std::vector<bool> retired_medium_induced_;
 double unused_evolution_time_{};
 std::map<ParticleId,ShowerParticle> archived_;
public:
 explicit ShowerState(double unused_evolution_time=0);
 int colour_max{};
 double weight{1};
 bool discarded{};
 ParticleId append(ShowerParticle,bool inherit_evolution_time=false,bool inherit_medium_induced=false);
 const ShowerParticle& at(ParticleId) const;
 ShowerParticle& at(ParticleId);
 const auto& particles() const{return particles_;}
 void connect(ParticleId parent,ParticleId first,ParticleId second,bool replace=false);
 void connect_collision(ParticleId incoming,ParticleId recoil,ParticleId outgoing);
 void connect_coherent(ParticleId first,ParticleId second,ParticleId coherent);
 // Physics follows source slot links; at()/particle.parents retain ancestry.
 ParticleId scheduling_parent(ParticleId) const;
 std::array<ParticleId,2> scheduling_daughters(ParticleId) const;
 CompressionResult compress(ParticleId protected_id=0);
 const auto& archived() const{return archived_;}
 void validate() const;
 ShowerSnapshot snapshot() const;
 // Caller must repair links/statuses before restoring a truncated snapshot.
 ShowerSnapshot snapshot_truncated(std::size_t active_count) const;
 void restore(const ShowerSnapshot&);
};
}
