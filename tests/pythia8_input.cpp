// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_input.hpp"
#include "jewel/pythia8_isr.hpp"
#include "jewel/pythia8_shower_input.hpp"
#include <cmath>
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
  check(a->initial_state_radiation==isr&&b->initial_state_radiation==isr);
  check(a->isr_emissions==b->isr_emissions&&a->isr_final==b->isr_final);
  emission_count+=a->isr_emissions.size();
  if(isr){
   auto chains=jewel::map_pythia_isr_ancestry(*a);unsigned branches=0;
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
}
