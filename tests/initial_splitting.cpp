// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/shower.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("initial splitting contract failed");}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad);}
int main(){
 jewel::BranchingSampler sampler({},1,1,0);jewel::ShowerState state;state.colour_max=20;
 jewel::ShowerParticle leg;leg.particle.status=1;leg.particle.pdg=1;leg.particle.colour=10;
 leg.particle.momentum={0,0,20,20};leg.particle.production={1,2,3,.4};leg.decay_time=7;
 auto id=state.append(leg);auto baseline=state.snapshot();
 jewel::InitialSplitRequest request;request.fraction=.4;request.transfer_sum=-4;request.virtuality=4;
 request.incoming_flavour=1;request.time=4;request.radiation_time=.7;
 jewel::ReplayRandom random({.000001,.3});
 auto result=jewel::make_initial_splitting(state,id,request,sampler,random);state.validate();
 check(state.particles().size()==3&&result.fraction==.4&&random.checkpoint()==2);
 const auto& c=state.at(result.radiated);const auto& b=state.at(result.continuing);
 check(b.record_mass==-4&&c.record_mass==0&&state.at(id).particle.status==11);
 check(c.particle.pdg==21&&b.particle.pdg==1&&c.parent==id&&b.parent==id);
 check(c.particle.anticolour==b.particle.colour&&state.colour_max==21);
 check(c.particle.production.t==b.particle.production.t&&std::abs(c.particle.production.t-3.3)<1e-15);
 check(std::abs(c.particle.production.z-5.9)<1e-14&&b.decay_time==0&&c.decay_time==0);
 check(std::abs(c.particle.momentum.pz+b.particle.momentum.pz-20)<1e-14);
 state.restore(baseline);state.at(id).particle.momentum.pz=std::sqrt(336.);state.at(id).record_mass=8;
 request.transfer_sum=0;request.virtuality=0;request.fraction=.05;jewel::ReplayRandom empty({});
 result=jewel::make_initial_splitting(state,id,request,sampler,empty);
 check(result.fraction==1&&result.radiated==0&&state.particles().size()==1&&state.at(id).decay_time==7&&empty.checkpoint()==0);
 request.formation_factor=0;rejects([&]{jewel::make_initial_splitting(state,id,request,sampler,empty);});
 request.formation_factor=1;request.transfer_sum=1;rejects([&]{jewel::make_initial_splitting(state,id,request,sampler,empty);});
 request.transfer_sum=0;
 while(state.particles().size()<22990)state.append(leg);
 result=jewel::make_initial_splitting(state,id,request,sampler,empty);
 check(state.discarded&&result.radiated==0&&state.particles().size()==22990&&empty.checkpoint()==0);
}
