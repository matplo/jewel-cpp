// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_input.hpp"
#include <algorithm>
#include <stdexcept>
#include <set>
#include <cmath>
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
std::vector<IsrTimingAttachment> attach_pythia_isr_times(ShowerState& state,
 const std::map<ParticleId,ParticleId>& input_sources,const HardEventSnapshot& input,
 const std::vector<std::array<IsrTimingChain,2>>& chains,const IsrTimeConfig& config,Random& random){
 auto require=[](bool ok,const char* why){if(!ok)throw std::domain_error(why);};
 validate_hard_snapshot(input);state.validate();
 require(chains.size()==input.subsystems.size(),"missing ISR timing subsystem");
 std::set<ParticleId> live,finals,expected(input.isr_final.begin(),input.isr_final.end()),seen;
 for(const auto& node:state.particles())live.insert(node.particle.id);
 for(const auto& node:input.particles)if(node.final)finals.insert(node.id);
 std::map<ParticleId,ParticleId> native_for;
 auto same=[](Momentum a,Momentum b){return a.px==b.px&&a.py==b.py&&a.pz==b.pz&&a.energy==b.energy;};
 for(const auto& [native,source]:input_sources){
  require(live.count(native)&&finals.count(source),"ISR timing source map requires live final records");
  require(native_for.emplace(source,native).second,"duplicated ISR timing source mapping");
  const auto& original=input.particles[source-1];const auto& node=state.at(native);
  require(node.particle.pdg==original.pdg&&node.record_mass==original.mass&&same(node.particle.momentum,original.momentum),"ISR timing must attach before final-record kinematic changes");
 }
 require(native_for.size()==finals.size(),"ISR timing source map omits final records");
 std::vector<IsrTimingAttachment> bindings;
 for(std::size_t system=0;system<chains.size();++system){
  const auto& subsystem=input.subsystems[system];const auto& roots=input.particles[subsystem.process_outgoing.front()-1].mothers;
  require(roots.size()==2,"missing ISR timing process roots");
  for(std::size_t side=0;side<2;++side){const auto& chain=chains[system][side];
   require(chain.hard_incoming==roots[side],"ISR timing chain belongs to another process root");
   const auto& beam_copy=input.particles[subsystem.incoming[side]-1];
   require(beam_copy.mothers.size()==1&&chain.beam==beam_copy.mothers[0],"ISR timing chain belongs to another beam");
   require(chain.steps.size()==chain.emitted_final.size(),"incomplete ISR timing emission metadata");
   for(std::size_t j=0;j<chain.emitted_final.size();++j){auto source=chain.emitted_final[j];
    require(expected.count(source)&&seen.insert(source).second,"missing or duplicated ISR timing emission");
    require(std::find(subsystem.outgoing_final.begin(),subsystem.outgoing_final.end(),source)!=subsystem.outgoing_final.end(),"ISR timing emission belongs to another subsystem");
    const auto& original=input.particles[source-1];const auto& k=chain.steps[j].emitted;
    require(k.record_mass==original.mass&&same(k.momentum,original.momentum),"ISR timing emission kinematics disagree with final record");
    bindings.push_back({native_for.at(source),source,system,side});
   }
  }
 }
 require(seen==expected,"unmapped ISR timing emission");
 auto working=state;std::size_t binding=0;
 for(const auto& pair:chains)for(const auto& chain:pair){
  auto times=assign_isr_emission_times(chain.hard,chain.steps,config,random);
  for(const auto& time:times){auto& node=working.at(bindings[binding++].native);
   node.particle.production=time.production;node.decay_time=time.end_time;
  }
 }
 require(binding==bindings.size(),"ISR timing output count mismatch");working.validate();
 state=std::move(working);return bindings;
}

ImportedPythiaRecord import_pythia_final_record(const HardEventSnapshot& input,
 double x,double y,double lifetime,const std::vector<ParticleId>& requested_order){
 auto require=[](bool ok,const char* why){if(!ok)throw std::domain_error(why);};
 auto pairs=subsystem_hard_final_pairs(input);
 require(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(lifetime)&&lifetime>=0,"invalid imported record spacetime");
 std::vector<ParticleId> order;std::set<ParticleId> finals;
 for(const auto& p:input.particles)if(p.final){order.push_back(p.id);finals.insert(p.id);}
 if(!requested_order.empty()){
  require(requested_order.size()==finals.size()&&std::set<ParticleId>(requested_order.begin(),requested_order.end())==finals,"import order is not a complete final-record permutation");
  order=requested_order;
 }
 ImportedPythiaRecord result;result.state.weight=input.weight;result.junctions=input.junctions;
 result.scratch=result.state.append({});
 std::map<ParticleId,std::pair<std::size_t,std::size_t>> hard_owner;
 for(std::size_t system=0;system<pairs.size();++system){
  ShowerParticle anchor;anchor.particle.status=17;
  auto id=result.state.append(anchor,true,true);
  result.subsystems.push_back({input.subsystems[system].index,id,{},pairs[system],input.subsystems[system].pt_hat});
  for(std::size_t side=0;side<2;++side)require(hard_owner.emplace(pairs[system][side],std::make_pair(system,side)).second,"hard final belongs to multiple subsystems");
 }
 std::set<ParticleId> isr(input.isr_final.begin(),input.isr_final.end());
 for(auto source:order){const auto& p=input.particles[source-1];ShowerParticle node;
  node.particle.pdg=p.pdg;node.particle.status=4;node.particle.momentum=p.momentum;node.record_mass=p.mass;
  node.particle.colour=p.colour;node.particle.anticolour=p.anticolour;
  node.particle.production={x,y,0,0};node.decay_time=lifetime;
  node.generator_vertex={p.vertex_mm[0]*1e12,p.vertex_mm[1]*1e12,p.vertex_mm[2]*1e12,p.vertex_mm[3]*1e12};
  auto hard=hard_owner.find(source);ImportedRole role=ImportedRole::remnant;
  if(hard!=hard_owner.end()){
   require(!isr.count(source),"hard final also labeled as ISR emission");
   auto system=hard->second.first;node.parent=result.subsystems[system].process_anchor;
   role=system==0?ImportedRole::primary_hard:ImportedRole::mpi_hard;
  }else if(isr.count(source))role=ImportedRole::isr_emission;
  result.state.colour_max=std::max({result.state.colour_max,p.colour,p.anticolour});
  auto id=result.state.append(node,true,true);result.input_sources.emplace(id,source);result.roles.emplace(id,role);
  if(hard!=hard_owner.end())result.subsystems[hard->second.first].hard[hard->second.second]=id;
 }
 for(const auto& system:result.subsystems)result.state.connect(system.process_anchor,system.hard[0],system.hard[1]);
 result.state.validate();return result;
}

}
