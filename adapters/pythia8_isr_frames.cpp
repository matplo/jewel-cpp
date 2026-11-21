// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_isr.hpp"
#include <Pythia8/Basics.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
void require(bool b,const char* message){if(!b)throw std::domain_error(message);}
std::array<double,4> apply(const IsrRecoilFrame& frame,std::array<double,4> input){
 std::array<double,4> out{};
 for(int i=0;i<4;++i)for(int j=0;j<4;++j){require(std::isfinite(frame.matrix[4*i+j])&&std::isfinite(input[j]),"nonfinite ISR frame input");out[i]+=frame.matrix[4*i+j]*input[j];}
 for(double value:out)require(std::isfinite(value),"nonfinite ISR frame output");return out;
}
bool collinear(const GeneratorParticle& p){return p.momentum.energy>0&&p.mass==0&&std::hypot(p.momentum.px,p.momentum.py)<=1e-10*p.momentum.energy&&std::abs(std::abs(p.momentum.pz)-p.momentum.energy)<=1e-10*p.momentum.energy;}
}
Momentum apply_isr_frame(const IsrRecoilFrame& frame,Momentum p){auto q=apply(frame,{p.energy,p.px,p.py,p.pz});return {q[1],q[2],q[3],q[0]};}
SpaceTime apply_isr_frame_displacement(const IsrRecoilFrame& frame,SpaceTime v){auto q=apply(frame,{v.t,v.x,v.y,v.z});return {q[1],q[2],q[3],q[0]};}
std::vector<IsrRecoilFrame> reconstruct_isr_recoil_frames(const HardEventSnapshot& input){
 auto chains=map_pythia_isr_ancestry(input);std::vector<IsrRecoilFrame> frames;
 auto at=[&](ParticleId id)->const GeneratorParticle&{require(id>0&&id<=input.particles.size(),"missing ISR recoil line");return input.particles[id-1];};
 const double pi=std::acos(-1.);
 for(int side=0;side<2;++side)for(const auto& branch:chains[side].branches){
  const auto& mother=at(branch.mother);const auto& daughter=at(branch.daughter);const auto& sister=at(branch.emitted);
  // SimpleSpaceShower appends the incoming radiator/recoiler pair consecutively.
  std::vector<ParticleId> candidates;
  for(auto id:{mother.id-1,mother.id+1})if(id>0&&id<=input.particles.size()&&std::abs(at(id).status)==42)candidates.push_back(id);
  require(candidates.size()==1,"ambiguous or unsupported ISR recoil pair");const auto& recoiler=at(candidates[0]);
  require(recoiler.daughters.size()==1,"invalid ISR recoiler daughter");const auto& old=at(recoiler.daughters[0]);
  require(std::find(chains[1-side].backward_lines.begin(),chains[1-side].backward_lines.end(),recoiler.id)!=chains[1-side].backward_lines.end(),"ISR recoiler is not on opposite beam chain");
  require(collinear(mother)&&collinear(daughter)&&collinear(recoiler)&&collinear(old),"ISR frame requires massless collinear II incoming records");
  double beam_sign=side==0?1.:-1.;require(mother.momentum.pz*beam_sign>0&&daughter.momentum.pz*beam_sign>0&&recoiler.momentum.pz*beam_sign<0&&old.momentum.pz*beam_sign<0,"ISR frame beam directions disagree");
  double ea=side==0?daughter.momentum.energy:old.momentum.energy,eb=side==0?old.momentum.energy:daughter.momentum.energy;
  double ena=side==0?mother.momentum.energy:recoiler.momentum.energy,enb=side==0?recoiler.momentum.energy:mother.momentum.energy;
  double m2=4*ea*eb,m=std::sqrt(m2),q2=branch.virtuality_squared,z=daughter.momentum.energy/mother.momentum.energy;
  require(z>0&&z<1,"invalid reconstructed ISR momentum fraction");double ms2=sister.mass*sister.mass;
  double pt2=q2-z*(m2+q2)*(q2+ms2)/m2;require(pt2>0&&std::isfinite(pt2),"invalid reconstructed ISR transverse scale");
  double sign=side==0?1.:-1.,er=.5*(m2+q2)/m,pzr=-sign*er;
  double pt=std::sqrt(pt2)*m2/(z*(m2+q2)),pzm=sign*.5*m*((m2-q2)/(z*(m2+q2))+(q2+ms2)/m2);
  Pythia8::Vec4 pm(pt,0,pzm,std::sqrt(pt*pt+pzm*pzm)),pr(0,0,pzr,er),ps(pt,0,pzm+pzr,std::sqrt(pt*pt+(pzm+pzr)*(pzm+pzr)+ms2));
  auto total=pm+pr;
  auto from_rest=[&](double phi){Pythia8::RotBstMatrix matrix;matrix.bst(-total.px()/total.e(),0,-total.pz()/total.e());
   auto transformed=pm;transformed.rotbst(matrix);double theta=transformed.theta();if(transformed.px()<0)theta=-theta;if(side==1)theta+=pi;
   matrix.rot(-theta,phi);matrix.bst(0,0,(ena-enb)/(ena+enb));return matrix;};
  auto base=ps;base.rotbst(from_rest(0));
  double phi=std::atan2(sister.momentum.py,sister.momentum.px)-base.phi();
  Pythia8::RotBstMatrix matrix;matrix.bst(0,0,(eb-ea)/(ea+eb));matrix.rot(0,-phi);matrix.rotbst(from_rest(phi));
  IsrRecoilFrame frame;frame.emitted=sister.id;frame.mother=mother.id;frame.recoiler=recoiler.id;frame.old_recoiler=old.id;
  for(int i=0;i<4;++i)for(int j=0;j<4;++j){frame.matrix[4*i+j]=matrix.value(i,j);require(std::isfinite(frame.matrix[4*i+j]),"nonfinite reconstructed ISR matrix");}
  for(auto id=std::max(mother.id,recoiler.id)+1;id<sister.id;++id){const auto& after=at(id);
   require(std::abs(after.status)==44&&after.mothers.size()==1,"unsupported ISR final-state recoil record");const auto& before=at(after.mothers[0]);
   require(before.id<std::min(mother.id,recoiler.id)&&before.pdg==after.pdg,"invalid ISR recoil-copy provenance");
   auto predicted=apply_isr_frame(frame,before.momentum);const auto& actual=after.momentum;
   for(auto pair:{std::array<double,2>{predicted.px,actual.px},{predicted.py,actual.py},{predicted.pz,actual.pz},{predicted.energy,actual.energy}}){
    double error=std::abs(pair[0]-pair[1]);frame.maximum_copy_error=std::max(frame.maximum_copy_error,error);
    require(error<=2e-9+2e-11*std::abs(pair[1]),"reconstructed ISR frame disagrees with recorded recoil copy");}
   frame.copies.push_back({before.id,after.id});
  }
  require(frame.copies.size()>=2,"missing ISR final-state recoil copies");frames.push_back(frame);
 }
 std::sort(frames.begin(),frames.end(),[](const auto& a,const auto& b){return a.emitted<b.emitted;});return frames;
}
}
