// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/collision.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("collision sequence contract failed");}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed);}
int main(){
 jewel::Kernels k;jewel::BranchingSampler sampler(k,1,1,0);jewel::Vacuum medium;
 jewel::ShowerState state;state.colour_max=20;
 jewel::ShowerParticle scratch,leg;leg.particle.status=1;leg.particle.pdg=1;leg.particle.colour=10;
 leg.particle.momentum={0,0,std::sqrt(396.),20};leg.record_mass=2;leg.particle.production.t=.4;leg.decay_time=7;leg.fraction=.4;
 auto tmp=state.append(scratch),id=state.append(leg);auto before=state.snapshot();
 jewel::TransferHistory history;history.records.resize(2);
 for(int i=0;i<2;++i){auto& r=history.records[i];r.invariant=-.2;r.time=1.2+.3*i;r.position={0,0,0,1.2+.3*i};r.scatterer={21,{0,0,0,1},1};}
 jewel::RecoilStorage ledger;ledger.capacity=3;
 jewel::CollisionSequenceRequest request;request.last=1;request.target_mass=2;request.fraction=.4;
 jewel::CollisionFinalizationConfig finish;finish.storage={0,true,false};jewel::ReplayRandom random({.3,.4});
 auto result=jewel::apply_collisions(state,id,tmp,history,ledger,request,{},finish,sampler,medium,random);
 state.validate();check(state.particles().size()==6&&ledger.used==2&&state.colour_max==22&&!result.retry_splitting);
 check(state.at(result.outgoing).decay_time==7&&random.checkpoint()==2);
 auto first=state.at(id).daughters;auto second=state.at(first[1]).daughters;
 check(state.at(first[0]).parent==0&&state.at(first[0]).collision_source==id);
 check(state.at(first[0]).particle.parents==std::vector<jewel::ParticleId>{id});
 check(state.at(first[1]).parent==id&&second[1]==result.outgoing);
 auto saved=state.snapshot();state.at(id).collision_source=result.outgoing;state.at(id).particle.parents={result.outgoing};
 rejects([&]{state.validate();});state.restore(saved);
 state.restore(before);ledger={};jewel::ReplayRandom again({.3,.4});
 auto repeated=jewel::apply_collisions(state,id,tmp,history,ledger,request,{},finish,sampler,medium,again);
 check(repeated.outgoing>result.outgoing);state.validate();
 state.restore(before);jewel::ReplayRandom empty({});
 rejects([&]{jewel::apply_collisions(state,id,id,history,ledger,request,{},finish,sampler,medium,empty);});
 request.last=2;rejects([&]{jewel::apply_collisions(state,id,tmp,history,ledger,request,{},finish,sampler,medium,empty);});
 request.last=1;request.target_mass=2;state.at(id).record_mass=0;state.at(id).particle.momentum={0,0,20,20};
 for(auto& r:history.records)r.invariant=0;
 auto no_kick=jewel::apply_collisions(state,id,tmp,history,ledger,request,{},finish,sampler,medium,empty);
 check(no_kick.retries==2&&state.particles().size()==2&&empty.checkpoint()==0);
 state.validate();
 // Capacity guard occurs before consuming a random number or adding records.
 while(state.particles().size()<22990)state.append(scratch);
 auto capped=jewel::apply_collisions(state,id,tmp,history,ledger,request,{},finish,sampler,medium,empty);
 check(state.discarded&&state.particles().size()==22990&&capped.outgoing==id&&empty.checkpoint()==0);
}
