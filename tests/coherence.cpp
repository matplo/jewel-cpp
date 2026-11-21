// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/coherence.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
void check(bool value) {if(!value)throw std::runtime_error("coherence contract failed");}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::domain_error&){caught=true;}check(caught);}
double mass2(jewel::Momentum p){return p.energy*p.energy-p.px*p.px-p.py*p.py-p.pz*p.pz;}
void merge_contracts(){
 jewel::ShowerState state;jewel::ShowerParticle p;p.particle.status=11;p.particle.pdg=21;
 p.particle.momentum={0,0,20,std::sqrt(416.)};p.record_mass=4;p.fraction=.4;p.angle=.3;p.decay_time=7;
 auto root=state.append(p);p.parent=root;auto mother=state.append(p);
 p.parent=mother;p.particle.status=1;p.particle.production={1,2,3,.4};p.particle.momentum={1,0,10,11};
 auto first=state.append(p),second=state.append(p);state.connect(mother,first,second);
 auto baseline=state.snapshot();
 auto start=jewel::begin_coherent(state,second,true,1,10,.3);state.validate();
 check(start.active&&start.duration==6&&start.remaining==10);
 check(start.first==first&&start.second==second);
 check(state.at(start.coherent).particle.parents==std::vector<jewel::ParticleId>{first,second});
 check(state.at(start.coherent).parent==first&&state.at(start.coherent).coherent_partner==second);
 check(state.at(first).daughters==std::array<jewel::ParticleId,2>{start.coherent,start.coherent});
 check(state.at(second).daughters==state.at(first).daughters);
 check(state.at(first).decay_time==.4&&state.at(second).decay_time==.4);
 auto merged=state.snapshot();auto corrupt=merged;
 corrupt.particles[3].parent=start.coherent;corrupt.particles[3].particle.parents={start.coherent};
 bool cycle=false;try{state.restore(corrupt);}catch(const std::domain_error& e){cycle=std::string(e.what())=="cyclic shower ancestry";}
 check(cycle);state.validate();check(state.at(start.coherent).coherent_partner==second);
 corrupt=merged;corrupt.particles[3].daughters={0,0};rejects([&]{state.restore(corrupt);});state.validate();
 corrupt=merged;corrupt.particles.back().particle.parents={first};rejects([&]{state.restore(corrupt);});
 state.restore(baseline);bool removed=false;try{state.at(start.coherent);}catch(const std::out_of_range&){removed=true;}check(removed);
 auto next=jewel::begin_coherent(state,first,true,1,1,.3);check(next.coherent>start.coherent);state.validate();
 // A restored record ordering, not numeric IDs, determines sib1 and sib2.
 std::swap(baseline.particles[2],baseline.particles[3]);state.restore(baseline);
 auto reordered=jewel::begin_coherent(state,first,true,1,1,.3);
 check(reordered.first==second&&reordered.second==first);state.validate();
 state.restore(baseline);state.at(first).daughters={second,second};rejects([&]{state.validate();});
 state.restore(baseline);auto inactive=jewel::begin_coherent(state,first,false,1,1,.3);
 check(!inactive.active&&state.particles().size()==4);state.validate();
}
int main(){
 merge_contracts();
 jewel::Momentum p{0,0,20,std::sqrt(416.)};
 jewel::TransferRecord q;q.invariant=-double(.2f);q.momentum={0,0,99,0};
 auto boundary=jewel::coherent_resolution(p,q,1,1,1,0);
 check(!boundary.resolved&&boundary.transverse_transfer==boundary.threshold);
 q.invariant=-std::nextafter(double(.2f),1.);
 check(jewel::coherent_resolution(p,q,1,1,1,0).resolved);
 auto zero=jewel::coherent_resolution(p,q,1,1,0,0);
 check(!zero.resolved&&std::isinf(zero.threshold));
 // Stock ignores qz here; ensure a future cleanup cannot silently replace its mapping.
 q.momentum.pz=-1000;
 check(jewel::coherent_resolution(p,q,1,1,1,0).transverse_transfer==std::abs(q.invariant));
 rejects([&]{jewel::coherent_resolution({},q,1,1,1,0);});
 auto result=jewel::reconstruct_coherent(p,{1,2,3,.4},4,.3,.5,.4,.7);
 check(result.possible);
 check(std::abs(mass2(result.first)-.09)<1e-10);
 check(std::abs(mass2(result.second)-.25)<1e-10);
 check(std::abs(result.first.energy+result.second.energy-p.energy)<1e-12);
 check(std::abs(result.first.pz+result.second.pz-p.pz)<1e-12);
 check(result.first.px+result.second.px==0&&result.first.py+result.second.py==0);
 auto rejected=jewel::reconstruct_coherent(p,{},4,30,30,.5,.7);
 check(!rejected.possible&&rejected.transverse_squared<0);
 rejects([&]{jewel::reconstruct_coherent(p,{},4,0,0,0,0);});
 rejects([&]{jewel::reconstruct_coherent(p,{},4,-1,0,.5,0);});
 rejects([&]{jewel::reconstruct_coherent({0,0,0,4},{},4,0,0,.5,0);});
 rejects([&]{jewel::reconstruct_coherent(p,{},4,0,0,.5,std::numeric_limits<double>::quiet_NaN());});
}
