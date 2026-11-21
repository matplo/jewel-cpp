// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_isr.hpp"
#include <Pythia8/Basics.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>
namespace jewel {
namespace {void require(bool value,const char* message){if(!value)throw std::domain_error(message);}}
Momentum apply_primordial_frame(const PrimordialRecoilFrame& frame,Momentum p){IsrRecoilFrame view;view.matrix=frame.matrix;return apply_isr_frame(view,p);}
SpaceTime apply_primordial_frame_displacement(const PrimordialRecoilFrame& frame,SpaceTime v){IsrRecoilFrame view;view.matrix=frame.matrix;return apply_isr_frame_displacement(view,v);}
PrimordialRecoilFrame reconstruct_primordial_recoil_frame(const HardEventSnapshot& input){
 validate_hard_snapshot(input);PrimordialRecoilFrame result;
 auto at=[&](ParticleId id)->const GeneratorParticle&{require(id>0&&id<=input.particles.size(),"missing primordial recoil line");return input.particles[id-1];};
 std::unordered_set<ParticleId> visited,beams;
 for(std::size_t side=0;side<2;++side){auto id=input.hard_incoming[side];
  for(;;){const auto& line=at(id);require(visited.insert(id).second,"cyclic/shared primordial incoming ancestry");
   require(line.mothers.size()==1,"primordial incoming line requires one mother");auto parent=line.mothers.front();const auto& mother=at(parent);
   require(std::find(mother.daughters.begin(),mother.daughters.end(),id)!=mother.daughters.end(),"nonreciprocal primordial ancestry");
   if(std::abs(mother.status)==61){
    require(mother.daughters.size()==1&&mother.pdg==line.pdg&&mother.mothers.size()==1,"invalid primordial incoming copy");
    const auto& beam=at(mother.mothers[0]);require(std::abs(beam.status)==12&&beams.insert(beam.id).second,"invalid primordial beam association");
    require(std::find(beam.daughters.begin(),beam.daughters.end(),mother.id)!=beam.daughters.end(),"nonreciprocal primordial beam association");
    result.incoming_before[side]=id;result.incoming_after[side]=parent;break;
   }
   require(std::abs(mother.status)==41||std::abs(mother.status)==42,"unsupported primordial incoming history");id=parent;
  }
 }
 auto vector=[&](ParticleId id){const auto& p=at(id).momentum;return Pythia8::Vec4(p.px,p.py,p.pz,p.energy);};
 auto olda=vector(result.incoming_before[0]),oldb=vector(result.incoming_before[1]);
 auto newa=vector(result.incoming_after[0]),newb=vector(result.incoming_after[1]);
 require((olda+oldb).m2Calc()>0&&(newa+newb).m2Calc()>0,"non-timelike primordial system");
 Pythia8::RotBstMatrix matrix;matrix.toCMframe(olda,oldb);matrix.fromCMframe(newa,newb);
 for(int i=0;i<4;++i)for(int j=0;j<4;++j){result.matrix[4*i+j]=matrix.value(i,j);require(std::isfinite(result.matrix[4*i+j]),"nonfinite primordial matrix");}
 unsigned incoming=0;
 for(const auto& after:input.particles){int status=std::abs(after.status);if(status!=61&&status!=62)continue;
  ParticleId before=0;
  if(status==61){++incoming;
   auto found=std::find(result.incoming_after.begin(),result.incoming_after.end(),after.id);
   require(found!=result.incoming_after.end(),"multiple primordial systems unsupported");before=result.incoming_before[found-result.incoming_after.begin()];
  }else{require(after.mothers.size()==1,"invalid primordial outgoing copy");before=after.mothers[0];
   const auto& original=at(before);require(original.pdg==after.pdg&&std::find(original.daughters.begin(),original.daughters.end(),after.id)!=original.daughters.end(),"invalid primordial outgoing provenance");}
  auto predicted=apply_primordial_frame(result,at(before).momentum);const auto& actual=after.momentum;
  for(auto pair:{std::array<double,2>{predicted.px,actual.px},{predicted.py,actual.py},{predicted.pz,actual.pz},{predicted.energy,actual.energy}}){
   double error=std::abs(pair[0]-pair[1]);result.maximum_copy_error=std::max(result.maximum_copy_error,error);
   require(error<=2e-9+2e-11*std::abs(pair[1]),"primordial frame disagrees with recorded copy");}
  result.copies.push_back({before,after.id});
 }
 require(incoming==2&&result.copies.size()>=4,"incomplete primordial copy record");return result;
}
}
