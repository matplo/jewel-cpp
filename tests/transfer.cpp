// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/transfer.hpp"
#include <limits>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("transfer contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::TransferSampler sampler({},jewel::ExpandingMedium{},false);
 jewel::ShowerState state;jewel::TransferHistory history;jewel::ReplayRandom empty({});
 sampler.sample(state,2,1,history,10000,0,1,false,empty);
 check(state.discarded&&history.records.empty()&&empty.checkpoint()==0);
 state.discarded=false;jewel::ShowerParticle scratch,leg;
 leg.particle.status=1;leg.particle.pdg=21;leg.particle.momentum={0,0,10,10};leg.particle.production.t=.4;
 auto a=state.append(scratch),b=state.append(leg);
 rejects([&]{sampler.sample(state,b,b,history,0,.8,1,false,empty);});
 rejects([&]{sampler.sample(state,b,a,history,0,.8,1,true,empty);});
 rejects([&]{sampler.sample(state,b,a,history,0,.8,1.1,false,empty);});
 history.sum.energy=std::numeric_limits<double>::infinity();
 rejects([&]{sampler.sample(state,b,a,history,0,.8,1,false,empty);});
 check(empty.checkpoint()==0&&!state.discarded);
}
