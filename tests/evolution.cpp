// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/evolution.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("evolution contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::Kernels kernels;jewel::ExpandingMedium medium;
 auto maximum=medium.maximum_scatterer(false);
 jewel::InitialSudakovTable initial(kernels,1,20,1,maximum);
 jewel::FinalSplittingTable final(kernels,1,20,1,maximum);
 jewel::BranchingSampler branching(kernels,1,1,0);
 jewel::ScatteringRates rates(kernels,1,20,1,medium,false);
 jewel::TransferSampler transfers(kernels,medium,false);
 jewel::EvolutionConfig config;config.cross_section_scale=0;
 jewel::ScatteringEvolution evolution(config,branching,initial,final,rates,transfers);
 config.cutoff=0;rejects([&]{jewel::ScatteringEvolution bad(config,branching,initial,final,rates,transfers);});
 jewel::ShowerState state;jewel::ShowerParticle temp,leg;
 leg.particle.status=1;leg.particle.pdg=21;leg.particle.momentum={0,0,10,10};leg.particle.production.t=.4;
 auto scratch=state.append(temp),id=state.append(leg);
 jewel::TransferHistory history;jewel::ReplayRandom empty({});
 rejects([&]{evolution.final_state(state,id,scratch,history,1,1,{},empty);});
 history.first=1;history.last=1;history.records.resize(1);
 history.records[0].invariant=-.2;history.records[0].momentum={std::sqrt(.1979),0,-.11,-.1};
 history.sum=history.records[0].momentum;history.sum_squared=-.2;
 rejects([&]{evolution.final_state(state,id,id,history,1,1,{},empty);});
 rejects([&]{evolution.final_state(state,id,scratch,history,1,-1,{},empty);});
 jewel::ReplayRandom random({.5});jewel::FinalScatteringResult input;input.fraction=.37;input.quark_pair=true;
 auto result=evolution.final_state(state,id,scratch,history,1,2,input,random);
 check(result.mass==0&&result.elapsed==0&&result.fraction==.37&&result.quark_pair&&!result.above_cutoff);
 check(history.first==1&&history.last==1&&history.sum_squared==-.2&&random.checkpoint()==1);
 check(state.at(id).particle.momentum.energy==10&&!state.discarded);
}
