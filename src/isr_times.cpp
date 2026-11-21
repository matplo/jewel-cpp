// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/isr_times.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
void require(bool value,const char* message){if(!value)throw std::domain_error(message);}
void validate(const IsrKinematics& k,bool timed){
 for(double x:{k.momentum.px,k.momentum.py,k.momentum.pz,k.momentum.energy,k.record_mass,k.nominal_mass})
  require(std::isfinite(x),"nonfinite ISR kinematics");
 require(k.nominal_mass>=0,"negative ISR nominal mass");
 if(timed)require(k.momentum.energy!=0&&k.record_mass!=0,"undefined ISR formation rate");
}
double interval(const IsrKinematics& k,double factor,Random& random){
 // Stock default-REAL 0.2 is promoted into the double-precision expression.
 double rate=1./(factor*k.momentum.energy*double(.2f)/(k.record_mass*k.record_mass));
 require(std::isfinite(rate)&&rate!=0,"invalid ISR formation rate");
 double u=random.uniform();require(std::isfinite(u)&&u>0&&u<1,"invalid ISR random draw");
 return std::log(1.-u)/rate;
}
SpaceTime position(SpaceTime base,const IsrKinematics& k,double t,double delta){
 const auto& p=k.momentum;
 double speed_denominator=std::max(std::sqrt(p.px*p.px+p.py*p.py+p.pz*p.pz),p.energy);
 require(std::isfinite(speed_denominator)&&speed_denominator>0,"invalid ISR velocity");
 return {base.x+delta*p.px/speed_denominator,base.y+delta*p.py/speed_denominator,
         base.z+delta*p.pz/speed_denominator,t};
}
void finite(const IsrTimedLine& l){for(double x:{l.production.x,l.production.y,l.production.z,l.production.t,l.end_time})require(std::isfinite(x),"nonfinite ISR time output");}
}
IsrTimeResult assign_isr_times(const IsrKinematics& hard,const std::vector<IsrTimeStep>& steps,
 const IsrTimeConfig& config,Random& random){
 for(double x:{config.x,config.y,config.formation_factor,config.lifetime})require(std::isfinite(x),"nonfinite ISR configuration");
 require(config.formation_factor>0,"invalid ISR formation factor");validate(hard,true);
 for(std::size_t i=0;i<steps.size();++i){const auto& s=steps[i];
  require(s.beam_side>=-1&&s.beam_side<=1,"invalid ISR beam side");
  require((s.beam_side!=0)==(i+1==steps.size()),"ISR ancestry must terminate at one beam");
  validate(s.incoming,s.beam_side==0);validate(s.emitted,s.emitted.record_mass!=s.emitted.nominal_mass);
 }
 IsrTimeResult result;result.hard.end_time=config.scatter?0.:-1.e6;
 double time=result.hard.end_time+interval(hard,config.formation_factor,random);
 result.hard.production=position({config.x,config.y,0,0},hard,time,time);finite(result.hard);
 auto previous=result.hard;
 for(const auto& s:steps){IsrTimedLine incoming,emitted;incoming.end_time=previous.production.t;
  if(s.beam_side)incoming.production={1.e6,1.e6,s.beam_side*1.e6,-1.e6};
  else {time=incoming.end_time+interval(s.incoming,config.formation_factor,random);
   incoming.production=position(previous.production,s.incoming,time,time-incoming.end_time);}
  emitted.production=previous.production;
  emitted.end_time=s.emitted.record_mass==s.emitted.nominal_mass?config.lifetime:
   previous.production.t-interval(s.emitted,config.formation_factor,random);
  finite(incoming);finite(emitted);result.incoming.push_back(incoming);result.emitted.push_back(emitted);previous=incoming;
 }
 return result;
}
}
