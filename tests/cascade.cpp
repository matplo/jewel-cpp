// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/cascade.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("cascade contract failed");}
int main(){
 jewel::Kernels kernels;jewel::ExpandingMedium medium;auto maximum=medium.maximum_scatterer(false);
 jewel::InitialSudakovTable initial(kernels,1,30,1,maximum);jewel::FinalSplittingTable final(kernels,1,30,1,maximum);
 jewel::PartonicPdf pdf(kernels,1,30,1,maximum);jewel::ScatteringRates rates(kernels,1,30,1,medium,false);
 jewel::TransferSampler transfers(kernels,medium,false);jewel::BranchingSampler branching(kernels,1,1,0);
 jewel::InitialEvolutionConfig config;config.evolution.cross_section_scale=0;config.coherent.enabled=false;
 jewel::InitialScatteringEvolution initial_evolution(config,kernels,branching,initial,final,pdf,rates,transfers,medium);
 jewel::ScatteringEvolution final_evolution(config.evolution,branching,initial,final,rates,transfers);
 jewel::CascadeConfig cascade_config;cascade_config.evolution=config.evolution;
 jewel::Cascade cascade(cascade_config,initial_evolution,final_evolution,branching,medium);
 jewel::ShowerState state;jewel::ShowerParticle scratch,parton;
 auto temp=state.append(scratch);parton.particle.status=1;parton.particle.pdg=21;parton.particle.momentum={0,0,10,10};
 parton.particle.production.t=-.5;parton.evolution_time=-1e6;
 auto first=state.append(parton);parton.particle.status=2;auto second=state.append(parton);
 jewel::TransferHistory history;history.records.resize(100);jewel::RecoilStorage ledger;jewel::ReplayRandom random({});
 cascade.branch(state,first,temp,history,ledger,random);
 check(state.at(first).particle.status==4&&state.at(first).evolution_time==20&&random.checkpoint()==0);
 auto result=cascade.evolve(state,temp,history,ledger,random);
 check(result.branch_calls==1&&result.passes==2&&state.at(second).particle.status==5);
 check(history.last==0&&ledger.used==0);state.validate();
 bool rejected=false;try{cascade.branch(state,temp,temp,history,ledger,random);}catch(const std::domain_error&){rejected=true;}check(rejected);
 // Diagnostic listing has a physical auxiliary-record side effect (V=MV).
 state.at(first).particle.status=4;state.at(first).evolution_time=.2;
 state.at(first).particle.production={1,2,3,.4};state.at(second).particle.production={4,5,6,.7};
 cascade.branch(state,first,temp,history,ledger,random);
 check(state.at(first).generator_vertex.x==1&&state.at(second).generator_vertex.t==.7);
 // Infinite/no-progress source histories must fail explicitly, never report a
 // completed shower. Two active legs exceed an intentionally tiny call budget.
 auto active=parton;active.particle.status=1;active.evolution_time=0;state.append(active);state.append(active);
 cascade_config.maximum_branch_calls=1;
 jewel::Cascade limited(cascade_config,initial_evolution,final_evolution,branching,medium);
 rejected=false;try{limited.evolve(state,temp,history,ledger,random);}catch(const std::runtime_error&){rejected=true;}check(rejected);
 state.validate();
}
