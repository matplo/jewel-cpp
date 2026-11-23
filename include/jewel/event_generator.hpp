// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include <vector>

namespace jewel {

struct EventGeneratorConfig {
 PythiaInputConfig input;
 // The expanding medium uses a fixed impact parameter, in fm.
 std::string mode{"vacuum"};
 double impact_parameter_fm{0};
 bool longitudinal_boost{false};
 bool keep_recoils{true};
 bool jewel_fragmentation{false};
 bool pi0_decay{true};
 double decay_tau0_max_mm{10};
 int angular_ordering{0};
};

struct FinalParticle {
 int pdg{};
 Momentum momentum; // GeV
};

struct GeneratedEvent {
 std::uint64_t attempt{};
 bool accepted{false};
 double weight{},pt_hat{};
 std::string stage,error;
 std::vector<FinalParticle> particles;
};

// Persistent Pythia8 input and hadronization streams. Each call attempts
// one event and returns an owned snapshot, including failed attempts.
class EventGenerator {
 struct Impl;
 std::unique_ptr<Impl> impl_;
public:
 explicit EventGenerator(EventGeneratorConfig config);
 ~EventGenerator();
 EventGenerator(const EventGenerator&)=delete;
 EventGenerator& operator=(const EventGenerator&)=delete;
 GeneratedEvent next();
 // Counters and cross section at the Pythia hard-input boundary.
 GeneratorStatistics hard_statistics() const;
};

}
