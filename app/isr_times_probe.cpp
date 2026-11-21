// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/isr_times.hpp"
#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main(){try{
 int n,enabled,ntape;jewel::IsrTimeConfig c;std::cout<<std::setprecision(17);
 while(std::cin>>n>>c.formation_factor>>c.x>>c.y>>c.lifetime>>enabled>>ntape){
  if(n<106||n>23000||ntape<1||ntape>10000)throw std::domain_error("invalid probe dimensions");
  c.scatter=enabled;std::vector<int> mother(n+1);std::vector<jewel::IsrKinematics> k(n+1);std::vector<jewel::IsrTimedLine> output(n+1);
  for(int i=105;i<=n;++i){auto& a=k[i];std::cin>>mother[i]>>a.momentum.px>>a.momentum.py>>a.momentum.pz>>a.momentum.energy>>a.record_mass>>a.nominal_mass;}
  std::vector<double> tape(ntape);for(auto& x:tape)std::cin>>x;if(!std::cin)throw std::domain_error("invalid probe input");jewel::ReplayRandom random(tape);
  for(int hard:{105,106}){
   std::vector<jewel::IsrTimeStep> steps;std::vector<std::array<int,2>> indices;int previous=hard;
   while(mother.at(previous)>4){int incoming=mother.at(previous),emitted=0;
    if(incoming>n||steps.size()>static_cast<unsigned>(n))throw std::domain_error("invalid probe ancestry");
    for(int j=105;j<=n;++j)if(mother[j]==incoming)emitted=j;
    if(!emitted||emitted==previous)throw std::domain_error("missing emitted sibling");
    int side=mother[incoming]==3?1:mother[incoming]==4?-1:0;
    steps.push_back({k[incoming],k[emitted],side});indices.push_back({incoming,emitted});previous=incoming;
   }
   auto times=jewel::assign_isr_times(k[hard],steps,c,random);output[hard]=times.hard;
   for(std::size_t i=0;i<indices.size();++i){output[indices[i][0]]=times.incoming[i];output[indices[i][1]]=times.emitted[i];}
  }
  std::cout<<random.checkpoint()<<'\n';for(int i=105;i<=n;++i){const auto& l=output[i];std::cout<<l.production.x<<' '<<l.production.y<<' '<<l.production.z<<' '<<l.production.t<<' '<<l.end_time<<'\n';}
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
