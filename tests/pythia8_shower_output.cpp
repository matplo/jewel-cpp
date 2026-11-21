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
}
