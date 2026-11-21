// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_input.hpp"
#include "jewel/pythia8_isr.hpp"
#include "jewel/pythia8_shower_input.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("Pythia8 input contract failed");}
int main(){
 jewel::PythiaInputConfig bad;bool invalid=false;
 bad.external_pdf="CT14nlo/0";
 try{jewel::Pythia8Input input(bad);}catch(const std::domain_error&){invalid=true;}check(invalid);
 bad.hard_alpha_s=.118;bad.hard_alpha_s_order=2;bad.external_pdf="CT14nlo/0\nPartonLevel:ISR = on";invalid=false;
 try{jewel::Pythia8Input input(bad);}catch(const std::domain_error&){invalid=true;}check(invalid);
 bad.external_pdf="";bad.hard_alpha_s_order=4;invalid=false;
 try{jewel::Pythia8Input input(bad);}catch(const std::domain_error&){invalid=true;}check(invalid);
 for(bool isr:{false,true}){
 jewel::PythiaInputConfig config;config.seed=2600317;config.initial_state_radiation=isr;unsigned emission_count=0,negative_energy_count=0;
 jewel::Pythia8Input first(config),second(config);unsigned accepted=0;
 for(int i=0;i<20;++i){auto a=first.next(),b=second.next();check(a.has_value()==b.has_value());if(!a)continue;++accepted;
  check(a->process_code==b->process_code&&a->weight==b->weight&&a->particles.size()==b->particles.size());
  check(a->hard_incoming==b->hard_incoming&&a->hard_outgoing==b->hard_outgoing&&a->hard_final==b->hard_final);
  for(std::size_t j=0;j<a->particles.size();++j){const auto& x=a->particles[j];const auto& y=b->particles[j];
   check(x.status==y.status&&x.pdg==y.pdg&&x.mothers==y.mothers&&x.daughters==y.daughters&&x.colour==y.colour&&x.anticolour==y.anticolour);
   check(x.momentum.px==y.momentum.px&&x.momentum.py==y.momentum.py&&x.momentum.pz==y.momentum.pz&&x.momentum.energy==y.momentum.energy);
  }
  auto timing_chains=jewel::prepare_subsystem_isr_timing(*a,[&](int pdg){return first.nominal_mass(pdg);});
  check(timing_chains.size()==a->subsystems.size());unsigned timed_emissions=0;
  std::map<jewel::ParticleId,jewel::IsrTimedLine> expected_times;std::vector<std::size_t> chain_draws;
  for(const auto& pair:timing_chains)for(const auto& chain:pair){
   jewel::ReplayRandom timing_random(std::vector<double>(2+2*chain.steps.size(),.5));
   auto times=jewel::assign_isr_emission_times(chain.hard,chain.steps,{},timing_random);
   check(times.size()==chain.emitted_final.size());timed_emissions+=times.size();
   for(std::size_t j=0;j<times.size();++j)check(expected_times.emplace(chain.emitted_final[j],times[j]).second);
   for(const auto& time:times)check(std::isfinite(time.production.t)&&std::isfinite(time.end_time));
   std::size_t draws=1;
   for(const auto& step:chain.steps)draws+=(step.beam_side==0)+(step.emitted.record_mass!=step.emitted.nominal_mass);
   check(timing_random.checkpoint()==draws);chain_draws.push_back(draws);
  }
  check(timed_emissions==a->isr_emissions.size());
  std::vector<jewel::ParticleId> import_order;
  for(auto it=a->particles.rbegin();it!=a->particles.rend();++it)if(it->final)import_order.push_back(it->id);
  auto imported=jewel::import_pythia_final_record(*a,1.2,-.7,20,import_order);
  auto& native=imported.state;auto& sources=imported.input_sources;
  check(native.particles()[0].particle.id==imported.beams[0]&&native.particles()[1].particle.id==imported.beams[1]&&imported.scratch==imported.beams[0]);
  check(native.weight==a->weight&&sources.size()==import_order.size()&&imported.junctions.size()==a->junctions.size());
  for(std::size_t j=0;j<imported.junctions.size();++j)check(imported.junctions[j].kind==a->junctions[j].kind&&imported.junctions[j].colours==a->junctions[j].colours);
  auto pairs=jewel::subsystem_hard_final_pairs(*a);check(pairs.front()==a->hard_final);
  for(std::size_t system=0;system<pairs.size();++system){const auto& pair=imported.subsystems[system];
   check(pair.source_hard==pairs[system]&&pair.index==a->subsystems[system].index&&pair.pt_hat==a->subsystems[system].pt_hat);
   check(pair.hard_origins[0]!=pair.hard_origins[1]);
   for(int side=0;side<2;++side)check(native.at(pair.incoming[side]).parent==imported.beams[side]&&native.at(pair.hard_origins[side]).parent==0);
   for(int side=0;side<2;++side){auto id=pair.hard[side];check(sources.at(id)==pair.source_hard[side]&&native.at(id).parent==pair.hard_origins[side]);
    check(imported.roles.at(id)==(system==0?jewel::ImportedRole::primary_hard:jewel::ImportedRole::mpi_hard));
    check(native.scheduling_parent(id)==(system==0?pair.hard_origins[side]:0));}
  }
  for(const auto& [id,source]:sources){auto& node=native.at(id);const auto& original=a->particles[source-1];
   check(node.particle.status==4&&node.particle.production.x==1.2&&node.particle.production.y==-.7&&node.particle.production.z==0&&node.particle.production.t==0&&node.decay_time==20);
   check(node.generator_vertex.x==original.vertex_mm[0]*1e12&&node.generator_vertex.y==original.vertex_mm[1]*1e12&&node.generator_vertex.z==original.vertex_mm[2]*1e12&&node.generator_vertex.t==original.vertex_mm[3]*1e12);
   if(std::find(a->isr_final.begin(),a->isr_final.end(),source)!=a->isr_final.end())check(imported.roles.at(id)==jewel::ImportedRole::isr_emission&&node.parent!=0);
   node.particle.production={7,8,9,10};node.decay_time=42;node.generator_vertex={1,2,3,4};
  }
  auto bad_order=import_order;bad_order.back()=bad_order.front();bool bad_import=false;
  try{jewel::import_pythia_final_record(*a,0,0,20,bad_order);}catch(const std::domain_error&){bad_import=true;}check(bad_import);
  auto native_before=native;std::size_t expected_draws=0;for(auto draws:chain_draws)expected_draws+=draws;
  auto incomplete=sources;incomplete.erase(incomplete.begin());jewel::ReplayRandom unused({.5});bool attachment_rejected=false;
  try{jewel::attach_pythia_isr_times(native,incomplete,*a,timing_chains,{},unused);}catch(const std::domain_error&){attachment_rejected=true;}
  check(attachment_rejected&&unused.checkpoint()==0);
  auto changed_native=native;changed_native.at(sources.begin()->first).particle.momentum.energy+=1;
  attachment_rejected=false;
  try{jewel::attach_pythia_isr_times(changed_native,sources,*a,timing_chains,{},unused);}catch(const std::domain_error&){attachment_rejected=true;}
  check(attachment_rejected&&unused.checkpoint()==0);
  if(timed_emissions){
   auto corrupted_timing=timing_chains;bool changed_id=false;
   for(auto& pair:corrupted_timing)for(auto& chain:pair)if(!changed_id&&!chain.emitted_final.empty()){chain.emitted_final[0]=a->hard_final[0];changed_id=true;}
   attachment_rejected=false;
   try{jewel::attach_pythia_isr_times(native,sources,*a,corrupted_timing,{},unused);}catch(const std::domain_error&){attachment_rejected=true;}
   check(attachment_rejected&&unused.checkpoint()==0);
  }

  jewel::ReplayRandom exhausted(std::vector<double>(chain_draws.front(),.5));attachment_rejected=false;
  try{jewel::attach_pythia_isr_times(native,sources,*a,timing_chains,{},exhausted);}catch(const std::out_of_range&){attachment_rejected=true;}
  check(attachment_rejected&&exhausted.checkpoint()==chain_draws.front());
  for(const auto& entry:sources)check(native.at(entry.first).particle.production.t==10&&native.at(entry.first).decay_time==42);
  jewel::ReplayRandom attach_rng(std::vector<double>(expected_draws,.5));
  auto bindings=jewel::attach_pythia_isr_times(native,sources,*a,timing_chains,{},attach_rng);
  check(bindings.size()==expected_times.size()&&attach_rng.checkpoint()==expected_draws);
  for(const auto& [id,source]:sources){const auto& node=native.at(id);const auto& old=native_before.at(id);
   check(node.particle.momentum.px==old.particle.momentum.px&&node.particle.momentum.py==old.particle.momentum.py&&node.particle.momentum.pz==old.particle.momentum.pz&&node.particle.momentum.energy==old.particle.momentum.energy);
   check(node.record_mass==old.record_mass&&node.particle.status==old.particle.status&&node.evolution_time==old.evolution_time);
   check(node.generator_vertex.x==1&&node.generator_vertex.y==2&&node.generator_vertex.z==3&&node.generator_vertex.t==4);
   if(expected_times.count(source)){const auto& time=expected_times.at(source);
    check(node.particle.production.x==time.production.x&&node.particle.production.y==time.production.y&&node.particle.production.z==time.production.z&&node.particle.production.t==time.production.t&&node.decay_time==time.end_time);
   }else check(node.particle.production.x==7&&node.particle.production.y==8&&node.particle.production.z==9&&node.particle.production.t==10&&node.decay_time==42);
  }
  for(const auto& binding:bindings)check(sources.at(binding.native)==binding.source);
  native.validate();
  // A nominal-mass failure after earlier records have been visited must not
  // partially activate the imported state or lose attached timing.
  auto preparation_copy=imported;unsigned mass_calls=0;bool preparation_rejected=false;
  try{jewel::prepare_imported_partons(preparation_copy,[&](int pdg){if(++mass_calls==2)throw std::domain_error("mass lookup failure");return first.nominal_mass(pdg);},2);}
  catch(const std::domain_error&){preparation_rejected=true;}
  check(preparation_rejected&&mass_calls==2);
  for(const auto& [id,source]:sources){const auto& old=native.at(id);const auto& node=preparation_copy.state.at(id);
   check(node.particle.status==4&&node.record_mass==old.record_mass&&node.particle.momentum.energy==old.particle.momentum.energy&&node.decay_time==old.decay_time&&node.particle.production.t==old.particle.production.t);}

  jewel::prepare_imported_partons(preparation_copy,[&](int pdg){return first.nominal_mass(pdg);},2);
  for(const auto& [id,source]:sources){const auto& old=native.at(id);const auto& node=preparation_copy.state.at(id);
   check(node.parent==old.parent&&node.scheduling_slots==old.scheduling_slots);
   check(node.particle.production.t==old.particle.production.t&&node.decay_time==old.decay_time);}

  bool invalid_mass=false;
  try{jewel::prepare_subsystem_isr_timing(*a,[](int){return -1.;});}catch(const std::domain_error&){invalid_mass=true;}
  check(invalid_mass);
  auto primordial=jewel::reconstruct_primordial_recoil_frame(*a);
  check(primordial.copies.size()>=4);
  auto displacement=jewel::apply_primordial_frame_displacement(primordial,{.2,-.4,.1,1.});
  check(std::abs(displacement.t*displacement.t-displacement.x*displacement.x-displacement.y*displacement.y-displacement.z*displacement.z-.79)<1e-9);
  auto corrupted_primordial=*a;
  for(auto pair:primordial.copies)if(std::abs(a->particles[pair[1]-1].status)==62){corrupted_primordial.particles[pair[1]-1].momentum.energy+=1;break;}
  bool bad_primordial=false;try{jewel::reconstruct_primordial_recoil_frame(corrupted_primordial);}catch(const std::domain_error&){bad_primordial=true;}check(bad_primordial);
  check(a->initial_state_radiation==isr&&b->initial_state_radiation==isr);
  check(a->isr_emissions==b->isr_emissions&&a->isr_final==b->isr_final);
  emission_count+=a->isr_emissions.size();
  if(isr){
   auto chains=jewel::map_pythia_isr_ancestry(*a);unsigned branches=0;
   auto frames=jewel::reconstruct_isr_recoil_frames(*a);check(frames.size()==a->isr_emissions.size());
   if(!frames.empty()){
    auto corrupted=*a;corrupted.particles[frames[0].copies[0][1]-1].momentum.energy+=1.;bool rejected=false;
    try{jewel::reconstruct_isr_recoil_frames(corrupted);}catch(const std::domain_error&){rejected=true;}check(rejected);
   }
   for(const auto& f:frames){
    auto displacement=jewel::apply_isr_frame_displacement(f,{.2,-.4,.1,1.});
    double interval=displacement.t*displacement.t-displacement.x*displacement.x-displacement.y*displacement.y-displacement.z*displacement.z;
    check(std::abs(interval-.79)<1e-9);
   }
   for(std::size_t side=0;side<2;++side){const auto& chain=chains[side];
    check(chain.hard_incoming==a->hard_incoming[side]&&chain.backward_lines.front()==chain.hard_incoming);
    check(std::abs(a->particles[chain.beam-1].status)==12);branches+=chain.branches.size();
    for(const auto& branch:chain.branches){check(branch.virtuality_squared>0&&std::isfinite(branch.spacelike_daughter.energy));negative_energy_count+=branch.spacelike_daughter.energy<0;}
   }
   check(branches==a->isr_emissions.size());
   auto malformed=*a;malformed.particles[chains[0].beam-1].daughters.clear();bool bad_chain=false;
   try{jewel::map_pythia_isr_ancestry(malformed);}catch(const std::domain_error&){bad_chain=true;}check(bad_chain);
   std::array<double,4> total{};
   for(const auto& p:a->particles)if(p.final){total[0]+=p.momentum.px;total[1]+=p.momentum.py;total[2]+=p.momentum.pz;total[3]+=p.momentum.energy;}
   for(int j=0;j<3;++j)check(std::abs(total[j])<1e-8*config.ecm);
   check(std::abs(total[3]-config.ecm)<1e-8*config.ecm);
   if(!a->isr_emissions.empty()){
    auto broken=*a;broken.isr_final[0]=a->hard_final[0];bool rejected=false;
    try{jewel::validate_hard_snapshot(broken);}catch(const std::domain_error&){rejected=true;}check(rejected);
    broken=*a;broken.isr_emissions.pop_back();broken.isr_final.pop_back();rejected=false;
    try{jewel::validate_hard_snapshot(broken);}catch(const std::domain_error&){rejected=true;}check(rejected);
   }
   jewel::BranchingSampler sampler({},1,1,0);jewel::SplitMixRandom stream(2600100+i);bool rejected=false;
   try{jewel::prepare_pythia_dijet(*a,{},sampler,stream);}catch(const std::domain_error&){rejected=true;}
   check(rejected&&stream.checkpoint()==0);continue;
  }
  jewel::BranchingSampler sampler({},1,1,0);jewel::SplitMixRandom stream(2600100+i);
  jewel::HardInitializationConfig hard;hard.x=1.2;hard.y=-.7;
  auto prepared=jewel::prepare_pythia_dijet(*a,hard,sampler,stream);prepared.state.validate();
  check(prepared.state.at(prepared.hard_process).particle.status==17&&!prepared.state.at(prepared.hard_process).parent);
  for(std::size_t leg=0;leg<2;++leg){check(prepared.input_sources.at(prepared.hard[leg])==a->hard_final[leg]);check(prepared.state.at(prepared.hard[leg]).particle.production.x==1.2);}
  std::array<double,4> before{},after{};
  for(const auto& p:a->particles)if(p.final){before[0]+=p.momentum.px;before[1]+=p.momentum.py;before[2]+=p.momentum.pz;before[3]+=p.momentum.energy;}
  for(const auto& p:prepared.state.particles())if(p.particle.status==1||p.particle.status==4){after[0]+=p.particle.momentum.px;after[1]+=p.particle.momentum.py;after[2]+=p.particle.momentum.pz;after[3]+=p.particle.momentum.energy;}
  for(int j=0;j<4;++j)check(std::abs(before[j]-after[j])<1e-7);
  auto wrong_copy=*a;wrong_copy.hard_final[0]=wrong_copy.hard_outgoing[0];bool copy_rejected=false;
  try{jewel::validate_hard_snapshot(wrong_copy);}catch(const std::domain_error&){copy_rejected=true;}check(copy_rejected);
  auto broken=*a;broken.hard_outgoing[1]=broken.hard_outgoing[0];bool rejected=false;
  try{jewel::validate_hard_snapshot(broken);}catch(const std::domain_error&){rejected=true;}check(rejected);
 }
 auto s=first.statistics();check(accepted>0&&s.attempts==20&&s.returned==accepted&&s.failed==20-accepted);
 check(s.sigma_mb>0&&std::isfinite(s.sigma_error_mb));
 check(isr?emission_count>0:emission_count==0);
 if(isr)check(negative_energy_count>0);
 }
 for(bool isr:{false,true}){
  jewel::PythiaInputConfig config;config.seed=2600317;
  config.multiple_parton_interactions=true;config.initial_state_radiation=isr;
  jewel::Pythia8Input input(config);unsigned secondary=0;
  for(int i=0;i<20;++i){auto event=input.next();if(!event)continue;
   check(event->multiple_parton_interactions);
   secondary+=event->subsystems.size()-1;
   if(!event->isr_fractions.empty()){
    auto missing=*event;missing.isr_fractions.pop_back();bool rejected=false;
    try{jewel::validate_hard_snapshot(missing);}catch(const std::domain_error&){rejected=true;}check(rejected);
    auto duplicate=*event;duplicate.isr_fractions.push_back(duplicate.isr_fractions[0]);rejected=false;
    try{jewel::validate_hard_snapshot(duplicate);}catch(const std::domain_error&){rejected=true;}check(rejected);
    auto unchanged=*event;unchanged.isr_fractions[0].after=unchanged.isr_fractions[0].before;rejected=false;
    try{jewel::validate_hard_snapshot(unchanged);}catch(const std::domain_error&){rejected=true;}check(rejected);
   }
   auto chains=jewel::map_subsystem_isr_ancestry(*event);check(chains.size()==event->subsystems.size());
   std::size_t branches=0;for(std::size_t j=0;j<chains.size();++j)for(std::size_t side=0;side<2;++side){
    check(chains[j][side].backward_lines.back()==event->subsystems[j].incoming[side]);branches+=chains[j][side].branches.size();}
   check(branches==event->isr_emissions.size());
   auto recoil_frames=jewel::reconstruct_subsystem_isr_frames(*event);check(recoil_frames.size()==chains.size());
   for(std::size_t j=0;j<chains.size();++j)check(recoil_frames[j].size()==chains[j][0].branches.size()+chains[j][1].branches.size());
   for(const auto& fs:recoil_frames)if(!fs.empty()){
    auto corrupt=*event;auto id=fs[0].copies[0][1];*corrupt.particles[id-1].energy_before_beam_repair+=1.;bool rejected=false;
    try{jewel::reconstruct_subsystem_isr_frames(corrupt);}catch(const std::domain_error&){rejected=true;}check(rejected);break;}
   auto bad_chain=*event;bad_chain.particles[chains.back()[0].hard_incoming-1].mothers.clear();bool bad_ancestry=false;
   try{jewel::map_subsystem_isr_ancestry(bad_chain);}catch(const std::domain_error&){bad_ancestry=true;}check(bad_ancestry);
   if(event->subsystems.size()>1){auto wrong_system=*event;
    std::swap(wrong_system.subsystems[0].incoming,wrong_system.subsystems[1].incoming);bool rejected=false;
    try{jewel::map_subsystem_isr_ancestry(wrong_system);}catch(const std::domain_error&){rejected=true;}check(rejected);}
   if(!event->isr_emissions.empty()){auto missing_energy=*event;
    missing_energy.particles[event->isr_emissions[0]-1].energy_before_beam_repair.reset();bool rejected=false;
    try{jewel::map_subsystem_isr_ancestry(missing_energy);}catch(const std::domain_error&){rejected=true;}check(rejected);}
   auto frames=jewel::reconstruct_subsystem_primordial_frames(*event);check(frames.size()==event->subsystems.size());
   for(std::size_t j=0;j<frames.size();++j){check(frames[j].incoming_after==event->subsystems[j].incoming);
    check(frames[j].copies.size()==event->subsystems[j].outgoing.size()+2);}
   auto missing=*event;missing.subsystems.back().primordial_targets.reset();bool no_targets=false;
   try{jewel::reconstruct_subsystem_primordial_frames(missing);}catch(const std::domain_error&){no_targets=true;}check(no_targets);
   missing=*event;missing.particles[event->subsystems.back().outgoing[0]-1].energy_before_beam_repair.reset();bool no_energy=false;
   try{jewel::reconstruct_subsystem_primordial_frames(missing);}catch(const std::domain_error&){no_energy=true;}check(no_energy);
   auto bad_frame=*event;bad_frame.particles[event->subsystems.back().outgoing[0]-1].momentum.energy+=1;
   bool bad_recoil=false;try{jewel::reconstruct_subsystem_primordial_frames(bad_frame);}catch(const std::domain_error&){bad_recoil=true;}check(bad_recoil);
   std::array<double,4> total{};
   for(const auto& p:event->particles)if(p.final){total[0]+=p.momentum.px;total[1]+=p.momentum.py;total[2]+=p.momentum.pz;total[3]+=p.momentum.energy;}
   for(int j=0;j<4;++j)check(std::abs(total[j]-(j==3?config.ecm:0))<1e-8*config.ecm);
   auto rejects=[](const jewel::HardEventSnapshot& broken){bool rejected=false;
    try{jewel::validate_hard_snapshot(broken);}catch(const std::domain_error&){rejected=true;}check(rejected);};
   auto broken=*event;broken.subsystems.clear();rejects(broken);
   broken=*event;broken.particles[broken.subsystems.back().process_outgoing[0]-1].momentum.energy+=1;rejects(broken);
   broken=*event;broken.particles[broken.subsystems.back().process_outgoing[0]-1].mothers.clear();rejects(broken);
   broken=*event;broken.subsystems[0].process_outgoing[1]=broken.subsystems[0].process_outgoing[0];rejects(broken);
   broken=*event;broken.subsystems[0].outgoing_final[0]=0;rejects(broken);
   if(event->subsystems.size()>1){broken=*event;broken.multiple_parton_interactions=false;rejects(broken);
    broken=*event;broken.subsystems[1].process_outgoing=broken.subsystems[0].process_outgoing;rejects(broken);}
   jewel::BranchingSampler sampler({},1,1,0);jewel::SplitMixRandom stream(2600100+i);bool rejected=false;
   try{jewel::prepare_pythia_dijet(*event,{},sampler,stream);}catch(const std::domain_error&){rejected=true;}
   check(rejected&&stream.checkpoint()==0);
   rejected=false;try{jewel::map_pythia_isr_ancestry(*event);}catch(const std::domain_error&){rejected=true;}check(rejected);
   rejected=false;try{jewel::reconstruct_primordial_recoil_frame(*event);}catch(const std::domain_error&){rejected=true;}check(rejected);
  }
  check(secondary>0);
 }

}
