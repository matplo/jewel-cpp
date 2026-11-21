// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/coherence.hpp"
#include <iostream>
#include <iomanip>
#include <string>
int main() {
 std::cout<<std::scientific<<std::setprecision(17);std::string op;
 while(std::cin>>op) {
  jewel::Momentum p;std::cin>>p.px>>p.py>>p.pz>>p.energy;
  if(op=="GATE") {
   jewel::TransferRecord q;double f,a,t,s;
   std::cin>>q.invariant>>q.momentum.px>>q.momentum.py>>q.momentum.pz>>f>>a>>t>>s;
   if(!std::cin)return 2;
   try {auto r=jewel::coherent_resolution(p,q,f,a,t,s);std::cout<<0<<' '<<r.transverse_transfer<<' '<<r.separation<<' '<<r.threshold<<' '<<int(r.resolved)<<'\n';}
   catch(const std::domain_error&){std::cout<<1<<'\n';}
  } else if(op=="SPLIT") {
   jewel::SpaceTime v;double m,ma,mb,z,phi;std::cin>>m>>v.x>>v.y>>v.z>>v.t>>ma>>mb>>z>>phi;
   if(!std::cin)return 2;
   try {auto r=jewel::reconstruct_coherent(p,v,m,ma,mb,z,phi);
    std::cout<<0<<' '<<int(r.possible)<<' '<<r.longitudinal<<' '<<r.transverse_squared;
    for(auto q:{r.parent,r.first,r.second})std::cout<<' '<<q.px<<' '<<q.py<<' '<<q.pz<<' '<<q.energy;
    std::cout<<' '<<r.generator_vertex.x<<' '<<r.generator_vertex.y<<' '<<r.generator_vertex.z<<' '<<r.generator_vertex.t<<'\n';
   }catch(const std::domain_error&){std::cout<<1<<'\n';}
  } else return 3;
 }
}
