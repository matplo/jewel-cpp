// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/radiation.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("radiation contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::Kernels k;jewel::InitialRadiation r(k,1);
 check(r.sudakov(1,1,jewel::Channel::QQ)==1);
 check(r.sudakov(2,1,jewel::Channel::QQ)==1);
 check(r.splitting(.3,jewel::Channel::GG)==0);
 check(r.splitting(.3,jewel::Channel::QQ)==0);
 auto gg=r.splitting(10,jewel::Channel::GG),qg=r.splitting(10,jewel::Channel::QG);
 check(r.splitting(10,jewel::Channel::GC)==gg+qg);
 rejects([&]{jewel::InitialRadiation bad(k,.1);});
 rejects([&]{r.splitting(0,jewel::Channel::QQ);});
 rejects([&]{r.sudakov(.5,10,jewel::Channel::QQ);});
 jewel::ExpandingMedium medium;
 jewel::InitialSudakovTable table(k,1,50,1,medium.maximum_scatterer(false));
 for(auto row:table.table())for(int i=1;i<5;++i)check(row[i]>=0&&row[i]<=1);
 check(table.ratio(1,1,jewel::Channel::GC)==1);
 check(table.ratio(1,2,jewel::Channel::GC)<=1);
 rejects([&]{table.reduced(0,jewel::Channel::QQ);});
 rejects([&]{table.reduced(10,jewel::Channel::GQ);});
 jewel::ExponentialIntegral ei;
 check(std::isfinite(ei.extrapolated(60)));
 rejects([&]{ei(60);}); // Explicit bounded and stock-extrapolating APIs stay distinct.
}
