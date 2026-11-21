// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/kinematics.hpp"
#include "jewel/hard_initialization.hpp"
#include <iostream>
#include <iomanip>
#include <string>
int main(){
 try{
 std::cout<<std::scientific<<std::setprecision(17);std::string op;
 while(std::cin>>op){
  if(op=="PREPARE"){
   std::size_t count,first,second;double limit;
   if(!(std::cin>>count>>first>>second>>limit)||count<2||count>23000||first==0||second==0)throw std::runtime_error("invalid preparation dimensions");
   std::vector<jewel::ShowerParticle> nodes(count);std::vector<double> nominal(count);
   for(std::size_t i=0;i<count;++i){auto& node=nodes[i];auto& m=node.particle.momentum;auto& v=node.generator_vertex;
    if(!(std::cin>>node.particle.status>>node.particle.pdg>>m.px>>m.py>>m.pz>>m.energy>>node.record_mass>>v.x>>v.y>>v.z>>v.t>>nominal[i]))throw std::runtime_error("bad preparation input");}
   jewel::prepare_surviving_partons(nodes,first-1,second-1,nominal,limit);
   for(const auto& node:nodes){const auto& m=node.particle.momentum;const auto& v=node.generator_vertex;
    std::cout<<node.particle.status<<' '<<m.px<<' '<<m.py<<' '<<m.pz<<' '<<m.energy<<' '<<node.record_mass<<' '<<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t<<'\n';}
   continue;
  }
  if(op=="MASSLESS"){
   std::array<jewel::ShowerParticle,3> nodes;
   for(auto& node:nodes){auto& m=node.particle.momentum;auto& v=node.generator_vertex;node.particle.status=1;
    if(!(std::cin>>m.px>>m.py>>m.pz>>m.energy>>node.record_mass>>v.x>>v.y>>v.z>>v.t))throw std::runtime_error("bad massless input");}
   jewel::make_parton_massless(nodes[2],nodes[0],nodes[1]);
   for(const auto& node:nodes){const auto& m=node.particle.momentum;const auto& v=node.generator_vertex;
    std::cout<<m.px<<' '<<m.py<<' '<<m.pz<<' '<<m.energy<<' '<<node.record_mass<<' '<<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t<<'\n';}
   continue;
  }
  jewel::Momentum p;jewel::SpaceTime v;
  if(!(std::cin>>p.px>>p.py>>p.pz>>p.energy))throw std::runtime_error("bad momentum");
  if(op=="OBSERVABLES"){
   double mass;if(!(std::cin>>mass))throw std::runtime_error("bad mass");
   std::cout<<jewel::momentum_magnitude(p)<<' '<<jewel::polar_angle(p)<<' '<<jewel::azimuth(p)<<' '<<jewel::rapidity(p,mass)<<' '<<jewel::pseudorapidity(p)<<'\n';
  }else{
   if(!(std::cin>>v.x>>v.y>>v.z>>v.t))throw std::runtime_error("bad coordinates");
   if(op=="TRANSFORM"){
    double theta,phi;std::array<double,3> beta;
    if(!(std::cin>>theta>>phi>>beta[0]>>beta[1]>>beta[2]))throw std::runtime_error("bad transform");
    bool rescaled=jewel::rotate_boost(p,&v,theta,phi,beta);
    std::cout<<p.px<<' '<<p.py<<' '<<p.pz<<' '<<p.energy<<' '<<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.t<<' '<<int(rescaled)<<'\n';
   }else if(op=="POSITION"){
    double time;if(!(std::cin>>time))throw std::runtime_error("bad time");auto point=jewel::position_at(p,v,time);
    std::cout<<point.x<<' '<<point.y<<' '<<point.z<<'\n';
   }else throw std::runtime_error("unknown query");
  }
 }
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
