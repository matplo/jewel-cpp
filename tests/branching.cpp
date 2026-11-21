// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/branching.hpp"
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("branching contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::Kernels k;jewel::BranchingSampler sampler(k,1,1,3);
 jewel::ReplayRandom empty({});
 auto no_phase=sampler.mass(2,1,-1,100,jewel::Channel::QQ,100,false,empty);
 check(no_phase.mass==0&&no_phase.fraction==0&&no_phase.quark_pair==false);
 check(sampler.fraction(0,0,.6,jewel::Channel::QG,empty)==.5);
 jewel::ReplayRandom qg({.5,.5,.1}),gq({.5,.5,.1});
 check(sampler.fraction(0,0,.1,jewel::Channel::QG,qg)==sampler.fraction(0,0,.1,jewel::Channel::GQ,gq));
 check(qg.checkpoint()==gq.checkpoint());
 jewel::ReplayRandom narrow({.00001});
 auto threshold=sampler.mass(1,1.000001,-1,100,jewel::Channel::GC,100,false,narrow);
 check(threshold.mass==1&&threshold.fraction==.5&&threshold.quark_pair==true&&narrow.checkpoint()==1);
 jewel::ReplayRandom initial_rng({.9,.00001});
 auto initial=sampler.mass(0,20,-1,100,jewel::Channel::QQ,100,true,initial_rng);
 check(initial.mass>0&&!initial.fraction.has_value()&&!initial.quark_pair.has_value());
 check(initial_rng.checkpoint()==2);
 jewel::ReplayRandom fraction_rng({.5,.5,.1});
 check(sampler.fraction(0,0,.5,jewel::Channel::QG,fraction_rng)==.5&&fraction_rng.checkpoint()==3);
 rejects([&]{sampler.mass(0,20,1,100,jewel::Channel::QQ,100,true,empty);});
 rejects([&]{sampler.mass(0,20,-1,100,jewel::Channel::QQ,100,false,empty);});
 rejects([&]{sampler.fraction(0,0,0,jewel::Channel::QQ,empty);});
 rejects([&]{sampler.fraction(.5,100,.1,jewel::Channel::QQ,empty);});
 rejects([&]{sampler.fraction(10,1,.1,jewel::Channel::QQ,empty);});
 rejects([&]{jewel::BranchingSampler bad(k,1,.01,0);});
}
