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
 // A coherent projectile has two incoming ancestors; outgoing collision records
 // must descend only from that projectile, never inherit its second input.
 {
  jewel::ShowerState coherent;coherent.colour_max=20;
  auto scratch_id=coherent.append(scratch);auto root_id=coherent.append(leg);
  auto sibling=leg;sibling.parent=root_id;
  auto a=coherent.append(sibling),b=coherent.append(sibling);coherent.connect(root_id,a,b);
  auto merged_leg=leg;merged_leg.parent=a;merged_leg.coherent_partner=b;
  auto merged_id=coherent.append(merged_leg);coherent.connect_coherent(a,b,merged_id);
  jewel::TransferHistory kicks=history;for(auto& r:kicks.records)r.invariant=-.2;
  jewel::RecoilStorage storage;jewel::ReplayRandom tape({.3,.4});
  auto coherent_request=request;coherent_request.coherent=true;coherent_request.time=4;
  auto answer=jewel::apply_collisions(coherent,merged_id,scratch_id,kicks,storage,coherent_request,{},finish,sampler,medium,tape);
  coherent.validate();check(answer.outgoing!=merged_id&&coherent.at(answer.outgoing).decay_time==4);
  for(const auto& node:coherent.particles())if(node.particle.id>merged_id)check(!node.coherent_partner&&node.particle.parents.size()==1);
  check(coherent.at(merged_id).coherent_partner==b);
 }
 // A failure after consuming the first collision must not publish partial
 // event or recoil mutations. The random stream deliberately is not rewound.
 {
  jewel::ShowerState isolated;isolated.colour_max=20;
  auto scratch_id=isolated.append(scratch),leg_id=isolated.append(leg);
  jewel::TransferHistory kicks=history;for(auto& r:kicks.records)r.invariant=-.2;
  jewel::RecoilStorage storage;storage.capacity=3;
  jewel::ReplayRandom exhausted({.3});
  rejects([&]{jewel::apply_collisions(isolated,leg_id,scratch_id,kicks,storage,request,{},finish,sampler,medium,exhausted);});
  isolated.validate();check(isolated.particles().size()==2&&isolated.colour_max==20);
  check(isolated.at(leg_id).daughters==std::array<jewel::ParticleId,2>{0,0});
  check(storage.used==0&&storage.records.empty()&&exhausted.checkpoint()==1);
 }
 // Capacity guard occurs before consuming a random number or adding records.
 while(state.particles().size()<22990)state.append(scratch);
 auto capped=jewel::apply_collisions(state,id,tmp,history,ledger,request,{},finish,sampler,medium,empty);
 check(state.discarded&&state.particles().size()==22990&&capped.outgoing==id&&empty.checkpoint()==0);
}
