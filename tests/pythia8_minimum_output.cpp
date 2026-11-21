// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_output.hpp"
#include "jewel/pythia8_hadronizer.hpp"
#include <cmath>
#include <map>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("minimum string adapter contract failed");}
int main(){
 jewel::BranchingSampler sampler({},1,1,0);jewel::Pythia8Hadronizer hadronizer({});
 for(int flavour:{1,-1,2101,-2101}){
  jewel::ShowerState state;state.colour_max=102;jewel::ShowerParticle p;p.particle.status=4;p.particle.pdg=flavour;
  p.particle.colour=101;p.particle.momentum={20,0,0,20};p.particle.production={1,2,0,0};
  auto id=state.append(p);jewel::ReplayRandom random({.25});
  auto input=jewel::prepare_minimum_hadronization(state,sampler,random,2);
  check(input.size()==2&&input[0].pdg==flavour&&input[1].pdg==-flavour);
  check(input[0].source_index==0&&input[1].source_index==-1);
  check(input[0].id!=input[1].id&&input[0].vertex_mm[0]==1e-12);
  check(input[1].momentum.pz==10&&input[1].momentum.energy==10);
  std::map<int,int> balance;for(const auto& q:input){if(q.colour)++balance[q.colour];if(q.anticolour)--balance[q.anticolour];}
  for(auto entry:balance)check(entry.second==0);
  check(hadronizer.hadronize(input).has_value());
  check(random.checkpoint()==1&&state.at(id).particle.status==4&&state.particles().size()==1);
 }
 // An isolated gluon produces two quark halves, each with an artificial partner.
 jewel::ShowerState state;state.colour_max=102;jewel::ShowerParticle p;p.particle.status=4;p.particle.pdg=21;
 p.particle.colour=101;p.particle.anticolour=102;p.particle.momentum={20,0,0,20};state.append(p);
 jewel::ReplayRandom random({.5,.8,.1,.25,.75});
 auto input=jewel::prepare_minimum_hadronization(state,sampler,random,2);
 check(input.size()==4&&random.checkpoint()==5);
 double energy=0;int synthetic=0;for(const auto& q:input){energy+=q.momentum.energy;if(q.source_index<0)++synthetic;}
 check(synthetic==2&&std::abs(energy-40)<1e-12);
 check(hadronizer.hadronize(input).has_value());
 // The same scaling must work for a massive parent without changing p4.
 state.at(1).record_mass=.2;state.at(1).particle.momentum.energy=std::sqrt(400.04);
 jewel::ReplayRandom massive_random({.5,.8,.1,.25,.75});
 auto massive=jewel::prepare_minimum_hadronization(state,sampler,massive_random,2);
 for(const auto& q:massive)if(q.source_index>=0)check(std::abs(q.mass-.2*q.momentum.energy/std::sqrt(400.04))<1e-15);
 check(hadronizer.hadronize(massive).has_value());
 state.at(1).particle.status=1;bool rejected=false;
 try{jewel::prepare_minimum_hadronization(state,sampler,random,2);}catch(const std::domain_error&){rejected=true;}
 check(rejected);
}
