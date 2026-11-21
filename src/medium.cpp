// SPDX-License-Identifier: GPL-2.0-or-later
// Translated from JEWEL 2.6.0: Korinna C. Zapp; medium contributions by Jochen Klein.
// Positive Ei algorithm: S. Zhang and J. M. Jing (1996), see NOTICE.
#include "jewel/medium.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace jewel {
namespace {
constexpr double pi=3.141592653589793;
void check(bool ok,const char* why) { if(!ok) throw std::domain_error(why); }
double rk4(const std::function<double(double)>& f,double x,double y,double dydx,double h) {
 double k1=h*dydx, k2=h*f(x+h/2), k4=h*f(x+h);
 return y+k1/6+2*k2/3+k4/6;
}
double positive_ei(double x) {
 double ei=1,r=1;
 if(x<=40) {
  for(int k=1;k<=100;++k) { r=r*k*x/((k+1.)*(k+1.)); ei+=r; if(std::abs(r/ei)<=1e-15) break; }
  return 0.5772156649015328+std::log(x)+x*ei;
 }
 for(int k=1;k<=20;++k) { r=r*k/x; ei+=r; }
 return std::exp(x)/x*ei;
}
void position_check(SpaceTime v) {
 check(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&std::isfinite(v.t),"nonfinite position");
}
}
double integrate(const std::function<double(double)>& f,double a,double b,double eps,double first_step) {
 check(std::isfinite(a)&&std::isfinite(b)&&eps>0&&std::isfinite(eps)&&first_step>0&&std::isfinite(first_step),"invalid quadrature parameters");
 if(a==b) return 0;
 double x=a,y=0,h=std::copysign(first_step,b-a);
 for(int step=0;step<100000;++step) {
  double dydx=f(x), scale=std::abs(y)+std::abs(h*dydx)+double(1.e-25f);
  if((x+h-b)*h>0) h=b-x;
  bool rejected=false;
  double next=h;
  for(int retry=0;;++retry) {
   if(x==x+h || retry>=100000) throw std::runtime_error("quadrature step underflow");
   double half=rk4(f,x,y,dydx,h/2);
   double y2=rk4(f,x+h/2,half,f(x+h/2),h/2);
   double y1=rk4(f,x,y,dydx,h), delta=y2-y1;
   double error=std::abs(delta)/(scale*eps);
   check(std::isfinite(error),"nonfinite quadrature integrand");
   if(error>1) { rejected=true; h*=std::max(0.1,double(0.9f)/std::pow(error,0.25)); continue; }
   next=rejected?h:std::min(5.,error==0?5.:double(0.9f)/std::pow(error,double(0.2f)))*h;
   x+=h; y=y2+delta/15; break;
  }
  if((x-b)*h>=0) return y;
  h=next;
 }
 throw std::runtime_error("quadrature exceeded JEWEL step limit");
}
ExpandingMedium::ExpandingMedium(MediumConfig c):config_(c) {
 check(c.mass_number>0&&c.nf>=0&&c.nf<=6,"invalid nucleus/flavour count");
 for(double v:{c.tau_initial,c.temperature_initial,c.nuclear_density,c.skin_depth,c.sigma_nn}) check(std::isfinite(v)&&v>0,"invalid positive medium parameter");
 for(double v:{c.temperature_cutoff,c.impact_parameter,c.max_rapidity,c.debye_floor,c.debye_scale}) check(std::isfinite(v)&&v>=0,"invalid medium parameter");
 // The radius expression in CALCTA is default REAL, promoted only on assignment.
 float mass=static_cast<float>(c.mass_number);
 double radius=1.12f*std::pow(mass,0.33333f)-0.86f*std::pow(mass,-0.33333f);
 for(int i=0;i<100;++i) {
  double b=i*2.*radius/100;
  auto f=[=](double z){return c.nuclear_density/(1+std::exp((std::sqrt(b*b+z*z)-radius)/c.skin_depth));};
  thickness_[i]={b,integrate(f,-2*radius,2*radius,double(1.e-4f),0.1)};
 }
}
double ExpandingMedium::thickness(double x,double y) const {
 check(std::isfinite(x)&&std::isfinite(y),"nonfinite transverse point");
 double r=std::sqrt(x*x+y*y), max=thickness_.back()[0];
 if(r>max) return 0;
 int index=std::clamp(int(r*99/max+1),1,99)-1;
 auto a=thickness_[index],b=thickness_[index+1];
 double slope=(b[1]-a[1])/(b[0]-a[0]), intercept=a[1]-slope*a[0];
 return slope*r+intercept;
}
double ExpandingMedium::participants(double x1,double y1,double x2,double y2) const {
 double a=thickness(x1,y1),b=thickness(x2,y2),s=config_.sigma_nn;
 return a*(1-std::exp(-s*b))+b*(1-std::exp(-s*a));
}
double ExpandingMedium::temperature(SpaceTime v) const {
 position_check(v);
 // Stock GETTEMP is singular on the light cone; explicitly define zero there.
 if(v.t<=std::abs(v.z)) return 0;
 const auto& c=config_; double b=c.impact_parameter;
 double tau=std::sqrt(v.t*v.t-v.z*v.z);
 if(thickness(v.x-b/2,v.y)==0 || thickness(v.x+b/2,v.y)==0) return 0;
 double rapidity=0.5*std::log((v.t+v.z)/(v.t-v.z));
 if(std::abs(rapidity)>c.max_rapidity) return 0;
 double initial=c.temperature_initial;
 if(c.woods_saxon) {
  double eps0=(16.*8+7.*2*6*c.nf)*pi*pi*std::pow(initial,4)/240;
  double eps=eps0*participants(v.x-b/2,v.y,v.x+b/2,v.y)/participants(0,0,0,0);
  initial=std::pow(eps*240/(pi*pi*(16.*8+7.*2*6*c.nf)),0.25);
 }
 double t=tau<=c.tau_initial?initial*tau/c.tau_initial:initial*std::pow(c.tau_initial/tau,double(0.3333f));
 return t<c.temperature_cutoff?0:t;
}
MediumSample ExpandingMedium::sample(SpaceTime v) const {
 double t=temperature(v);
 double md=std::max(config_.debye_scale*3*t,config_.debye_floor);
 double density=0;
 if(v.t>std::abs(v.z)) {
  double tau=std::sqrt(v.t*v.t-v.z*v.z),cosheta=v.t/tau;
  density=(2.*6*config_.nf*0.9*2/3+16.*1.2*3/2)*t*t*t/(pi*pi)/cosheta;
 }
 return {t,md,md/double(std::sqrt(2.f)),density};
}
ExponentialIntegral::ExponentialIntegral() {
 for(int i=1;i<=1000;++i) {
  double x=i*55./1000;
  table_[i-1]={x,-integrate([](double v){return std::exp(-v)/v;},x,1000.,1.e-6,double(0.01f)),positive_ei(x)};
 }
}
double ExponentialIntegral::operator()(double x) const {
 check(std::isfinite(x)&&std::abs(x)<=55,"Ei outside frozen table range [-55,55]");
 int index=std::clamp(int(std::abs(x)*1000/55),1,999)-1;
 int col=x>=0?2:1;
 const auto& a=table_[index]; const auto& b=table_[index+1];
 double slope=(b[col]-a[col])/(b[0]-a[0]), intercept=a[col]-slope*a[0];
 return slope*std::abs(x)+intercept;
}
std::array<std::array<double,3>,200> ExpandingMedium::cross_section_table() const {
 std::array<std::array<double,3>,200> table{};
 double previous=0;
 for(int ib=1;ib<=200;++ib) {
  double b=0.1*ib, product=1;
  for(int ix=1;ix<=100;++ix) for(int iy=1;iy<=100;++iy) {
   double x=-20.+ix*0.4,y=-20.+iy*0.4;
   product=product*std::pow(std::exp(-thickness(x+b/2,y)*config_.sigma_nn),0.16*thickness(x-b/2,y));
  }
  double probability=(1-product)*8.8/14*b;
  table[ib-1]={b,probability,ib==1?0:table[ib-2][2]+(probability+previous)/2*double(0.1f)};
  previous=probability;
 }
 return table;
}
double ExponentialIntegral::intpqglow(double z,double q,const Kernels& k) const {
 check(std::isfinite(z)&&z>0&&z<1&&std::isfinite(q)&&q>0&&std::isfinite(k.lambda_shower)&&k.lambda_shower>0&&k.nf>=0&&k.nf<=6,"invalid integrated kernel parameters");
 double l=k.lambda_shower,x=std::log(q*q/(l*l))+std::log(z);
 return 6*(l*l*(*this)(x)/(q*q)-2*std::pow(l,4)*(*this)(2*x)/std::pow(q,4)
  +2*std::pow(l,6)*(*this)(3*x)/std::pow(q,6))/((33-2*k.nf)*2);
}
double ExponentialIntegral::intpqghigh(double z,double q,const Kernels& k) const {
 check(std::isfinite(z)&&z>0&&z<1,"z must be in (0,1)");
 return -intpqglow(1-z,q,k);
}
double ExponentialIntegral::scatprimfunc(double t,double debye,const Kernels& k) const {
 check(std::isfinite(t)&&t>=0&&std::isfinite(debye)&&debye>=0,"invalid scattering primitive arguments");
 double l=k.lambda_scattering,s=t+debye*debye;
 k.alphas(s,l); // Validate flavour, Lambda and supported scale domain.
 double logarithm=std::log(s/(l*l));
 return 2*pi*std::pow(12*pi,2)*(-(*this)(-logarithm)/(l*l)-1/(s*logarithm))/std::pow(33-2*k.nf,2);
}

}
