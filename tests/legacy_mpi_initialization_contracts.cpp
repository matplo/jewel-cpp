#include "jewel/legacy_mpi.hpp"
#include <cmath>
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("MPI initialization contract failed");}
int main(){using namespace jewel;
 LegacyMpiScreeningConfig c;c.ecm=5020;c.pt0_reference=4;c.reference_energy=1800;c.energy_power=.25;c.sigma_nd=10;c.samples_per_bin=1;c.phase_coefficients={1,.5,.5};
 ReplayRandom equal(std::vector<double>(120,.5));unsigned calls=0;
 auto accepted=initialize_legacy_mpi(c,equal,[&](double,double,double){++calls;return [](double,const auto&){return 11.;};});
 check(accepted.size()==1&&accepted[0].table.integral==11&&calls==1&&equal.checkpoint()==120);
 ReplayRandom retry(std::vector<double>(240,.5));calls=0;
 auto retried=initialize_legacy_mpi(c,retry,[&](double,double,double){double sigma=calls++==0?0:100;return [sigma](double,const auto&){return sigma;};});
 check(retried.size()==2&&retried[0].table.integral==0&&retried[1].table.integral==100&&retry.checkpoint()==240);
 check(retried[1].pt0_reference==3.6&&std::abs(retried[1].screening_squared/retried[0].screening_squared-.81)<1e-15);
 c.max_passes=1;ReplayRandom exhausted(std::vector<double>(120,.5));bool failed=false;
 try{initialize_legacy_mpi(c,exhausted,[](double,double,double){return [](double,const auto&){return 0.;};});}catch(const std::runtime_error&){failed=true;}
 check(failed&&exhausted.checkpoint()==120);
 c.max_passes=0;ReplayRandom invalid({});failed=false;
 try{initialize_legacy_mpi(c,invalid,[](double,double,double){return [](double,const auto&){return 0.;};});}catch(const std::domain_error&){failed=true;}
 check(failed&&invalid.checkpoint()==0);
 c.max_passes=3;
 ReplayRandom lifecycle_draws(std::vector<double>(480,.5));
 calls=0;
 auto make_legacy=[&](double pt0,double,double){
  check(lifecycle_draws.checkpoint()==120*calls);
  check(pt0==(calls==0?4.:3.6));
  const double weight=calls++==0?0.:20.;
  return [weight](double,const auto&){return weight;};
 };
 unsigned secondary_calls=0;
 auto make_secondary=[&](double pt0,double,double){
  check(lifecycle_draws.checkpoint()==240+120*secondary_calls);
  check(pt0==(secondary_calls==0?3.6:.9*3.6));
  const double weight=secondary_calls++==0?0.:30.;
  return [weight](double,const auto&){return weight;};
 };
 auto initialized=initialize_legacy_mpi_lifecycle(c,.5,.4,lifecycle_draws,make_legacy,make_secondary);
 check(initialized.legacy.size()==2&&initialized.secondary.size()==2&&lifecycle_draws.checkpoint()==480);
 check(initialized.overlap.state().integral==20&&initialized.secondary.back().table.integral==30);
 const auto screening=initialized.secondary.back().screening_squared;
 LegacyMpiTable expected(initialized.legacy.back().table.sums,initialized.legacy.back().table.counts,
  initialized.overlap.state().overlap_average);
 check(initialized.hard_table.hard_weight(screening,screening,1,10).probability==
  expected.hard_weight(screening,screening,1,10).probability);

}
