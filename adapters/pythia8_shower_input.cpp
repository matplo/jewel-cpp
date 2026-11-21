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
 auto chains=map_subsystem_isr_ancestry(input);
 ImportedPythiaRecord result;result.state.weight=input.weight;result.junctions=input.junctions;
 auto anchor=[&](ParticleId source,ParticleId parent){const auto& p=input.particles[source-1];ShowerParticle node;node.fraction=0;
  node.particle.pdg=p.pdg;node.particle.status=21;node.particle.momentum=p.momentum;node.record_mass=p.mass;
  node.particle.production={x,y,0,0};node.decay_time=lifetime;node.parent=parent;
  node.generator_vertex={p.vertex_mm[0]*1e12,p.vertex_mm[1]*1e12,p.vertex_mm[2]*1e12,p.vertex_mm[3]*1e12};
  return result.state.append(node,true,true);
 };
 for(std::size_t side=0;side<2;++side){result.source_beams[side]=chains.front()[side].beam;result.beams[side]=anchor(result.source_beams[side],0);}
 result.scratch=result.beams[0];
 std::map<ParticleId,std::pair<std::size_t,std::size_t>> hard_owner,isr_owner;
 for(std::size_t system=0;system<pairs.size();++system){
  ImportedSubsystem native;native.index=input.subsystems[system].index;native.source_hard=pairs[system];native.pt_hat=input.subsystems[system].pt_hat;
  for(std::size_t side=0;side<2;++side){
   require(chains[system][side].beam==result.source_beams[side],"subsystems disagree on beam identity");
   native.incoming[side]=anchor(chains[system][side].hard_incoming,result.beams[side]);
   for(const auto& branch:chains[system][side].branches)require(isr_owner.emplace(branch.emitted_final,std::make_pair(system,side)).second,"ISR final belongs to multiple incoming chains");
  }
  for(std::size_t side=0;side<2;++side){
   native.hard_origins[side]=anchor(input.subsystems[system].process_outgoing[side],0);
   require(hard_owner.emplace(pairs[system][side],std::make_pair(system,side)).second,"hard final belongs to multiple subsystems");
  }
  result.subsystems.push_back(native);
 }
 auto remnant_parent=[&](ParticleId source){std::set<ParticleId> visited;
  while(source!=result.source_beams[0]&&source!=result.source_beams[1]){
   require(visited.insert(source).second,"cyclic remnant ancestry");const auto& node=input.particles[source-1];
   require((std::abs(node.status)==63||std::abs(node.status)==64)&&node.mothers.size()==1,"unsupported remnant ancestry");
   auto parent=node.mothers.front();const auto& mother=input.particles[parent-1];
   require(std::find(mother.daughters.begin(),mother.daughters.end(),source)!=mother.daughters.end(),"nonreciprocal remnant ancestry");
   if(parent!=result.source_beams[0]&&parent!=result.source_beams[1])require(mother.pdg==node.pdg,"remnant copy changes flavour");
   source=parent;
  }
  return result.beams[source==result.source_beams[0]?0:1];
 };
 std::set<ParticleId> isr(input.isr_final.begin(),input.isr_final.end());
 for(auto source:order){const auto& p=input.particles[source-1];ShowerParticle node;node.fraction=0;
  node.particle.pdg=p.pdg;node.particle.status=4;node.particle.momentum=p.momentum;node.record_mass=p.mass;
  node.particle.colour=p.colour;node.particle.anticolour=p.anticolour;
  node.particle.production={x,y,0,0};node.decay_time=lifetime;
  node.generator_vertex={p.vertex_mm[0]*1e12,p.vertex_mm[1]*1e12,p.vertex_mm[2]*1e12,p.vertex_mm[3]*1e12};
  auto hard=hard_owner.find(source);ImportedRole role=ImportedRole::remnant;
  if(hard!=hard_owner.end()){
   require(!isr.count(source),"hard final also labeled as ISR emission");
   auto system=hard->second.first;node.parent=result.subsystems[system].hard_origins[hard->second.second];
   role=system==0?ImportedRole::primary_hard:ImportedRole::mpi_hard;
  }else if(isr.count(source)){
   require(isr_owner.count(source),"ISR final lacks an incoming parent");auto owner=isr_owner.at(source);
   node.parent=result.subsystems[owner.first].incoming[owner.second];role=ImportedRole::isr_emission;
  }else node.parent=remnant_parent(source);
  result.state.colour_max=std::max({result.state.colour_max,p.colour,p.anticolour});
  auto id=result.state.append(node,true,true);result.input_sources.emplace(id,source);result.roles.emplace(id,role);
  // JEWEL's MPI candidates have K(:,3)=0. Keep stable source ancestry, but
  // expose the source scheduling parent to GETTHETAM/coherence separately.
  if(role==ImportedRole::mpi_hard)result.state.at(id).scheduling_slots=std::array<std::size_t,3>{0,0,0};
  if(hard!=hard_owner.end())result.subsystems[hard->second.first].hard[hard->second.second]=id;
 }
 result.state.validate();return result;
}

