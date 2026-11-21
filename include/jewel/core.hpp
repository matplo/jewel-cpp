// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>
namespace jewel {
struct Momentum { double px{}, py{}, pz{}, energy{}; }; // GeV
struct SpaceTime { double x{}, y{}, z{}, t{}; }; // fm; c = 1
using ParticleId = std::uint64_t;
struct Particle {
 ParticleId id{}; int pdg{}, status{}, colour{}, anticolour{};
 Momentum momentum; SpaceTime production;
 std::vector<ParticleId> parents;
};
class Event {
 std::vector<Particle> particles_;
 ParticleId next_{1};
public:
 ParticleId append(int pdg, Momentum p, SpaceTime v, std::vector<ParticleId> parents = {});
 const std::vector<Particle>& particles() const { return particles_; }
};
struct Random { virtual ~Random() = default; virtual double uniform() = 0; };
class ReplayRandom final : public Random {
 std::vector<double> tape_; std::size_t cursor_{};
public:
 explicit ReplayRandom(std::vector<double> tape);
 double uniform() override;
 std::size_t checkpoint() const { return cursor_; }
 void restore(std::size_t cursor);
};
// Versioned, portable SplitMix64 stream with midpoint conversion to (0,1).
// Persist algorithm, seed and draw count together for restart.
class SplitMixRandom final : public Random {
 std::uint64_t seed_,draws_{};
public:
 static constexpr const char* algorithm="splitmix64-open52-v1";
 explicit SplitMixRandom(std::uint64_t seed):seed_(seed){}
 double uniform() override;
 std::uint64_t seed() const {return seed_;}
 std::uint64_t checkpoint() const {return draws_;}
 void restore(std::uint64_t draws){draws_=draws;}
};
struct MediumSample { double temperature{}, debye_mass{}, scatterer_mass{}, density{}; };
struct Medium { virtual ~Medium() = default; virtual MediumSample sample(SpaceTime) const = 0; };
struct Vacuum final : Medium { MediumSample sample(SpaceTime) const override { return {}; } };
// Configuration is per engine, never a shared Fortran COMMON block.
struct Kernels {
 int nf{3}; double lambda_shower{0.2}, lambda_scattering{0.2};
 double pqq(double z) const;
 double pgq(double z) const;
 double pgg(double z) const;
 double pqg(double z) const;
 double alphas(double t, double lambda) const;
 double intpqq(double z, double q) const;
 double intpgglow(double z, double q) const;
 double intpgghigh(double z, double q) const;
 double gett(double minimum, double maximum, double debye, Random&) const;
private:
 void validate() const;
 double log_integral(double z, double q) const;
};
}
