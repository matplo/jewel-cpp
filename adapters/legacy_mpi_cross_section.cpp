// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/legacy_mpi_cross_section.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {void check(bool v){if(!v)throw std::domain_error("invalid legacy MPI cross-section input");}}
LegacyMpiCrossSection::LegacyMpiCrossSection(LegacyMpiCrossSectionConfig c,const std::string& set,
 int member,std::optional<double> floor,std::function<double(double)> alpha)
 :config_(std::move(c)),pdf_(set,member,floor),alpha_s_(std::move(alpha)){
 check(bool(alpha_s_));
 for(int beam:{config_.beam_a,config_.beam_b})check(beam==2212||beam==-2212||beam==2112||beam==-2112);
 check(config_.max_flavour>=0&&config_.max_flavour<=6&&(config_.interference==0||config_.interference==1));
 for(double v:config_.coefficients)check(std::isfinite(v)&&v>=0);
 for(double v:config_.masses)check(std::isfinite(v)&&v>=0);
 if(config_.kinematic_pt_cut)check(std::isfinite(*config_.kinematic_pt_cut)&&*config_.kinematic_pt_cut>=0);
 for(double v:{config_.ecm,config_.screening_squared,config_.scale_shift,config_.renormalization_multiplier,
  config_.gev2_to_mb,config_.k_factor,config_.asymmetric_factor,config_.width_fudge})check(std::isfinite(v));
 check(config_.ecm>0&&config_.screening_squared>0&&config_.scale_shift>=0&&config_.renormalization_multiplier>0&&config_.gev2_to_mb>0&&config_.k_factor>0&&config_.asymmetric_factor>=0&&config_.width_fudge>=0);
}
LegacyMpiCrossSectionResult LegacyMpiCrossSection::evaluate(double xt,const LegacyMpiPhaseSpace& phase,double remaining_a,double remaining_b) const {
 check(std::isfinite(xt)&&xt>0&&xt<1&&std::isfinite(phase.tau)&&phase.tau>=xt&&phase.tau<1&&std::isfinite(phase.rapidity)&&std::isfinite(phase.cos_theta)&&std::abs(phase.cos_theta)<=1);
 for(double v:{remaining_a,remaining_b})check(std::isfinite(v)&&v>0&&v<=1);
 const auto& c=config_;LegacyMpiCrossSectionResult r;
 double x1=std::sqrt(phase.tau)*std::exp(phase.rapidity),x2=std::sqrt(phase.tau)*std::exp(-phase.rapidity);
 // PYSIGH's hadron beam endpoint return precedes all matrix/PDF work.
 if(x1>1.-1e-7||x2>1.-1e-7)return r;
 double collision_s=c.ecm*c.ecm;check(std::isfinite(collision_s));
 r.s=std::max(1.,phase.tau*collision_s);
 double pt=c.kinematic_pt_cut.value_or(.5*c.ecm*std::sqrt(xt)),pt2=pt*pt;
 double rpts=4*pt2/r.s,be34l=std::sqrt(std::max(0.,1.-rpts));
 double rthm=rpts/(1.+be34l);
 r.t=-.5*r.s*std::max(rthm,1.-phase.cos_theta);
 r.u=-.5*r.s*std::max(rthm,1.+phase.cos_theta);
 double sqpth=std::max(pt2,.25*r.s*(1.-phase.cos_theta*phase.cos_theta));
 const double screened_scale=sqpth+c.scale_shift;
 r.alpha_s=alpha_s_(c.renormalization_multiplier*screened_scale);
 r.q2=c.renormalization_multiplier*
  (c.pdf_scale==LegacyMpiPdfScale::SecondaryInitialization?sqpth:screened_scale);
 r.common_factor=legacy_mpi_common_factor(collision_s,xt,phase.tau,phase.rapidity,c.screening_squared,c.coefficients,c.gev2_to_mb,c.k_factor);
 r.pdf_a=pdf_.evaluate(c.beam_a,x1,remaining_a,r.q2,c.max_flavour,c.masses[3],c.masses[4]);
 r.pdf_b=pdf_.evaluate(c.beam_b,x2,remaining_b,r.q2,c.max_flavour,c.masses[3],c.masses[4]);
 r.annihilation_weight=legacy_gluon_width(r.s,c.decays,c.width_fudge).annihilation_weight;
 LegacyMpiChannelInput p;p.s=r.s;p.t=r.t;p.u=r.u;p.cos_theta=phase.cos_theta;p.common_factor=r.common_factor;
 p.alpha_s=r.alpha_s;p.asymmetric_factor=c.asymmetric_factor;p.annihilation_weight=r.annihilation_weight;
 p.interference=c.interference;p.masses=c.masses;p.enabled=c.enabled;
 r.channels=legacy_mpi_channels(p,r.pdf_a,r.pdf_b);return r;
}
LegacyMpiInitialization initialize_legacy_mpi_with_pdfs(
 const LegacyMpiScreeningConfig& screening,double core_fraction,double radius_ratio,
 Random& random,LegacyMpiCrossSectionConfig cross_section,const std::string& set,
 int member,std::optional<double> floor,std::function<double(double)> alpha_s){
 cross_section.ecm=screening.ecm;
 cross_section.kinematic_pt_cut.reset();
 auto factory=[&](LegacyMpiPdfScale policy){
  return [&,policy](double,double screen,double shift)->LegacyMpiIntegralEvaluator {
   auto config=cross_section;
   config.screening_squared=screen;
   config.scale_shift=shift;
   config.pdf_scale=policy;
   auto provider=std::make_shared<LegacyMpiCrossSection>(std::move(config),set,member,floor,alpha_s);
   return [provider](double xt,const LegacyMpiPhaseSpace& phase){
    return provider->evaluate(xt,phase,1,1).channels.total;
   };
  };
 };
 return initialize_legacy_mpi_lifecycle(screening,core_fraction,radius_ratio,random,
  factory(LegacyMpiPdfScale::Screened),factory(LegacyMpiPdfScale::SecondaryInitialization));
}

}
