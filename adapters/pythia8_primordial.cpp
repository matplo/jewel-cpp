// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/pythia8_isr.hpp"
#include <Pythia8/Basics.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <unordered_set>
namespace jewel {
namespace {void require(bool value,const char* message){if(!value)throw std::domain_error(message);}}
Momentum apply_primordial_frame(const PrimordialRecoilFrame& frame,Momentum p){IsrRecoilFrame view;view.matrix=frame.matrix;return apply_isr_frame(view,p);}
SpaceTime apply_primordial_frame_displacement(const PrimordialRecoilFrame& frame,SpaceTime v){IsrRecoilFrame view;view.matrix=frame.matrix;return apply_isr_frame_displacement(view,v);}
PrimordialRecoilFrame reconstruct_primordial_recoil_frame(const HardEventSnapshot& input){
 validate_hard_snapshot(input);require(!input.multiple_parton_interactions,"single-system recoil mapping does not support MPI snapshots");PrimordialRecoilFrame result;
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
std::vector<PrimordialRecoilFrame> reconstruct_subsystem_primordial_frames(const HardEventSnapshot& input){
 validate_hard_snapshot(input);
 std::vector<PrimordialRecoilFrame> frames;
 auto at=[&](ParticleId id)->const GeneratorParticle&{require(id>0&&id<=input.particles.size(),"missing subsystem recoil line");return input.particles[id-1];};
 std::unordered_set<ParticleId> mapped;
 for(const auto& system:input.subsystems){
  PrimordialRecoilFrame frame;std::unordered_set<ParticleId> beams;
  for(std::size_t side=0;side<2;++side){const auto& after=at(system.incoming[side]);
   require(std::abs(after.status)==61&&after.daughters.size()==1&&after.mothers.size()==1,"unsupported subsystem primordial incoming record");
   const auto& before=at(after.daughters[0]);const auto& beam=at(after.mothers[0]);
   require(std::abs(beam.status)==12&&beams.insert(beam.id).second,"rescattered/non-beam subsystem unsupported");
   require(before.pdg==after.pdg&&before.mothers.size()==1&&before.mothers[0]==after.id,"invalid subsystem incoming copy provenance");
   frame.incoming_before[side]=before.id;frame.incoming_after[side]=after.id;
   frame.copies.push_back({before.id,after.id});
  }
  auto vector=[&](ParticleId id){const auto& p=at(id).momentum;return Pythia8::Vec4(p.px,p.py,p.pz,p.energy);};
  auto olda=vector(frame.incoming_before[0]),oldb=vector(frame.incoming_before[1]);
  auto newa=vector(frame.incoming_after[0]),newb=vector(frame.incoming_after[1]);
  require((olda+oldb).m2Calc()>0&&(newa+newb).m2Calc()>0,"non-timelike subsystem primordial system");
  Pythia8::RotBstMatrix to_rest,from_rest;to_rest.toCMframe(olda,oldb);from_rest.fromCMframe(newa,newb);
  auto norm=[](Pythia8::RotBstMatrix& m){double value=0;for(int i=0;i<4;++i){double row=0;for(int j=0;j<4;++j)row+=std::abs(m.value(i,j));value=std::max(value,row);}return value;};
  // The after momenta are already rounded results of the source transform.
  // Reconstructing two highly boosted rest-frame maps amplifies that rounding.
  // Keep this conditioning allowance explicit and separate from the residual.
  double conditioning=norm(to_rest)*norm(from_rest);
  Pythia8::RotBstMatrix matrix;matrix.toCMframe(olda,oldb);matrix.fromCMframe(newa,newb);
  for(int i=0;i<4;++i)for(int j=0;j<4;++j){frame.matrix[4*i+j]=matrix.value(i,j);require(std::isfinite(frame.matrix[4*i+j]),"nonfinite subsystem primordial matrix");}
  for(auto id:system.outgoing){const auto& after=at(id);
   require(std::abs(after.status)==62&&after.mothers.size()==1,"unsupported subsystem outgoing recoil record");
   const auto& before=at(after.mothers[0]);
   require(before.pdg==after.pdg&&before.daughters.size()==1&&before.daughters[0]==after.id,"invalid subsystem outgoing copy provenance");
   frame.copies.push_back({before.id,after.id});
  }
  for(auto pair:frame.copies){require(mapped.insert(pair[1]).second,"duplicate subsystem recoil copy");
   auto predicted=apply_primordial_frame(frame,at(pair[0]).momentum);const auto& actual=at(pair[1]).momentum;
   const auto& original=at(pair[0]).momentum;
   double scale=std::max({1.,std::abs(original.px),std::abs(original.py),std::abs(original.pz),std::abs(original.energy),std::abs(actual.energy)});
   double roundoff=64*std::numeric_limits<double>::epsilon()*conditioning*scale;
   frame.maximum_roundoff_allowance=std::max(frame.maximum_roundoff_allowance,roundoff);
   require(roundoff<=1e-7*scale,"subsystem recoil reconstruction is numerically ill-conditioned");
   for(auto values:{std::array<double,2>{predicted.px,actual.px},{predicted.py,actual.py},{predicted.pz,actual.pz},{predicted.energy,actual.energy}}){
    double error=std::abs(values[0]-values[1]);frame.maximum_copy_error=std::max(frame.maximum_copy_error,error);
    if(error>2e-9+2e-11*std::abs(values[1])+roundoff){std::ostringstream message;
     message<<std::setprecision(17)<<"subsystem primordial frame disagrees with recorded copy: system "<<system.index<<" before "<<pair[0]<<" after "<<pair[1]<<" predicted "<<values[0]<<" actual "<<values[1]<<" error "<<error;
     throw std::domain_error(message.str());}
   }
  }
  frames.push_back(std::move(frame));
 }
 for(const auto& node:input.particles)if(std::abs(node.status)==61||std::abs(node.status)==62)
  require(mapped.count(node.id)==1,"unmapped primordial copy");
 return frames;
}

}
