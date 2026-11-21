// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/core.hpp"
#include <cmath>
#include <stdexcept>
#include <limits>
void check(bool ok) { if(!ok) throw std::runtime_error("contract failed"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught); }
int main() {
 jewel::Event event;
 auto first=event.append(21,{0,0,10,10},{1,2,0,0});
 for(int i=0;i<2000;++i) event.append(21,{1,0,0,1},{},{first});
 check(event.particles().front().id==first);
 check(event.particles().back().parents.front()==first);
 rejects([&]{event.append(1,{}, {}, {99999});});
 rejects([&]{event.append(1,{0,0,0,-1},{});});
 jewel::ReplayRandom rng({0.9,0.99,0.2,0.01});
 auto checkpoint=rng.checkpoint(); check(rng.uniform()==0.9); rng.restore(checkpoint);
 check(rng.uniform()==0.9); rejects([&]{rng.restore(5);});
 rejects([]{jewel::ReplayRandom r({0});});
 jewel::ReplayRandom empty({}); rejects([&]{empty.uniform();});
 jewel::Kernels k;
 check(k.gett(2,1,0.5,empty)==0); check(empty.checkpoint()==0);
 check(k.gett(1,1,0.5,rng)==1); check(rng.checkpoint()==3);
 for(double z: {0.,1.,-0.1,1.1,std::numeric_limits<double>::quiet_NaN()}) {
  rejects([&]{k.pqq(z);}); rejects([&]{k.pgq(z);});
  rejects([&]{k.pgg(z);}); rejects([&]{k.pqg(z);});
  rejects([&]{k.intpqq(z,10);}); rejects([&]{k.intpgglow(z,10);});
  rejects([&]{k.intpgghigh(z,10);});
 }
 rejects([&]{k.alphas(0.04,0.2);}); rejects([&]{k.alphas(1,-0.2);});
 rejects([&]{k.intpgglow(0.5,0.01);});
 rejects([&]{k.gett(-1,1,0.5,rng);});
 k.nf=7; rejects([&]{k.alphas(1,0.2);});
 auto vacuum=jewel::Vacuum{}.sample({1,2,3,4});
 check(vacuum.temperature==0 && vacuum.density==0 && vacuum.debye_mass==0);
}
