// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/collision.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("collision contract failed");}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(const std::domain_error&){bad=true;}check(bad);}
int main(){
 jewel::Kernels k;jewel::BranchingSampler sampler(k,1,1,0);jewel::Vacuum medium;
 jewel::ShowerParticle leg,scratch;leg.particle.status=1;leg.particle.pdg=1;leg.particle.momentum={0,0,20,20};
 jewel::TransferRecord transfer;transfer.invariant=-.2;transfer.scatterer={21,{0,0,0,1},1};
 jewel::ReplayRandom random({.3});
 auto r=jewel::scatter_pair(leg,scratch,transfer,-.2,0,0,0,{},sampler,medium,random);
 check(!r.rejected&&r.reshuffled&&!r.split_recoil&&random.checkpoint()==1);
 check(r.incoming.particle.status==12&&r.outgoing.particle.status==1&&r.recoil.particle.status==3);
 auto p=r.outgoing.particle.momentum,q=r.recoil.particle.momentum;
 check(std::abs(p.px+q.px)<1e-12&&std::abs(p.py+q.py)<1e-12&&std::abs(p.pz+q.pz-20)<1e-12&&std::abs(p.energy+q.energy-21)<1e-12);
 check(leg.particle.status==1&&leg.particle.momentum.energy==20); // Value inputs are rollback-safe.
 jewel::ReplayRandom empty({});transfer.invariant=0;
 r=jewel::scatter_pair(leg,scratch,transfer,0,0,0,0,{},sampler,medium,empty);check(r.rejected&&empty.checkpoint()==0);
 transfer.invariant=1;rejects([&]{jewel::scatter_pair(leg,scratch,transfer,1,0,0,0,{},sampler,medium,empty);});
 transfer.invariant=-1;
 rejects([&]{jewel::scatter_pair(leg,scratch,transfer,-1,0,0,0,{1,1,4},sampler,medium,empty);});
 rejects([&]{jewel::scatter_pair(leg,scratch,transfer,-1,0,0,0,{std::numeric_limits<double>::quiet_NaN(),1,0},sampler,medium,empty);});
}
