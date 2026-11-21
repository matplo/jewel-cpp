// SPDX-License-Identifier: GPL-2.0-or-later
// PYMULT numerical translation; Pythia authors: T. Sjostrand, S. Mrenna, P. Skands.
#include "jewel/legacy_mpi.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
namespace jewel {
namespace {void require(bool ok){if(!ok)throw std::domain_error("invalid legacy MPI input");}}
double LegacyMpiOverlap::overlap(double b2,double normalization) const {
 const double pi=std::acos(-1.);
 return (a_*std::exp(-std::min(50.,b2))+
  b_*inverse_mixed_*std::exp(-std::min(50.,b2*inverse_mixed_))+
  c_*inverse_core_*std::exp(-std::min(50.,b2*inverse_core_)))/(2*pi*normalization);
}
LegacyMpiOverlap::LegacyMpiOverlap(double f,double radius,double integral,double sigma){
 require(std::isfinite(f)&&f>=0&&f<=1&&std::isfinite(radius)&&radius>=.1&&radius<=1);
 require(std::isfinite(integral)&&std::isfinite(sigma)&&sigma>0&&integral>=1.1*sigma);
 a_=(1-f)*(1-f);b_=2*f*(1-f);c_=f*f;
 inverse_core_=1/(radius*radius);inverse_mixed_=2/(1+radius*radius);
 const double pi=std::acos(-1.),target=integral/std::max(1e-10,sigma),so=.5;
 require(std::isfinite(target)&&target>1);
 const double step=std::min(.01,.05*radius);
 double xi=0,yi=0,xf=0,yf=0,xk=.5;int stage=0;
 for(int iteration=0;iteration<10000;++iteration){
  if(stage==0)xk=2*xk;else if(stage==1)xk=.5*xk;
  else xk=xi+(target-yi)*(xf-xi)/(yf-yi);
  require(std::isfinite(xk)&&xk>0);
  double sp=0,sop=0,bsp=0,impact=-.5*step,division=0;bool divided=false,converged=false;
  for(int n=0;n<1000000;++n){
   impact+=step;double ov=overlap(impact*impact);
   double acceptance=1-std::exp(-std::min(50.,pi*xk*ov));
   sp=sp+2*pi*impact*step*acceptance;
   sop=sop+2*pi*impact*step*ov*acceptance;
   bsp=bsp+impact*(2*pi)*impact*step*acceptance;
   if(!divided&&pi*xk*ov<1){divided=true;division=impact+.5*step;}
   if(!(impact<1||impact*acceptance>1e-6)){converged=true;break;}
  }
  if(!converged)throw std::runtime_error("legacy MPI overlap integration did not converge");
  require(std::isfinite(sp)&&sp>0);
  double yk=pi*xk*so/sp;
  require(std::isfinite(yk));
  if(yk<target){xi=xk;yi=yk;if(stage==1)stage=2;}
  else {xf=xk;yf=yk;if(stage==0)stage=1;}
  if(std::abs(yk-target)<1e-5*target){
   state_={integral,sop/so,sop/sp,bsp/sp,division,(sop/so)/(sop/sp)*target};
   require(std::isfinite(state_.mean_impact)&&state_.mean_impact>0);
   return;
  }
 }
 throw std::runtime_error("legacy MPI overlap normalization did not converge");
}
LegacyMpiImpact LegacyMpiOverlap::sample_hard(Random& random) const {
 double type=random.uniform();require(std::isfinite(type)&&type>0&&type<1);
 double radial=random.uniform();require(std::isfinite(radial)&&radial>0&&radial<1);
 double b2=-std::log(radial);
 if(type<a_){}else if(type<a_+b_)b2/=inverse_mixed_;else b2/=inverse_core_;
 return {b2,overlap(b2,state_.overlap_normalization),std::sqrt(b2)/state_.mean_impact};
}
LegacyMpiScaleSampler::LegacyMpiScaleSampler(double screening,double average,
 double enhancement,double bound,double sigma):screening_(screening){
 require(std::isfinite(screening)&&screening>0&&std::isfinite(average)&&average>0);
 require(std::isfinite(enhancement)&&enhancement>=0&&std::isfinite(bound)&&bound>=0);
 require(std::isfinite(sigma)&&sigma>=0);
 factor_=average*enhancement*bound/std::max(1e-10,sigma)*screening*(1+screening);
 require(std::isfinite(factor_));
}
LegacyMpiScaleProposal LegacyMpiScaleSampler::next(double upper,Random& random) const {
 require(std::isfinite(upper)&&upper>=0);
 if(upper<=.01001*screening_)return {false,upper};
 double draw=random.uniform();require(std::isfinite(draw)&&draw>0&&draw<1);
 double candidate=factor_*(upper+screening_)/(factor_-(upper+screening_)*std::log(draw))-screening_;
 require(std::isfinite(candidate));
 if(candidate<=0)return {false,candidate};
 return {true,std::max(.01*screening_,candidate)};
}
LegacyMpiPhaseSpace sample_legacy_mpi_phase_space(double xt,
 const std::array<double,3>& coefficients,Random& random){
 require(std::isfinite(xt)&&xt>0&&xt<=1);
 for(double c:coefficients)require(std::isfinite(c)&&c>=0&&c<=1);
 require(coefficients[1]+coefficients[2]<=1);
 auto draw=[&](){double u=random.uniform();require(std::isfinite(u)&&u>0&&u<1);return u;};
 LegacyMpiPhaseSpace out;
 if(draw()<=coefficients[0]){
  double taut=std::pow(2*(1+std::sqrt(1-xt))/xt-1,draw());
  out.tau=xt*((1+taut)*(1+taut))/(4*taut);
 }else{
  double tangent=std::tan(draw()*std::atan(std::sqrt(1/xt-1)));
  out.tau=xt*(1+tangent*tangent);
 }
 require(std::isfinite(out.tau)&&out.tau>0);
 double low=.5*std::log(out.tau),high=-low;
 double kind=draw();int mode=1;
 if(kind>coefficients[1])mode=2;
 if(kind>coefficients[1]+coefficients[2])mode=3;
 double u=draw();
 if(mode==1)out.rapidity=low+(high-low)*std::sqrt(u);
 else if(mode==2)out.rapidity=high-(high-low)*std::sqrt(1-u);
 else {double upper=std::atan(std::exp(high)),lower=std::atan(std::exp(low));
  out.rapidity=std::log(std::tan(lower+(upper-lower)*u));}
 out.rapidity=std::min(high,std::max(low,out.rapidity));
 int sign=int(1.5+draw());
 out.cos_theta=std::sqrt(std::max(0.,1-xt/out.tau))*(sign%2?-1:1);
 out.x1=std::sqrt(out.tau)*std::exp(out.rapidity);
 out.x2=std::sqrt(out.tau)*std::exp(-out.rapidity);
 require(std::isfinite(out.rapidity)&&std::isfinite(out.x1)&&std::isfinite(out.x2));
 return out;
}
bool LegacyMpiPhaseSpace::momentum_available(double a,double b) const {
 require(std::isfinite(a)&&a>=0&&a<=1&&std::isfinite(b)&&b>=0&&b<=1);
 return !(a-x1<.01||b-x2<.01);
}
double legacy_mpi_common_factor(double s,double xt,double tau,double y,double screen,
 const std::array<double,5>& c,double units,double k){
 require(std::isfinite(s)&&s>0&&std::isfinite(xt)&&xt>0&&xt<1);
 require(std::isfinite(tau)&&tau>=xt&&tau<1&&std::isfinite(y));
 require(std::isfinite(screen)&&screen>0&&std::isfinite(units)&&units>0&&std::isfinite(k)&&k>=0);
 for(double v:c)require(std::isfinite(v)&&v>=0);
 double low=.5*std::log(tau),high=-low;require(y>=low&&y<=high);
 double sh=std::max(1.,tau*s),sh2=sh*sh;
 double factor=std::acos(-1.)*units*k*.5*s/sh2;
 double atau1=std::log(2*(1+std::sqrt(1-xt))/xt-1);
 double atau2=2*std::atan(1/xt-1)/std::sqrt(xt);
 double h1=c[0]+(atau1/atau2)*c[1]/std::sqrt(tau);
 require(std::isfinite(h1)&&h1>0);
 factor=factor*atau1/h1;
 double width=high-low,ayst1=.5*((high-low)*(high-low));
 double ayst3=2*(std::atan(std::exp(high))-std::atan(std::exp(low)));
 double h2=(width/ayst1)*c[2]*(y-low)+(width/ayst1)*c[3]*(high-y)+
  (width/ayst3)*c[4]/std::cosh(y);
 require(std::isfinite(h2)&&h2>0);
 factor=factor*width/h2;
 factor=factor*(xt*xt)/(screen*(1+screen));
 require(std::isfinite(factor)&&factor>=0);return factor;
}
LegacyMpiTable::LegacyMpiTable(std::array<double,20> sums,std::array<int,20> counts,
 double normalization):sums_(sums),counts_(counts),overlap_normalization_(normalization){
 require(std::isfinite(normalization)&&normalization>0);
 for(int i=0;i<20;++i)require(std::isfinite(sums[i])&&sums[i]>=0&&counts[i]>0);
}
LegacyMpiHardWeight LegacyMpiTable::hard_weight(double xt,double screening,
 double enhancement,double sigma,bool sudakov,double rescale) const {
 require(std::isfinite(xt)&&xt>=0&&xt<=1&&std::isfinite(screening)&&screening>0);
 require(std::isfinite(enhancement)&&enhancement>=0&&std::isfinite(sigma)&&sigma>=0);
 require(std::isfinite(rescale)&&rescale>0);
 LegacyMpiHardWeight out;
 out.bin_coordinate=std::max(.000001,std::min(.999999,xt*(1+screening)/(xt+screening)));
 out.bin=int(1+20*out.bin_coordinate);
 if(sudakov)return out;
 double fraction=out.bin-20*out.bin_coordinate;
 double count=fraction*counts_[out.bin-1],sum=fraction*sums_[out.bin-1];
 for(int i=out.bin;i<20;++i){count+=counts_[i];sum+=sums_[i];}
 out.cross_section_above=(sum/count)*screening*(1-xt)/(xt+screening);
 out.cross_section_above*=rescale;
 out.probability=std::exp(-std::min(50.,overlap_normalization_*enhancement*
   out.cross_section_above/std::max(1e-10,sigma)));
 require(std::isfinite(out.cross_section_above)&&std::isfinite(out.probability));
 return out;
}
std::optional<LegacyMpiPair> sample_legacy_mpi_pair(double sigma,double bound,
 double ecm,double xt,double x1,double x2,double c,double attach,double gluon,
 double strange,double two_pi,Random& random){
 for(double v:{sigma,bound,ecm,xt,x1,x2,c,attach,gluon,strange,two_pi})require(std::isfinite(v));
 require(sigma>=0&&bound>0&&ecm>0&&xt>0&&xt<=1&&x1>0&&x1<=1&&x2>0&&x2<=1);
 require(c>=-1&&c<=1&&attach>=0&&attach<=1&&gluon>=0&&gluon<=1&&strange>=0&&strange<=1&&two_pi>0);
 if(sigma<bound*random.uniform())return std::nullopt;
 LegacyMpiPair p;p.flavour_draw=random.uniform();p.pt=.5*ecm*std::sqrt(xt);
 p.phi=two_pi*random.uniform();p.pdg_a=21;
 if(p.flavour_draw>=std::max(attach,gluon))p.pdg_a=1+int((2.+strange)*random.uniform());
 p.pdg_b=p.pdg_a==21?21:-p.pdg_a;
 p.a={p.pt*std::cos(p.phi),p.pt*std::sin(p.phi),
  .25*ecm*(x1*(1+c)-x2*(1-c)),.25*ecm*(x1*(1+c)+x2*(1-c))};
 p.b={-p.a.px,-p.a.py,.25*ecm*(x1*(1-c)-x2*(1+c)),.25*ecm*(x1*(1-c)+x2*(1+c))};
 return p;
}

std::vector<LegacyMpiStringPiece> legacy_mpi_attach_strings(
 const std::vector<Momentum>& momenta,std::vector<LegacyMpiStringPiece> pieces,
 double flavour,double attach,int pdg){
 require(momenta.size()>=2&&std::isfinite(flavour)&&flavour>=0&&flavour<1&&std::isfinite(attach)&&attach>=0&&attach<=1);
 require(pdg==21||(pdg>=1&&pdg<=3));
 for(auto p:momenta)for(double v:{p.px,p.py,p.pz,p.energy})require(std::isfinite(v));
 const auto first=momenta.size()-2,second=first+1;
 for(auto s:pieces)require(s.first<first&&s.second<first&&s.first!=s.second);
 if(flavour<attach&&!pieces.empty()){
  require(pdg==21);
  auto dot=[](Momentum a,Momentum b){return a.energy*b.energy-a.px*b.px-a.py*b.py-a.pz*b.pz;};
  for(auto i:{first,second}){
   double minimum=1e8;std::size_t chosen=0;
   for(std::size_t j=0;j<pieces.size();++j){
    auto s=pieces[j];double distance=dot(momenta[i],momenta[s.first])*dot(momenta[i],momenta[s.second])/
     std::max(1.,dot(momenta[s.first],momenta[s.second]));
    require(std::isfinite(distance));
    if(j==0||distance<minimum){minimum=distance;chosen=j;}
   }
   auto end=pieces[chosen].second;pieces[chosen].second=i;pieces.push_back({i,end});
  }
 }else if(pdg==21){pieces.push_back({first,second});pieces.push_back({second,first});}
 else pieces.push_back({first,second});
 return pieces;
}

LegacyMpiEvolution::LegacyMpiEvolution(LegacyMpiEvolutionConfig c,LegacyMpiEvolutionState s)
 :config_(c),state_(std::move(s)),scales_(c.screening,c.overlap_average,c.enhancement,c.bound,c.sigma_nd){
 require(std::isfinite(c.ecm)&&c.ecm>0&&c.bound>0);
 for(double v:c.phase_coefficients)require(std::isfinite(v)&&v>=0&&v<=1);
 require(c.phase_coefficients[1]+c.phase_coefficients[2]<=1);
 for(double v:{c.attach_probability,c.gluon_probability,c.strange_weight})require(std::isfinite(v)&&v>=0&&v<=1);
 require(std::isfinite(c.two_pi)&&c.two_pi>0);
 require(std::isfinite(state_.upper_xt)&&state_.upper_xt>=0&&state_.upper_xt<=1);
 for(double v:{state_.remaining_a,state_.remaining_b})require(std::isfinite(v)&&v>=0&&v<=1);
 for(double v:{state_.used_a,state_.used_b,state_.sum_pt,state_.first_pt})require(std::isfinite(v)&&v>=0);
 require(state_.interactions>=1&&state_.interactions<=240&&state_.momenta.size()==state_.pdgs.size());
 for(auto p:state_.momenta)for(double v:{p.px,p.py,p.pz,p.energy})require(std::isfinite(v));
 for(auto piece:state_.strings)require(piece.first<state_.momenta.size()&&piece.second<state_.momenta.size()&&piece.first!=piece.second);
 if(state_.interactions==240)state_.stopped=true;
}
LegacyMpiStep LegacyMpiEvolution::step(Random& random,const CrossSection& cross_section){
 if(state_.stopped)return LegacyMpiStep::Stopped;
 auto proposal=scales_.next(state_.upper_xt,random);state_.upper_xt=proposal.xt_squared;
 if(!proposal.proposed){state_.stopped=true;return LegacyMpiStep::Stopped;}
 auto phase=sample_legacy_mpi_phase_space(state_.upper_xt,config_.phase_coefficients,random);
 if(!phase.momentum_available(state_.remaining_a,state_.remaining_b))return LegacyMpiStep::MomentumRejected;
 double sigma=cross_section(state_.upper_xt,phase,state_);
 auto pair=sample_legacy_mpi_pair(sigma,config_.bound,config_.ecm,state_.upper_xt,phase.x1,phase.x2,phase.cos_theta,
  config_.attach_probability,config_.gluon_probability,config_.strange_weight,config_.two_pi,random);
 if(!pair)return LegacyMpiStep::CrossSectionRejected;
 // Build topology before committing the accepted pair to the event state.
 auto momenta=state_.momenta;momenta.push_back(pair->a);momenta.push_back(pair->b);
 auto strings=legacy_mpi_attach_strings(momenta,state_.strings,pair->flavour_draw,config_.attach_probability,pair->pdg_a);
 state_.momenta=std::move(momenta);state_.strings=std::move(strings);
 state_.pdgs.push_back(pair->pdg_a);state_.pdgs.push_back(pair->pdg_b);
 ++state_.additional_pairs;state_.sum_pt+=pair->pt;
 if(state_.additional_pairs==1)state_.first_pt=pair->pt;
 ++state_.interactions;state_.used_a+=phase.x1;state_.used_b+=phase.x2;
 state_.remaining_a-=phase.x1;state_.remaining_b-=phase.x2;
 if(state_.interactions==240)state_.stopped=true;
 return LegacyMpiStep::Accepted;
}

std::vector<std::array<int,2>> legacy_mpi_colour_tags(
 const std::vector<LegacyMpiStringPiece>& pieces,const std::vector<int>& types,int first){
 require(first>0&&pieces.size()<=std::size_t(std::numeric_limits<int>::max()-first)+1);
 std::vector<std::array<int,2>> tags(types.size());
 for(int type:types)require(type==0||type==1||type==-1||type==2);
 for(std::size_t i=0;i<pieces.size();++i){auto s=pieces[i];
  require(s.first<types.size()&&s.second<types.size()&&s.first!=s.second);
  require(!tags[s.first][0]&&!tags[s.second][1]);
  tags[s.first][0]=tags[s.second][1]=first+int(i);
 }
 for(std::size_t i=0;i<types.size();++i){
  require(bool(tags[i][0])==(types[i]==1||types[i]==2));
  require(bool(tags[i][1])==(types[i]==-1||types[i]==2));
 }
 return tags;
}

LegacyMpiIntegralTable integrate_legacy_mpi(double screen,int samples,
 const std::array<double,3>& coefficients,Random& random,
 const std::function<double(double,const LegacyMpiPhaseSpace&)>& cross_section){
 require(std::isfinite(screen)&&screen>0&&samples>0&&bool(cross_section));
 for(double v:coefficients)require(std::isfinite(v)&&v>=0&&v<=1);
 require(coefficients[1]+coefficients[2]<=1);
 LegacyMpiIntegralTable out;
 for(int bin=0;bin<20;++bin){
  out.counts[bin]=samples;
  for(int sample=0;sample<samples;++sample){
   double u=random.uniform();require(std::isfinite(u)&&u>0&&u<1);
   double rsca=.05*((20-bin)-u);
   double xt=screen*(1+screen)/(screen+rsca)-screen;
   xt=std::max(.01*screen,xt);
   auto phase=sample_legacy_mpi_phase_space(xt,coefficients,random);
   double sigma=cross_section(xt,phase);require(std::isfinite(sigma)&&sigma>=0);
   out.sums[bin]+=sigma;
  }
  out.integral+=out.sums[bin];
 }
 out.integral=out.integral/(20.*samples);require(std::isfinite(out.integral));
 return out;
}

std::vector<LegacyMpiInitializationPass> initialize_legacy_mpi(
 const LegacyMpiScreeningConfig& c,Random& random,
 const std::function<LegacyMpiIntegralEvaluator(double,double,double)>& factory){
 for(double v:{c.ecm,c.pt0_reference,c.reference_energy,c.energy_power,c.sigma_nd})require(std::isfinite(v));
 require(c.ecm>0&&c.pt0_reference>0&&c.reference_energy>0&&c.sigma_nd>0&&c.samples_per_bin>0&&c.max_passes>0&&bool(factory));
 std::vector<LegacyMpiInitializationPass> passes;double pt0=c.pt0_reference;
 for(unsigned iteration=0;iteration<c.max_passes;++iteration){
  double pt=pt0*std::pow(c.ecm/c.reference_energy,c.energy_power);
  double shift=pt*pt,screen=4*shift/(c.ecm*c.ecm);
  require(std::isfinite(shift)&&shift>0&&std::isfinite(screen)&&screen>0);
  auto evaluator=factory(pt0,screen,shift);
  auto table=integrate_legacy_mpi(screen,c.samples_per_bin,c.phase_coefficients,random,evaluator);
  passes.push_back({pt0,screen,shift,std::move(table)});
  if(!(passes.back().table.integral<1.1*c.sigma_nd))return passes;
  pt0=.9*pt0;
 }
 throw std::runtime_error("legacy MPI screening initialization did not meet acceptance threshold");
}

LegacyMpiInitialization initialize_legacy_mpi_lifecycle(
 LegacyMpiScreeningConfig config,double core_fraction,double radius_ratio,Random& random,
 const LegacyMpiEvaluatorFactory& legacy_factory,
 const LegacyMpiEvaluatorFactory& secondary_factory){
 auto legacy=initialize_legacy_mpi(config,random,legacy_factory);
 const auto& accepted=legacy.back();
 LegacyMpiOverlap overlap(core_fraction,radius_ratio,accepted.table.integral,config.sigma_nd);
 LegacyMpiTable hard_table(accepted.table.sums,accepted.table.counts,overlap.state().overlap_average);
 config.pt0_reference=accepted.pt0_reference;
 auto secondary=initialize_legacy_mpi(config,random,secondary_factory);
 return {std::move(legacy),std::move(secondary),std::move(overlap),std::move(hard_table)};
}

}
