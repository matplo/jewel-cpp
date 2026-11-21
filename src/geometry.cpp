// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/geometry.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace jewel {
PathLengths straight_path_lengths(double x0,double y0,double b,double radius,
 double lifetime,Momentum p,double step,const Medium& medium){
 PathLengths result;
 for(double v:{x0,y0,b,radius,lifetime,step})if(!std::isfinite(v))throw std::domain_error("nonfinite geometry input");
 if(step<=0)throw std::domain_error("geometry step must be positive");
 if(radius<=0)return result; // Custom vacuum convention: no medium geometry.
 for(double v:{p.px,p.py,p.pz,p.energy})if(!std::isfinite(v))throw std::domain_error("nonfinite shower-start momentum");
 if(p.energy<=0)throw std::domain_error("nonpositive shower-start energy");
 double vx=p.px/p.energy,vy=p.py/p.energy,vz=p.pz/p.energy;
 double speed=std::sqrt(vx*vx+vy*vy+vz*vz),vt=std::sqrt(vx*vx+vy*vy);
 if(speed>1+1e-8)throw std::domain_error("spacelike shower-start momentum");
 if(speed==0)return result;
 double end=std::max(0.,lifetime);
 if(vt>0){
  double nx=vx/vt,ny=vy/vt;result.geometric_transverse=std::numeric_limits<double>::max();
  for(int j=1;j<=2;++j){
   double x=x0+(2*j-3)*b/2.,dot=x*nx+y0*ny;
   double disc=dot*dot+radius*radius-x*x-y0*y0;
   if(x*x+y0*y0>radius*radius*(1+1e-10))throw std::domain_error("production point outside NTHICK support");
   double distance=std::max(0.,-dot+std::sqrt(std::max(0.,disc)));
   result.geometric_transverse=std::min(result.geometric_transverse,distance);
  }
  end=std::min(end,result.geometric_transverse/vt);
 }
 if(end<=0)return result;
 double ratio=end/step;
 if(!std::isfinite(ratio)||ratio>10000000)throw std::domain_error("geometry step exceeds interval limit");
 int n=std::max(1,static_cast<int>(std::ceil(ratio)));double dt=end/n;
 for(int i=1;i<=n;++i){
  double t=(i-.5)*dt,temp=medium.sample({x0+vx*t,y0+vy*t,vz*t,t}).temperature;
  if(!std::isfinite(temp))throw std::domain_error("medium returned nonfinite temperature");
  if(temp>0){result.hot+=speed*dt;result.hot_transverse+=vt*dt;
   result.temperature_cubed_integral+=temp*temp*temp*speed*dt;}
 }
 return result;
}
}
