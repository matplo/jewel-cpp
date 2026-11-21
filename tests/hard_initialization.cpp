// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/hard_initialization.hpp"
#include <cmath>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("hard initialization contract failed");}
int main(){
 jewel::ShowerState state(-1e6);state.colour_max=101;state.append({});
 jewel::ShowerParticle q;q.particle.status=1;q.particle.pdg=1;q.particle.colour=101;q.particle.momentum={10,0,0,10};
 auto first=state.append(q,true,true);q.particle.pdg=-1;q.particle.colour=0;q.particle.anticolour=101;q.particle.momentum={-10,0,0,10};
 auto second=state.append(q,true,true);auto baseline=state.snapshot();
 jewel::BranchingSampler sampler({},1,1,0);jewel::HardInitializationConfig config;config.x=2;config.y=-3;
 jewel::ReplayRandom no_mass({.5});auto r=jewel::initialize_hard_dijet(state,first,second,config,sampler,no_mass);
 check(no_mass.checkpoint()==1&&r.trials==1);
 check(state.at(first).record_mass==0&&state.at(first).decay_time==20);
 check(state.at(first).particle.production.x==2&&state.at(second).particle.production.y==-3);
 check(state.at(first).evolution_time==-1e6&&state.at(first).fraction==0);
 config.pt_hat=20;state.restore(baseline);jewel::ReplayRandom empty({});bool rejected=false;
 try{jewel::initialize_hard_dijet(state,first,second,config,sampler,empty);}catch(const std::out_of_range&){rejected=true;}
 check(rejected&&state.at(first).particle.production.x==0&&state.at(first).particle.momentum.px==10);
 jewel::SplitMixRandom random(2600100);jewel::initialize_hard_dijet(state,first,second,config,sampler,random);state.validate();
 auto a=state.at(first).particle.momentum,b=state.at(second).particle.momentum;
 check(std::abs(a.energy+b.energy-20)<1e-10&&std::abs(a.px+b.px)<1e-10);
}
