// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/scattering.hpp"
#include <cmath>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("scattering contract failed");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);}
int main(){
 jewel::Kernels k;jewel::MediumConfig c;c.max_rapidity=2;c.debye_floor=2;
 jewel::ScatteringRates rates(k,1,10,1,jewel::ExpandingMedium(c),false);
 check(rates.values().size()==100000);
 check(rates.debye_masses().front()==rates.debye_masses().back());
 check(std::isfinite(rates.integral(10,2,jewel::RateChannel::QA)));
 check(rates.integral(10,1,jewel::RateChannel::QA)==rates.integral(10,3,jewel::RateChannel::QA));
 check(rates.cross_section({10,0,0,10},0,1000,'Q','C',{0,0,0,1})==0);
 check(rates.cross_section({10,0,0,10},0,0,'Q','C',{0,0,0,0})==0);
 check(rates.cross_section({10,0,0,10},0,0,'Q','C',{0,0,0,1})>=0);
 rejects([&]{rates.integral(0,2,jewel::RateChannel::QA);});
 rejects([&]{rates.integral(10,-1,jewel::RateChannel::QA);});
 rejects([&]{rates.cross_section({10,0,0,10},0,0,'X','C',{0,0,0,1});});
 rejects([&]{rates.cross_section({10,0,0,10},0,0,'Q','C',{0,0,0,1},3);});
 jewel::ReplayRandom empty({});
 auto none=rates.next_collision({10,0,0,10},0,1,{},1,5,20,0,false,empty);
 check(!none.collision&&none.elapsed==0&&empty.checkpoint()==0);
 none=rates.next_collision({10,0,0,10},0,1,{},5,1,2,1,false,empty);
 check(!none.collision&&none.elapsed==0&&empty.checkpoint()==0);
 rejects([&]{rates.next_collision({10,0,0,10},0,1,{},1,5,20,1,false,empty);});
 rejects([&]{rates.next_collision({},0,1,{},1,5,20,1,false,empty);});
 rejects([&]{rates.next_collision({10,0,0,10},0,1,{},1,-1,20,1,false,empty);});
}