void prepare_imported_partons(ImportedPythiaRecord& imported,
 const std::function<double(int)>& nominal_mass,double eta_max){
 auto require=[](bool ok,const char* why){if(!ok)throw std::domain_error(why);};
 imported.state.validate();
 require(!imported.subsystems.empty(),"preparation lacks primary hard pair");
 require(bool(nominal_mass),"preparation lacks nominal mass provider");
 std::vector<ShowerParticle> records;std::vector<double> masses;
 std::array<std::size_t,2> hard{imported.input_sources.size(),imported.input_sources.size()};
 for(const auto& node:imported.state.particles())if(imported.input_sources.count(node.particle.id)){
  require(node.particle.status==4,"preparation requires inactive imported finals");
  for(std::size_t side=0;side<2;++side)if(node.particle.id==imported.subsystems.front().hard[side])hard[side]=records.size();
  masses.push_back(nominal_mass(node.particle.pdg));records.push_back(node);
 }
 require(records.size()==imported.input_sources.size(),"preparation map includes missing records");
 prepare_surviving_partons(records,hard[0],hard[1],masses,eta_max);
 auto working=imported.state;
 for(const auto& node:records)working.at(node.particle.id)=node;
 working.validate();imported.state=std::move(working);
}

SubsystemInitialization initialize_imported_subsystems(ImportedPythiaRecord& imported,
 HardInitializationConfig config,const BranchingSampler& sampler,Random& random,MpiChannelPolicy policy){
 auto require=[](bool ok,const char* why){if(!ok)throw std::domain_error(why);};
 require(policy==MpiChannelPolicy::source_primary||policy==MpiChannelPolicy::outgoing_flavour,"invalid MPI channel policy");
 imported.state.validate();require(!imported.subsystems.empty(),"initialization lacks primary subsystem");
 std::set<ParticleId> seen;
 for(std::size_t i=0;i<imported.subsystems.size();++i){const auto& pair=imported.subsystems[i];
  require(pair.index==int(i),"subsystem initialization requires ordered contiguous indices");
  for(auto id:pair.hard){
   require(seen.insert(id).second&&imported.input_sources.count(id),"invalid or duplicated subsystem hard leg");
   const auto& node=imported.state.at(id);int pdg=node.particle.pdg;
   bool photon=i>0&&pdg==22&&node.particle.status==4&&node.record_mass==0&&node.particle.colour==0&&node.particle.anticolour==0;
   require(pdg==21||(std::abs(pdg)>=1&&std::abs(pdg)<=5)||photon,"subsystem initialization requires quarks/gluons or inactive massless photons");
   require(node.particle.status==1||(i>0&&node.particle.status==4),"subsystem initialization requires prepared hard legs");
   require(imported.roles.at(id)==(i==0?ImportedRole::primary_hard:ImportedRole::mpi_hard),"subsystem hard role mismatch");
  }
 }
 // In native mode, do not evaluate an unused Pythia6 colour-partner diagnostic.
 // Pythia8 junction legs have no ordinary partner; no junction angle is invented.
 if(policy==MpiChannelPolicy::outgoing_flavour&&!config.first_angle_ordering)config.evaluate_colour_angles=false;
 const auto& primary=imported.subsystems.front();SubsystemInitialization result;
 for(std::size_t side=0;side<2;++side)result.primary_sampling_channels[side]=imported.state.at(primary.hard[side]).particle.pdg==21?Channel::GC:Channel::QQ;
 auto working=imported.state;config.pt_hat=primary.pt_hat;
 config.mpi_sampling_channels=result.primary_sampling_channels;
 for(std::size_t i=0;i<imported.subsystems.size();++i){const auto& pair=imported.subsystems[i];config.mpi_pair=i>0;
  if(policy==MpiChannelPolicy::outgoing_flavour){
   std::array<Channel,2> channels;
   for(std::size_t side=0;side<2;++side)channels[side]=working.at(pair.hard[side]).particle.pdg==21?Channel::GC:Channel::QQ;
   config.mpi_sampling_channels=channels;
  }
  result.pairs.push_back(initialize_hard_dijet(working,pair.hard[0],pair.hard[1],config,sampler,random));
 }
 working.validate();imported.state=std::move(working);return result;
}

}
