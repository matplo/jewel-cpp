// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/shower_state.hpp"
#include "jewel/kinematics.hpp"
#include "jewel/shower.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("state/kinematics contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::ShowerState state;jewel::ShowerParticle root;root.particle.pdg=21;root.particle.status=1;
 root.particle.momentum={3,4,5,std::sqrt(54.)};root.record_mass=2;
 auto parent=state.append(root);auto before=state.snapshot();
 auto child=root;child.parent=parent;auto a=state.append(child),b=state.append(child);state.connect(parent,a,b);state.validate();
 auto branched=state.snapshot();state.restore(before);rejects([&]{state.at(a);});
 auto later=state.append(root);check(later>b); // IDs are never recycled after rollback.
 state.restore(branched);check(state.at(parent).daughters[0]==a);
 auto bad=branched;bad.particles[1].particle.id=bad.particles[0].particle.id;
 rejects([&]{state.restore(bad);});check(state.at(parent).daughters[0]==a); // Strong restore guarantee.
 // A new ISR ancestor may be appended after a child.
 jewel::ShowerState isr;auto old=isr.append(root);auto ancestor=isr.append(root);
 isr.at(old).parent=ancestor;isr.at(old).particle.parents={ancestor};isr.validate();
 isr.at(ancestor).parent=old;isr.at(ancestor).particle.parents={old};rejects([&]{isr.validate();});
 jewel::Momentum p={3,4,5,std::sqrt(54.)},original=p;auto production=jewel::SpaceTime{1,2,3,4};
 auto x=production;double invariant=p.energy*p.energy-p.px*p.px-p.py*p.py-p.pz*p.pz;
 check(!jewel::rotate_boost(p,&x,0,0,{.1,.2,.3}));
 check(std::abs(p.energy*p.energy-p.px*p.px-p.py*p.py-p.pz*p.pz-invariant)<1e-12);
 jewel::rotate_boost(p,&x,0,0,{-.1,-.2,-.3});
 check(std::abs(p.px-original.px)<1e-13&&std::abs(p.energy-original.energy)<1e-13);
 check(std::abs(x.x-production.x)<1e-13&&std::abs(x.t-production.t)<1e-13);
 auto position=jewel::position_at({100,0,0,10},{1,2,3,0},5);check(position.x==6&&position.y==2);
 rejects([]{jewel::position_at({},{},1);});
 jewel::BranchingSampler sampler({},1,1,0);jewel::Vacuum vacuum;jewel::ReplayRandom empty({});
 jewel::ShowerState event;auto on_shell=root;on_shell.record_mass=0;on_shell.particle.pdg=1;on_shell.fraction=.5;
 auto leg=event.append(on_shell);
 jewel::make_splitting(event,leg,sampler,vacuum,1,empty);
 check(event.particles().size()==1&&!event.discarded&&empty.checkpoint()==0);
 event.at(leg).record_mass=2;event.at(leg).fraction=0;
 rejects([&]{jewel::make_splitting(event,leg,sampler,vacuum,0,empty);});
 jewel::make_splitting(event,leg,sampler,vacuum,1,empty);
 check(event.discarded&&empty.checkpoint()==0);

}
