// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_output.hpp"
#include "jewel/strings.hpp"
#include <map>
#include <stdexcept>
namespace jewel {
std::vector<GeneratorParticle> prepare_vacuum_hadronization(const ShowerState& state,bool recoils,bool compress){
 state.validate();if(state.discarded)throw std::domain_error("discarded hadronization input");
 std::vector<CompressionRecord> records;
 for(const auto& s:state.particles()){
  const auto& p=s.particle;
  if(p.status==1||p.status==2)throw std::domain_error("unfinished shower before hadronization");
  CompressionRecord r;r.k={p.status,p.pdg,0,0,0};if(!recoils&&p.status==3)r.k[0]=22;
  r.p={p.momentum.px,p.momentum.py,p.momentum.pz,p.momentum.energy,s.record_mass};
  r.v={s.generator_vertex.x,s.generator_vertex.y,s.generator_vertex.z,s.generator_vertex.t,0};
  r.production={p.production.x,p.production.y,p.production.z,p.production.t,s.decay_time};
  r.colour=p.colour;r.anticolour=p.anticolour;records.push_back(r);
 }
 auto result=make_vacuum_strings(records,records.size(),compress);
 if(result.discarded)throw std::domain_error("JEWEL vacuum string reconstruction discarded event");
 if(!result.unconsumed_sources.empty())throw std::domain_error("JEWEL vacuum string reconstruction left unconsumed partons");
 std::vector<GeneratorParticle> output;std::map<int,std::pair<int,int>> colours;
 for(auto slot:result.output_sources){const auto& s=state.particles().at(slot);const auto& p=s.particle;
  GeneratorParticle out;out.id=p.id;out.source_index=static_cast<int>(slot);out.pdg=p.pdg;out.status=23;out.final=true;
  out.colour=p.colour;out.anticolour=p.anticolour;out.momentum=p.momentum;out.mass=s.record_mass;
  out.vertex_mm={p.production.x*1e-12,p.production.y*1e-12,p.production.z*1e-12,p.production.t*1e-12};
  if(out.colour)++colours[out.colour].first;if(out.anticolour)++colours[out.anticolour].second;output.push_back(out);
 }
 // This HADROTYPE=0 adapter has no junction interpretation. Reject ambiguous
 // or open tags rather than asking Pythia to invent a different topology.
 for(const auto& entry:colours)if(entry.second!=std::pair<int,int>{1,1})throw std::domain_error("unbalanced or repeated hadronization colour tag");
 return output;
}
}
