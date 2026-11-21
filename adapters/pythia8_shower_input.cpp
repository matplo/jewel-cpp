// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_input.hpp"
#include <algorithm>
#include <stdexcept>
namespace jewel {
PreparedDijet prepare_pythia_dijet(const HardEventSnapshot& input,HardInitializationConfig config,
 const BranchingSampler& sampler,Random& random){
 validate_hard_snapshot(input);
 if(input.multiple_parton_interactions)throw std::domain_error("MPI snapshot requires subsystem evolution; reduced dijet adapter cannot discard it");
 if(input.initial_state_radiation)throw std::domain_error("ISR snapshot requires ISR transport initialization; reduced dijet adapter cannot discard it");
 if(config.first_angle_ordering)throw std::domain_error("Pythia first-angle ordering requires mapped hard ancestry");
 PreparedDijet result;result.weight=input.weight;result.state.weight=input.weight;
 result.scratch=result.state.append({});
 ShowerParticle anchor;anchor.particle.status=17;result.hard_process=result.state.append(anchor,true,true);
 for(const auto& p:input.particles)if(p.final){
  ShowerParticle node;auto& target=node.particle;target.pdg=p.pdg;target.status=4;
  target.momentum=p.momentum;target.colour=p.colour;target.anticolour=p.anticolour;node.record_mass=p.mass;
  // Explicitly convert Pythia's mm vertices to the core's fm coordinates.
  target.production={p.vertex_mm[0]*1e12,p.vertex_mm[1]*1e12,p.vertex_mm[2]*1e12,p.vertex_mm[3]*1e12};
  node.generator_vertex=target.production;
  for(auto id:input.hard_final)if(p.id==id){target.status=1;node.parent=result.hard_process;}
  auto id=result.state.append(node,true,true);result.input_sources.emplace(id,p.id);
  result.state.colour_max=std::max({result.state.colour_max,p.colour,p.anticolour});
  for(std::size_t i=0;i<2;++i)if(p.id==input.hard_final[i])result.hard[i]=id;
 }
 if(!result.hard[0]||!result.hard[1])throw std::domain_error("missing final hard copies");
 result.state.connect(result.hard_process,result.hard[0],result.hard[1]);
 config.pt_hat=input.pt_hat;
 result.initialization=initialize_hard_dijet(result.state,result.hard[0],result.hard[1],config,sampler,random);
 return result;
}
}
