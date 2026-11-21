// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/radiation.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("partonic PDF contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::Kernels k;jewel::MediumConfig config;config.impact_parameter=3;config.max_rapidity=2;
 jewel::ExpandingMedium medium(config);
 jewel::PartonicPdf pdf(k,1,50,1,medium.maximum_scatterer(false));
 check(pdf.table().size()==1000&&pdf.table()[0][0]==1);
 for(int i=1;i<5;++i)check(pdf.table()[0][i]==0);
 check(pdf.integrated_exact(1,jewel::Channel::QQ)==0);
 check(pdf.density(-.1,10,jewel::Channel::QQ)==0);
 check(pdf.density(1.1,10,jewel::Channel::GG)==0);
 check(pdf.density(.5,.5,jewel::Channel::QQ)==0);
 check(pdf.density(0,10,jewel::Channel::GG)==0);
 check(pdf.density(1,10,jewel::Channel::QQ)==0);
 check(pdf.density(0,10,jewel::Channel::QQ)>0);
 check(pdf.density(0,10,jewel::Channel::QG)>0);
 rejects([&]{pdf.integrated(0,jewel::Channel::QQ);});
 rejects([&]{pdf.integrated_exact(.5,jewel::Channel::QQ);});
 rejects([&]{pdf.density(.5,10,jewel::Channel::GC);});
 rejects([&]{pdf.density(0,100,jewel::Channel::QQ);});
}
