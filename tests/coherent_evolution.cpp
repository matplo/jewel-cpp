// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/coherence.hpp"
#include "jewel/kinematics.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("coherent evolution contract failed");}
int main(){
 jewel::Kernels kernels;jewel::ExpandingMedium medium;
 jewel::ScatteringRates rates(kernels,1,30,1,medium,false);
 jewel::TransferSampler transfers(kernels,medium,false);jewel::BranchingSampler branching(kernels,1,1,0);
 jewel::ShowerState state;state.colour_max=110;
 jewel::ShowerParticle scratch,root;root.particle.status=11;root.particle.pdg=21;
 root.particle.momentum={0,0,std::sqrt(384.),20};root.record_mass=4;root.particle.colour=101;root.particle.anticolour=102;
 root.particle.production.t=.4;root.decay_time=8;root.fraction=.4;root.angle=.3;
 auto temp=state.append(scratch),ancestor=state.append(root);root.parent=ancestor;auto mother=state.append(root);
 auto momenta=jewel::reconstruct_coherent(root.particle.momentum,{},4,.3,.5,.4,0);
 auto first=root,second=root;first.parent=mother;second.parent=mother;first.particle.status=second.particle.status=1;
 first.particle.momentum=momenta.first;second.particle.momentum=momenta.second;first.record_mass=.3;second.record_mass=.5;
 first.evolution_time=.7;second.evolution_time=.8;
 auto a=state.append(first),b=state.append(second);state.connect(mother,a,b);auto before=state.snapshot();
 jewel::TransferHistory history;jewel::RecoilStorage ledger;jewel::ReplayRandom random({});jewel::CoherentHistograms histograms;
 jewel::CoherentConfig config;config.cross_section_scale=0;
 auto result=jewel::evolve_coherent(state,a,temp,history,ledger,1,.5,.3,.4,config,rates,transfers,branching,medium,random,&histograms);
 state.validate();check(state.particles().size()==8&&random.checkpoint()==0&&ledger.used==0);
 check(result.initial.active&&result.collision_calls==1&&result.kinematic_calls==0);
 auto coherent=result.initial.coherent;auto daughters=state.at(coherent).daughters;
 check(result.selected==daughters[0]&&state.at(coherent).particle.status==21&&state.at(coherent).decay_time==1.5);
 check(state.at(coherent).particle.parents==std::vector<jewel::ParticleId>{a,b});
 const auto& x=state.at(daughters[0]);const auto& y=state.at(daughters[1]);
 check(x.evolution_time==.7&&y.evolution_time==1.5&&x.angular_required&&y.angular_required);
 check(x.decay_time==8&&y.decay_time==8&&x.particle.production.t==1.5);
 check(std::abs(x.particle.momentum.energy+y.particle.momentum.energy-20)<1e-12);
 auto expected=jewel::position_at(first.particle.momentum,first.particle.production,1.5);
 check(std::abs(x.particle.production.x-expected.x)<1e-12&&std::abs(x.particle.production.z-expected.z)<1e-12);
 for(double value:histograms.time.values)check(value==0);
 // Invalid final kinematics must leave a restorable caller-owned snapshot.
 state.restore(before);state.at(a).record_mass=10;state.at(b).record_mass=10;
 bool rejected=false;try{jewel::evolve_coherent(state,a,temp,history,ledger,1,.5,.3,.4,config,rates,transfers,branching,medium,random);}
 catch(const std::domain_error&){rejected=true;}check(rejected);
 state.restore(before);state.validate();check(state.particles().size()==5);
 config.enabled=false;
 auto inactive=jewel::evolve_coherent(state,a,temp,history,ledger,1,.5,.3,.4,config,rates,transfers,branching,medium,random);
 check(!inactive.initial.active&&inactive.selected==a&&state.particles().size()==5&&random.checkpoint()==0);
}
