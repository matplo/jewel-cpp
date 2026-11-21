// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/geometry.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
struct Constant: jewel::Medium {jewel::MediumSample sample(jewel::SpaceTime)const override{return {.5,0,0,0};}};
struct Heating: jewel::Medium {jewel::MediumSample sample(jewel::SpaceTime p)const override{return {p.t<1?0.:.5,0,0,0};}};
void close(double a,double b){if(std::abs(a-b)>1e-12)throw std::runtime_error("analytic geometry mismatch");}
int main(){
 Constant hot;Heating heating;jewel::Vacuum vacuum;
 auto g=jewel::straight_path_lengths(0,0,0,5,20,{3,0,4,5},.01,hot);
 close(g.geometric_transverse,5);close(g.hot,25./3);close(g.hot_transverse,5);close(g.temperature_cubed_integral,25./24);
 g=jewel::straight_path_lengths(0,0,0,5,2,{1,0,0,1},.25,heating);
 close(g.hot,1);close(g.temperature_cubed_integral,.125); // Do not stop at initial cold interval.
 g=jewel::straight_path_lengths(0,0,0,0,20,{},.02,vacuum);close(g.hot,0);close(g.geometric_transverse,0);
 g=jewel::straight_path_lengths(0,0,0,5,20,{0,0,0,1},.02,hot);close(g.hot,0);
 int rejected=0;
 for(int i=0;i<5;++i)try{
  jewel::straight_path_lengths(i==0?6:0,0,0,5,20,
   i==1?jewel::Momentum{2,0,0,1}:i==2?jewel::Momentum{0,0,0,0}:jewel::Momentum{1,0,0,1},
   i==3?0:i==4?1e-12:.02,hot);
 }catch(const std::domain_error&){++rejected;}
 if(rejected!=5)throw std::runtime_error("missing geometry domain check");
}
