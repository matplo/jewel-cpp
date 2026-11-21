// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/isr_times.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("ISR timing contract failed");}
int main(){
 jewel::IsrKinematics hard{{1,2,12,10},-2,0},incoming{{1,2,15,12},-3,0},emitted{{1,0,0,1},0,0};
 jewel::IsrTimeConfig config;config.x=1.2;config.y=-.7;config.lifetime=20;
 for(bool scatter:{false,true})for(int beam:{-1,1}){
  config.scatter=scatter;jewel::ReplayRandom random({.5,.25});
  auto result=jewel::assign_isr_times(hard,{{incoming,emitted,beam}},config,random);
  check(random.checkpoint()==1&&result.incoming.size()==1&&result.emitted.size()==1);
  check(result.hard.end_time==(scatter?0.:-1.e6));
  check(result.hard.production.t<result.hard.end_time);
  check(result.incoming[0].production.x==1.e6&&result.incoming[0].production.y==1.e6);
  check(result.incoming[0].production.z==beam*1.e6&&result.incoming[0].production.t==-1.e6);
  check(result.incoming[0].end_time==result.hard.production.t);
  check(result.emitted[0].production.t==result.hard.production.t&&result.emitted[0].end_time==20);
 }
 auto virtual_emitted=emitted;virtual_emitted.record_mass=.5;
 jewel::ReplayRandom random({.5,.25,.4,.75});
 auto result=jewel::assign_isr_times(hard,{{incoming,virtual_emitted,0},{incoming,emitted,1}},config,random);
 check(random.checkpoint()==3);
 check(result.incoming[0].production.t<result.hard.production.t);
 check(result.emitted[0].end_time>result.emitted[0].production.t);
 check(result.emitted[1].production.t==result.incoming[0].production.t);
 for(int bad=0;bad<5;++bad){
  auto h=hard;auto c=config;std::vector<jewel::IsrTimeStep> steps{{incoming,emitted,1}};
  if(bad==0)h.record_mass=0;
  if(bad==1)c.formation_factor=0;
  if(bad==2)steps[0].beam_side=0;
  if(bad==3)steps[0].beam_side=2;
  if(bad==4)steps[0].emitted.momentum.energy=std::numeric_limits<double>::quiet_NaN();
  jewel::ReplayRandom stream({.5});bool rejected=false;
  try{jewel::assign_isr_times(h,steps,c,stream);}catch(const std::domain_error&){rejected=true;}
  check(rejected&&stream.checkpoint()==0);
 }
}
