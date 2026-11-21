// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/collision.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("collision contract failed");}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(const std::domain_error&){bad=true;}check(bad);}
int main(){
 jewel::Kernels k;jewel::BranchingSampler sampler(k,1,1,0);jewel::Vacuum medium;
 jewel::ShowerParticle leg,scratch;leg.particle.status=1;leg.particle.pdg=1;leg.particle.momentum={0,0,20,20};leg.particle.colour=10;leg.particle.production.t=.4;
 jewel::TransferRecord transfer;transfer.invariant=-.2;transfer.position={0,0,0,1};transfer.scatterer={21,{0,0,0,1},1};
 jewel::ReplayRandom random({.3});
 auto r=jewel::scatter_pair(leg,scratch,transfer,-.2,0,0,0,{},sampler,medium,random);
 check(!r.rejected&&r.reshuffled&&!r.split_recoil&&random.checkpoint()==1);
 check(r.incoming.particle.status==12&&r.outgoing.particle.status==1&&r.recoil.particle.status==3);
 auto p=r.outgoing.particle.momentum,q=r.recoil.particle.momentum;
 check(std::abs(p.px+q.px)<1e-12&&std::abs(p.py+q.py)<1e-12&&std::abs(p.pz+q.pz-20)<1e-12&&std::abs(p.energy+q.energy-21)<1e-12);
 check(leg.particle.status==1&&leg.particle.momentum.energy==20); // Value inputs are rollback-safe.
 jewel::ReplayRandom empty({});
 jewel::RecoilStorage ledger;ledger.capacity=3;int colour_max=20;
 jewel::CollisionFinalizationConfig finish;finish.storage={0,true,false};
 jewel::CollisionStepTiming timing;timing.fraction=.4;timing.first_decay_time=7;timing.transfer_time=1.1;
 jewel::finish_collision(r,colour_max,ledger,finish,timing,medium,empty);
 check(colour_max==21&&ledger.used==1&&empty.checkpoint()==0);
 check(r.outgoing.particle.colour==21&&r.recoil.particle.colour==10&&r.recoil.particle.anticolour==21);
 check(r.recoil.particle.status==2&&r.incoming.decay_time==1.1&&r.outgoing.decay_time==0);
 check(r.outgoing.particle.production.t==1&&std::abs(r.outgoing.particle.production.z-.6)<1e-15);
 check(r.recoil.particle.production.z==r.outgoing.particle.production.z);
 check(r.outgoing.fraction==.4&&ledger.records[0].flavour==21);
 finish.formation_factor=0;rejects([&]{jewel::finish_collision(r,colour_max,ledger,finish,timing,medium,empty);});
 finish.formation_factor=1;timing.fraction=1;rejects([&]{jewel::finish_collision(r,colour_max,ledger,finish,timing,medium,empty);});
 transfer.invariant=0;
 r=jewel::scatter_pair(leg,scratch,transfer,0,0,0,0,{},sampler,medium,empty);check(r.rejected&&empty.checkpoint()==0);
 transfer.invariant=1;rejects([&]{jewel::scatter_pair(leg,scratch,transfer,1,0,0,0,{},sampler,medium,empty);});
 transfer.invariant=-1;
 rejects([&]{jewel::scatter_pair(leg,scratch,transfer,-1,0,0,0,{1,1,4},sampler,medium,empty);});
 rejects([&]{jewel::scatter_pair(leg,scratch,transfer,-1,0,0,0,{std::numeric_limits<double>::quiet_NaN(),1,0},sampler,medium,empty);});
}
