// SPDX-License-Identifier: GPL-2.0-or-later
// Standard Lorentz kinematics preserving JEWEL's PYANGL/PYROBO/PYP conventions.
#include "jewel/kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {
constexpr double pi=3.141592653589793;
void finite(double x){if(!std::isfinite(x))throw std::domain_error("nonfinite kinematics input");}
void validate(Momentum p){for(double x:{p.px,p.py,p.pz,p.energy})finite(x);}
double angle(double x,double y) {
 double r=std::sqrt(x*x+y*y);if(r<1e-20)return 0;
 if(std::abs(x)/r<.8)return std::copysign(std::acos(x/r),y);
 double value=std::asin(y/r);
 if(x<0&&value>=0)value=pi-value;else if(x<0)value=-pi-value;
 return value;
}
}
double momentum_magnitude(Momentum p){validate(p);return std::sqrt(p.px*p.px+p.py*p.py+p.pz*p.pz);}
double polar_angle(Momentum p){validate(p);return angle(p.pz,std::sqrt(p.px*p.px+p.py*p.py));}
double azimuth(Momentum p){validate(p);return angle(p.px,p.py);}
double rapidity(Momentum p,double mass){validate(p);finite(mass);double pr=std::max(1e-20,mass*mass+p.px*p.px+p.py*p.py);return std::copysign(std::log(std::min((std::sqrt(pr+p.pz*p.pz)+std::abs(p.pz))/std::sqrt(pr),1e20)),p.pz);}
double pseudorapidity(Momentum p){return rapidity(p,0);}
bool rotate_boost(Momentum& p,SpaceTime* position,double theta,double phi,std::array<double,3> beta) {
 validate(p);finite(theta);finite(phi);for(double b:beta)finite(b);
 if(position)for(double x:{position->x,position->y,position->z,position->t})finite(x);
 std::array<double,4> momentum{p.px,p.py,p.pz,p.energy},v{};
 if(position)v={position->x,position->y,position->z,position->t};
 if(theta*theta+phi*phi>1e-20) {
  double ct=std::cos(theta),st=std::sin(theta),cp=std::cos(phi),sp=std::sin(phi);
  double rotation[3][3]={{ct*cp,-sp,st*cp},{ct*sp,cp,st*sp},{-st,0,ct}};
  auto original=momentum,vertex=v;
  for(int i=0;i<3;++i){momentum[i]=rotation[i][0]*original[0]+rotation[i][1]*original[1]+rotation[i][2]*original[2];v[i]=rotation[i][0]*vertex[0]+rotation[i][1]*vertex[1]+rotation[i][2]*vertex[2];}
 }
 double square=beta[0]*beta[0]+beta[1]*beta[1]+beta[2]*beta[2];bool rescaled=false;
 if(square>1e-20) {
  double b=std::sqrt(square),limit=1-1e-12;
  if(b>limit){for(double& component:beta)component=component*(limit/b);b=limit;rescaled=true;}
  // Stock compiler folds the rescaled gamma from rounded EPS1**2; keep that
  // rounding separate from the runtime fused 1-b*b expression.
  constexpr double limit_squared=(1-1e-12)*(1-1e-12);
  double gamma=rescaled?1/std::sqrt(1-limit_squared):1/std::sqrt(1-b*b);
  auto boost=[&](std::array<double,4> vector){
   double dot=beta[0]*vector[0]+beta[1]*vector[1]+beta[2]*vector[2];
   double factor=gamma*(gamma*dot/(1+gamma)+vector[3]);
   return std::array<double,4>{vector[0]+factor*beta[0],vector[1]+factor*beta[1],vector[2]+factor*beta[2],gamma*(vector[3]+dot)};
  };
  momentum=boost(momentum);v=boost(v);
 }
 p={momentum[0],momentum[1],momentum[2],momentum[3]};if(position)*position={v[0],v[1],v[2],v[3]};return rescaled;
}
SpaceTime position_at(Momentum p,SpaceTime v,double t) {
 validate(p);for(double x:{v.x,v.y,v.z,v.t,t})finite(x);
 double denominator=std::max(momentum_magnitude(p),p.energy);
 if(denominator<=0)throw std::domain_error("undefined zero-momentum propagation");
 return {v.x+(t-v.t)*p.px/denominator,v.y+(t-v.t)*p.py/denominator,v.z+(t-v.t)*p.pz/denominator,t};
}
}
