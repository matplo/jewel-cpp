// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/coherence.hpp"
#include <iostream>
#include <iomanip>
int main(){
 try{
 std::cout<<std::scientific<<std::setprecision(17);
 int selected,enabled;double start,duration,angle;
 while(std::cin>>selected>>enabled>>start>>duration>>angle){
  jewel::ShowerState state;std::array<std::array<jewel::ParticleId,2>,4>daughters;
  for(int i=0;i<4;++i){
   jewel::ShowerParticle s;auto& p=s.particle;int pair;
   std::cin>>p.status>>p.pdg>>s.parent>>daughters[i][0]>>daughters[i][1]
    >>p.momentum.px>>p.momentum.py>>p.momentum.pz>>p.momentum.energy>>s.record_mass
    >>s.generator_vertex.x>>s.generator_vertex.y>>s.generator_vertex.z>>s.generator_vertex.t
    >>p.production.x>>p.production.y>>p.production.z>>p.production.t>>s.decay_time
    >>s.fraction>>s.angle>>pair>>p.colour>>p.anticolour;
   if(!std::cin)return 2;s.quark_pair=pair;state.colour_max=std::max(state.colour_max,std::max(p.colour,p.anticolour));state.append(s);
  }
  for(int i=0;i<4;++i)state.at(i+1).daughters=daughters[i];
  state.validate();auto r=jewel::begin_coherent(state,selected,enabled,start,duration,angle);state.validate();
  std::cout<<int(r.active)<<' '<<r.duration;
  if(r.active)std::cout<<' '<<r.remaining<<' '<<r.fraction<<' '<<r.azimuth<<' '<<int(r.quark)<<' '<<int(r.quark_pair)<<' '<<r.colour_direction;
  std::cout<<' '<<state.particles().size()<<'\n';
  for(const auto& s:state.particles()){
   const auto& p=s.particle;
   std::cout<<p.status<<' '<<p.pdg<<' '<<s.parent<<' '<<s.daughters[0]<<' '<<s.daughters[1]
    <<' '<<p.momentum.px<<' '<<p.momentum.py<<' '<<p.momentum.pz<<' '<<p.momentum.energy<<' '<<s.record_mass
    <<' '<<s.generator_vertex.x<<' '<<s.generator_vertex.y<<' '<<s.generator_vertex.z<<' '<<s.generator_vertex.t
    <<' '<<p.production.x<<' '<<p.production.y<<' '<<p.production.z<<' '<<p.production.t<<' '<<s.decay_time
    <<' '<<s.fraction<<' '<<s.angle<<' '<<int(s.quark_pair)<<' '<<p.colour<<' '<<p.anticolour<<'\n';
  }
 }
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 3;}
}
