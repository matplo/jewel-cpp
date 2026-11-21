// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/radiation.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("final splitting contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::Kernels k;jewel::ExpandingMedium medium;auto maximum=medium.maximum_scatterer(false);
 rejects([&]{jewel::FinalSplittingTable bad(k,1,1,1,maximum);});
 rejects([&]{jewel::FinalSplittingTable bad(k,.1,50,1,maximum);});
 jewel::FinalSplittingTable table(k,1,20,1,maximum);
 check(table.values().size()==1000000&&table.scales().front()==1);
 for(int j=0;j<1000;++j)for(double v:table.values()[j])check(v==0);
 check(table.splitting(100,1,1,100,jewel::Channel::QQ,0)==0);
 check(table.sudakov(1,100,1,1,100,jewel::Channel::GC,0)==1);
 check(table.sudakov(1,100,2,1,100,jewel::Channel::GC,0)==1);
 double a=table.splitting(100,2,1,100,jewel::Channel::QQ,0);
 for(int mode=1;mode<=5;++mode)check(table.splitting(100,2,1,100,jewel::Channel::QQ,mode)==a);
 rejects([&]{table.splitting(100,.5,1,100,jewel::Channel::QQ,0);});
 rejects([&]{table.splitting(100,2,1,1,jewel::Channel::QQ,0);});
 rejects([&]{table.splitting(100,2,1,100,jewel::Channel::GQ,0);});
 rejects([&]{table.splitting(100,2,1,100,jewel::Channel::QQ,6);});
 rejects([&]{table.splitting(1,2,.5,100,jewel::Channel::QQ,1);});
}
