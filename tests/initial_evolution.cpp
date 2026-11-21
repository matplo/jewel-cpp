// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/initial_evolution.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("initial evolution contract failed");}
int main(){
 jewel::Kernels kernels;jewel::ExpandingMedium medium;auto maximum=medium.maximum_scatterer(false);
 jewel::InitialSudakovTable initial(kernels,1,30,1,maximum);jewel::FinalSplittingTable final(kernels,1,30,1,maximum);
 jewel::PartonicPdf pdf(kernels,1,30,1,maximum);jewel::ScatteringRates rates(kernels,1,30,1,medium,false);
 jewel::TransferSampler transfers(kernels,medium,false);jewel::BranchingSampler branching(kernels,1,1,0);
 jewel::ShowerState state;state.colour_max=110;
 jewel::ShowerParticle scratch,root;root.particle.status=11;root.particle.pdg=21;
 root.particle.momentum={0,0,std::sqrt(384.),20};root.record_mass=4;root.particle.colour=101;root.particle.anticolour=102;
 root.particle.production.t=.4;root.decay_time=.4;root.fraction=.4;root.angle=.3;
 auto temp=state.append(scratch),ancestor=state.append(root);root.parent=ancestor;auto mother=state.append(root);
 auto momenta=jewel::reconstruct_coherent(root.particle.momentum,{},4,.3,.5,.4,0);
 auto first=root,second=root;first.parent=mother;second.parent=mother;first.particle.status=second.particle.status=1;
 first.particle.momentum=momenta.first;second.particle.momentum=momenta.second;first.record_mass=.3;second.record_mass=.5;
 first.decay_time=second.decay_time=8;first.evolution_time=.7;second.evolution_time=.8;
 auto a=state.append(first),b=state.append(second);state.connect(mother,a,b);auto before=state.snapshot();
 jewel::InitialEvolutionConfig config;config.evolution.cross_section_scale=0;config.coherent.enabled=false;
 jewel::TransferHistory history;history.first=history.last=1;history.records.resize(1);history.records[0].invariant=-.2;history.sum_squared=-.2;
 jewel::RecoilStorage ledger;jewel::ReplayRandom random({});jewel::CoherentHistograms histograms;
 jewel::InitialScatteringEvolution no_coherence(config,kernels,branching,initial,final,pdf,rates,transfers,medium);
 auto result=no_coherence.sample(state,a,temp,history,ledger,1,.5,false,random,&histograms);
 check(result.selected==a&&result.fraction==1&&result.virtuality==0&&result.elapsed==.5&&!result.above_cutoff&&!result.coherent);
 check(state.particles().size()==5&&random.checkpoint()==0&&history.first==0&&history.last==0&&history.sum_squared==0);
 check(history.records[0].invariant==-.2);state.validate();
 config.coherent.enabled=true;
 jewel::InitialScatteringEvolution with_coherence(config,kernels,branching,initial,final,pdf,rates,transfers,medium);
 result=with_coherence.sample(state,a,temp,history,ledger,1,.5,false,random,&histograms);
 check(result.coherent.has_value()&&result.selected!=a&&state.particles().size()==8&&random.checkpoint()==0);
 check(result.fraction==1&&result.virtuality==0&&result.elapsed==.5&&history.last==0);
 state.validate();state.restore(before);
 // RETRYSPLIT bypasses the no-collision return and still consumes radiation RNG.
 bool exhausted=false;try{no_coherence.sample(state,a,temp,history,ledger,1,.5,true,random);}
 catch(const std::out_of_range&){exhausted=true;}check(exhausted);state.restore(before);state.validate();
 bool rejected=false;try{no_coherence.sample(state,a,temp,history,ledger,1,-1,false,random);}
 catch(const std::domain_error&){rejected=true;}check(rejected);
}
