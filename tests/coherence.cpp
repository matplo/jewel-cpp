// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/coherence.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
void check(bool value) {if(!value)throw std::runtime_error("coherence contract failed");}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::domain_error&){caught=true;}check(caught);}
double mass2(jewel::Momentum p){return p.energy*p.energy-p.px*p.px-p.py*p.py-p.pz*p.pz;}
int main(){
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
