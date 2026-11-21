// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_shower_output.hpp"
#include "jewel/pythia8_hadronizer.hpp"
#include <cmath>
#include <stdexcept>
void check(bool x){if(!x)throw std::runtime_error("shower output contract failed");}
int main(){
 jewel::ShowerState state;state.colour_max=102;state.append({});jewel::ShowerParticle retired;retired.particle.status=11;state.append(retired);jewel::ShowerParticle p;
 p.particle.status=4;p.particle.pdg=1;p.particle.colour=101;p.particle.momentum={10,0,0,10};p.particle.production={1,2,3,4};
 auto a=state.append(p);p.particle.pdg=-1;p.particle.colour=0;p.particle.anticolour=101;p.particle.momentum={-10,0,0,10};auto b=state.append(p);
 auto input=jewel::prepare_vacuum_hadronization(state);check(input.size()==2&&input[0].id==a&&input[1].id==b);
 check(input[0].colour==101&&input[0].vertex_mm[0]==1e-12);check(state.particles().size()==4&&state.at(a).particle.status==4);
 jewel::Pythia8Hadronizer hadronizer({});auto output=hadronizer.hadronize(input);check(output.has_value());
 check(output->input_sources==std::vector<jewel::ParticleId>{a,b});
 state.at(a).particle.status=1;bool rejected=false;
 try{jewel::prepare_vacuum_hadronization(state);}catch(const std::domain_error&){rejected=true;}check(rejected);
 state.at(a).particle.status=4;state.at(a).particle.pdg=21;state.at(a).particle.anticolour=102;state.at(b).particle.pdg=21;state.at(b).particle.colour=102;
 rejected=false;try{jewel::prepare_vacuum_hadronization(state);}catch(const std::domain_error&){rejected=true;}check(rejected);
 // Preserve explicit outgoing junctions without inventing string partners.
 for(int kind:{1,2}){
  jewel::ShowerState junction_state;junction_state.colour_max=103;
  for(int leg=0;leg<3;++leg){jewel::ShowerParticle q;q.particle.status=4;
   q.particle.pdg=(kind==1?1:-1)*(leg==2?1:2);
   if(kind==1)q.particle.colour=101+leg;else q.particle.anticolour=101+leg;
   double phi=leg*2*std::acos(-1.)/3;q.particle.momentum={10*std::cos(phi),10*std::sin(phi),0,10};
   junction_state.append(q);
  }
  std::vector<jewel::GeneratorJunction> junctions{{kind,{101,102,103}}};
  auto particles=jewel::prepare_native_hadronization(junction_state,junctions);check(particles.size()==3);
  auto fragmented=hadronizer.hadronize(particles,junctions);check(fragmented.has_value());
  rejected=false;try{jewel::prepare_native_hadronization(junction_state,{});}catch(const std::domain_error&){rejected=true;}check(rejected);
  junctions[0].colours[2]=102;rejected=false;
  try{jewel::prepare_native_hadronization(junction_state,junctions);}catch(const std::domain_error&){rejected=true;}check(rejected);
 }
}
