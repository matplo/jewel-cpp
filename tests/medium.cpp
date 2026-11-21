// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/medium.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
void check(bool v) { if(!v) throw std::runtime_error("medium contract failed"); }
template<class F> void rejects(F f) { bool threw=false;try { f(); } catch(const std::exception&) { threw=true; } check(threw); }
int main() {
 auto f=[](double x){return x*x;};
 check(std::abs(jewel::integrate(f,0,1,1e-8,.1)-1./3)<1e-14);
 check(std::abs(jewel::integrate(f,1,0,1e-8,.1)+1./3)<1e-14);
 check(jewel::integrate(f,1,1,1e-8,.1)==0);
 rejects([&]{jewel::integrate(f,0,1,0,.1);});
 rejects([&]{jewel::integrate([](double){return std::numeric_limits<double>::quiet_NaN();},0,1,1e-8,.1);});
 jewel::MediumConfig c;
 jewel::ExpandingMedium medium(c);
 check(std::abs(medium.temperature({0,0,0,.4})-.59)<1e-14);
 auto outside=medium.sample({30,0,0,1});
 check(outside.temperature==0&&outside.density==0&&outside.debye_mass==.45);
 // Defined extension at singular/outside-forward-light-cone positions.
 for(auto point:{jewel::SpaceTime{0,0,0,0},jewel::SpaceTime{0,0,1,1},jewel::SpaceTime{0,0,2,1},jewel::SpaceTime{0,0,0,-1}}) {
  auto sample=medium.sample(point);check(sample.temperature==0&&sample.density==0);
 }
 rejects([&]{medium.sample({0,0,0,std::numeric_limits<double>::infinity()});});
 c.mass_number=0; rejects([&]{jewel::ExpandingMedium bad(c);});
 c.mass_number=208;c.skin_depth=-1;rejects([&]{jewel::ExpandingMedium bad(c);});
 jewel::ExponentialIntegral ei;
 rejects([&]{ei(55.00001);});rejects([&]{ei(-55.00001);});
 rejects([&]{ei(std::numeric_limits<double>::quiet_NaN());});
 check(std::isfinite(ei(0))); // Reference table extrapolation, not mathematical Ei(0).
 jewel::Kernels k;
 rejects([&]{ei.intpqglow(0,10,k);});rejects([&]{ei.intpqghigh(1,10,k);});
 rejects([&]{ei.intpqglow(.5,1e20,k);});
 rejects([&]{ei.scatprimfunc(-1,.5,k);});
}
