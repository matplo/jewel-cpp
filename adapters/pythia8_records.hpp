// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include <Pythia8/Event.h>
#include <stdexcept>
namespace jewel::detail {
inline std::vector<GeneratorParticle> capture_particles(const Pythia8::Event& event){
 auto indices=[&](const std::vector<int>& input){std::vector<ParticleId> result;
  for(int i:input){if(i<0||i>=event.size())throw std::domain_error("invalid Pythia history index");if(i)result.push_back(static_cast<ParticleId>(i)+1);}return result;};
 std::vector<GeneratorParticle> result;
 for(int i=0;i<event.size();++i){const auto& a=event[i];GeneratorParticle node;
  node.id=static_cast<ParticleId>(i)+1;node.source_index=i;node.pdg=a.id();node.status=a.status();node.colour=a.col();node.anticolour=a.acol();node.final=a.isFinal();
  node.momentum={a.px(),a.py(),a.pz(),a.e()};node.mass=a.m();node.scale=a.scale();node.vertex_mm={a.xProd(),a.yProd(),a.zProd(),a.tProd()};
  node.mothers=indices(a.motherList());node.daughters=indices(a.daughterList());result.push_back(std::move(node));
 }
 return result;
}
}
