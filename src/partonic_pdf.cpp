// SPDX-License-Identifier: GPL-2.0-or-later
// JEWEL 2.6.0 GETPDF, GETPDFXINTEXACT, PDFINT and GETPDFXINT translations.
#include "jewel/radiation.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace jewel {
namespace {
constexpr double pi=3.141592653589793;
void check(bool b,const char* why){if(!b)throw std::domain_error(why);}
int column(Channel c){switch(c){case Channel::QQ:return 1;case Channel::GQ:return 2;case Channel::QG:return 3;case Channel::GG:return 4;default:throw std::domain_error("GC is not a partonic PDF channel");}}
}
PartonicPdf::PartonicPdf(Kernels k,double q0,double emax,double scale,const Scatterer& maximum):kernels_(k),q0_(q0),radiation_(k,q0),sudakov_(k,q0,emax,scale,maximum) {
 auto p=maximum.momentum;double m=maximum.mass;
 double shat=m*m+2*emax*(p.energy+std::sqrt(p.px*p.px+p.py*p.py+p.pz*p.pz));
 double pcms2=std::pow(shat-m*m,2)/(4*shat),q2max=scale*4*pcms2,delta=std::log(q2max)-std::log(q0*q0);
 check(delta>0&&std::isfinite(delta),"invalid partonic PDF table scale");
 table_[0]={q0*q0,0,0,0,0};
 for(int j=1;j<1000;++j) {
  double q2=std::exp(j*delta/999+std::log(q0*q0)),q=std::sqrt(q2);
  table_[j]={q2,integrated_exact(q,Channel::QQ),integrated_exact(q,Channel::GQ),integrated_exact(q,Channel::QG),integrated_exact(q,Channel::GG)};
 }
}
double PartonicPdf::density(double z,double q,Channel c) const {
 column(c);check(std::isfinite(z)&&std::isfinite(q),"nonfinite PDF input");
 if(z<0||z>1||q<q0_)return 0; // GETPDF's stock out-of-domain return.
 if(z>1-1e-10)return 0;
 bool gluon=c==Channel::GQ||c==Channel::GG;
 if(gluon&&z<1e-10)return 0;
 double low;
 if(gluon)low=std::max(q0_,std::max(q0_/(2*std::sqrt(z)),q0_/(2*std::sqrt(1-z))));
 else low=std::max(q0_,q0_/(2*std::sqrt(1-z)));
 if(low>=q*(1-1e-10))return 0;
 auto f=[&](double x){
  double factor=2*sudakov_.ratio(x,q,gluon?Channel::GC:Channel::QQ)*kernels_.alphas((1-z)*(x*x),kernels_.lambda_shower);
  switch(c){
   case Channel::QQ:return factor*(4*(1+z*z)/(3*(1-z)))/(2*pi*x);
   case Channel::GQ:return factor*kernels_.pgq(z)/(2*pi*x);
   case Channel::QG:return factor*(.5*(z*z+(1-z)*(1-z)))/(2*pi*x);
   case Channel::GG:return factor*2*kernels_.pgg(z)/(2*pi*x);
   default:throw std::domain_error("invalid PDF channel");
  }
 };
 return integrate(f,low,q,1e-4,double(.01f)*(q-low));
}
double PartonicPdf::integrated_exact(double q,Channel c) const {
 column(c);check(std::isfinite(q)&&q>=q0_,"invalid integrated PDF scale");
 auto f=[&](double x){
  bool gluon=c==Channel::GQ||c==Channel::GG;
  double value=2*sudakov_.ratio(x,q,gluon?Channel::GC:Channel::QQ);
  if(c==Channel::GG)value*=2;
  return value*radiation_.splitting(x,c)/x;
 };
 return integrate(f,q0_,q,1e-4,.01);
}
double PartonicPdf::integrated(double q,Channel c) const {
 int col=column(c);check(std::isfinite(q)&&q>0,"invalid PDF table lookup scale");
 double q2=q*q;
 int i=int(std::clamp((std::log(q2)-std::log(table_[0][0]))*999/(std::log(table_[999][0])-std::log(table_[0][0]))+1,1.,999.))-1;
 auto a=table_[i],b=table_[i+1];double slope=(b[col]-a[col])/(b[0]-a[0]),intercept=a[col]-slope*a[0];return slope*q2+intercept;
}
}
