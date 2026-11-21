// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/scattering.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
int main() {
 try {
 jewel::ExponentialIntegral ei;
 std::unique_ptr<jewel::ExpandingMedium> medium;
 std::unique_ptr<jewel::InitialRadiation> radiation;
 std::unique_ptr<jewel::InitialSudakovTable> sudakov;
 std::unique_ptr<jewel::FinalSplittingTable> final_table;
 std::unique_ptr<jewel::PartonicPdf> pdf;
 std::unique_ptr<jewel::ScatteringRates> rates;
 std::string op; std::cout<<std::scientific<<std::setprecision(17);
 while(std::cin>>op) {
  if(op=="CONFIG") {
   jewel::MediumConfig c; int woods;
   std::cin>>c.mass_number>>c.nf>>woods>>c.tau_initial>>c.temperature_initial>>c.temperature_cutoff
    >>c.nuclear_density>>c.skin_depth>>c.sigma_nn>>c.impact_parameter>>c.max_rapidity>>c.debye_floor>>c.debye_scale;
   if(!std::cin) throw std::runtime_error("bad configuration");
   c.woods_saxon=woods!=0; medium=std::make_unique<jewel::ExpandingMedium>(c);
  } else if(op=="EI") { double x; if(!(std::cin>>x)) throw std::runtime_error("bad EI input"); std::cout<<ei(x)<<'\n'; }
  else if(op=="LOW"||op=="HIGH"||op=="SCAT") {
   jewel::Kernels k; double x,y;
   if(!(std::cin>>k.nf>>x>>y>>k.lambda_shower>>k.lambda_scattering)) throw std::runtime_error("bad kernel input");
   double v=op=="LOW"?ei.intpqglow(x,y,k):op=="HIGH"?ei.intpqghigh(x,y,k):ei.scatprimfunc(x,y,k);
   std::cout<<v<<'\n';
  }
  else if(op=="EITABLE") { for(auto row:ei.table()) std::cout<<row[0]<<' '<<row[1]<<' '<<row[2]<<'\n'; }
  else {
   if(!medium) throw std::runtime_error("CONFIG required");
   if(op=="XCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>k.lambda_scattering>>emax>>scale>>boost))throw std::runtime_error("bad rate configuration");
    rates=std::make_unique<jewel::ScatteringRates>(k,q0,emax,scale,*medium,boost!=0);
   } else if(op=="XDUMP") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    std::string filename;std::getline(std::cin>>std::ws,filename);std::ofstream out(filename,std::ios::binary);
    for(double x:rates->transfers())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(double x:rates->debye_masses())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(auto row:rates->values())for(double x:row)out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    if(!out)throw std::runtime_error("rate dump failed");
   } else if(op=="XINT") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    std::string name;double t,md;if(!(std::cin>>name>>t>>md))throw std::runtime_error("bad rate query");
    jewel::RateChannel c;
    if(name=="QA")c=jewel::RateChannel::QA;else if(name=="QB")c=jewel::RateChannel::QB;
    else if(name=="GA")c=jewel::RateChannel::GA;else if(name=="GB")c=jewel::RateChannel::GB;else throw std::runtime_error("bad rate channel");
    std::cout<<rates->integral(t,md,c)<<'\n';
   } else if(op=="XSEC") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    jewel::Momentum p;jewel::SpaceTime v;double mass,lower;char incoming,outgoing;int mode;
    if(!(std::cin>>p.px>>p.py>>p.pz>>p.energy>>mass>>lower>>incoming>>outgoing>>v.x>>v.y>>v.z>>v.t>>mode))throw std::runtime_error("bad cross section query");
    std::cout<<rates->cross_section(p,mass,lower,incoming,outgoing,v,mode)<<'\n';
   } else if(op=="PCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("bad partonic PDF config");
    pdf=std::make_unique<jewel::PartonicPdf>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="PTABLE") {
    if(!pdf)throw std::runtime_error("PCONFIG required");
    for(auto row:pdf->table()){for(double x:row)std::cout<<x<<' ';std::cout<<'\n';}
   } else if(op=="PDF"||op=="PEXACT"||op=="PINT") {
    if(!pdf)throw std::runtime_error("PCONFIG required");
    std::string name;double x,y;if(!(std::cin>>name>>x>>y))throw std::runtime_error("bad PDF query");
    jewel::Channel c;
    if(name=="QQ")c=jewel::Channel::QQ;else if(name=="QG")c=jewel::Channel::QG;
    else if(name=="GQ")c=jewel::Channel::GQ;else if(name=="GG")c=jewel::Channel::GG;else throw std::runtime_error("invalid PDF channel");
    std::cout<<(op=="PDF"?pdf->density(x,y,c):op=="PEXACT"?pdf->integrated_exact(x,c):pdf->integrated(x,c))<<'\n';
   } else if(op=="FCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("bad final splitting config");
    final_table=std::make_unique<jewel::FinalSplittingTable>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="FDUMP") {
    if(!final_table)throw std::runtime_error("FCONFIG required");
    std::string filename;std::getline(std::cin>>std::ws,filename);std::ofstream out(filename,std::ios::binary);
    for(double x:final_table->scales())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(double x:final_table->fractions())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(auto row:final_table->values())for(double x:row)out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    if(!out)throw std::runtime_error("table dump failed");
   } else if(op=="FSPLIT"||op=="FSUDA") {
    if(!final_table)throw std::runtime_error("FCONFIG required");
    std::string channel;double parent,q,zeta,energy,high=0;int angular;
    std::cin>>channel;if(op=="FSUDA")std::cin>>high;
    if(!(std::cin>>parent>>q>>zeta>>energy>>angular))throw std::runtime_error("bad final splitting query");
    jewel::Channel c;
    if(channel=="QQ")c=jewel::Channel::QQ;else if(channel=="QG")c=jewel::Channel::QG;
    else if(channel=="GG")c=jewel::Channel::GG;else if(channel=="GC")c=jewel::Channel::GC;else throw std::runtime_error("unsupported final channel");
    std::cout<<(op=="FSPLIT"?final_table->splitting(parent,q,zeta,energy,c,angular):final_table->sudakov(high,parent,q,zeta,energy,c,angular))<<'\n';
   } else if(op=="RCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("invalid radiation configuration");
    radiation=std::make_unique<jewel::InitialRadiation>(k,q0);
    sudakov=std::make_unique<jewel::InitialSudakovTable>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="RTABLE") {
    if(!sudakov)throw std::runtime_error("RCONFIG required");
    for(auto row:sudakov->table()) { for(double x:row)std::cout<<x<<' ';std::cout<<'\n'; }
   } else if(op=="RSPLIT"||op=="RSUDA"||op=="RRED"||op=="RFAST") {
    if(!sudakov)throw std::runtime_error("RCONFIG required");
    std::string channel;double x,y;if(!(std::cin>>channel>>x>>y))throw std::runtime_error("invalid radiation probe");
    jewel::Channel c;
    if(channel=="QQ")c=jewel::Channel::QQ;else if(channel=="QG")c=jewel::Channel::QG;
    else if(channel=="GG")c=jewel::Channel::GG;else if(channel=="GC")c=jewel::Channel::GC;
    else if(channel=="GQ")c=jewel::Channel::GQ;else throw std::runtime_error("unknown channel");
    double result=op=="RSPLIT"?radiation->splitting(x,c):op=="RSUDA"?radiation->sudakov(x,y,c):op=="RRED"?sudakov->reduced(x,c):sudakov->ratio(x,y,c);
    std::cout<<result<<'\n';
   } else if(op=="BOUNDS") {
    auto v=medium->bounds();std::cout<<v.temperature_max<<' '<<v.debye_max<<' '<<v.debye_min<<' '<<v.scatterer_mass_max<<' '<<v.density_at_debye_min<<' '<<v.lifetime_max<<' '<<v.density_max<<'\n';
   } else if(op=="AVERAGE"||op=="MAXIMUM") {
    jewel::SpaceTime v;int boost;
    if(op=="AVERAGE" && !(std::cin>>v.x>>v.y>>v.z>>v.t)) throw std::runtime_error("bad position");
    if(!(std::cin>>boost)) throw std::runtime_error("bad boost");
    auto a=op=="AVERAGE"?medium->average_scatterer(v,boost!=0):medium->maximum_scatterer(boost!=0);
    std::cout<<a.momentum.px<<' '<<a.momentum.py<<' '<<a.momentum.pz<<' '<<a.momentum.energy<<' '<<a.mass<<'\n';
   } else if(op=="EVENT"||op=="VERTEX"||op=="SCATTER") {
    double a=0,b=0; jewel::SpaceTime point; int boost=1;
    if(op=="SCATTER") std::cin>>point.x>>point.y>>point.z>>point.t>>boost;
    else std::cin>>a>>b;
    std::size_t count;
    if(!(std::cin>>count)||count>100000) throw std::runtime_error("invalid replay size");
    std::vector<double> tape(count); for(double& r:tape) if(!(std::cin>>r)) throw std::runtime_error("short replay");
    jewel::ReplayRandom random(tape);
    if(op=="XCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>k.lambda_scattering>>emax>>scale>>boost))throw std::runtime_error("bad rate configuration");
    rates=std::make_unique<jewel::ScatteringRates>(k,q0,emax,scale,*medium,boost!=0);
   } else if(op=="XDUMP") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    std::string filename;std::getline(std::cin>>std::ws,filename);std::ofstream out(filename,std::ios::binary);
    for(double x:rates->transfers())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(double x:rates->debye_masses())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(auto row:rates->values())for(double x:row)out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    if(!out)throw std::runtime_error("rate dump failed");
   } else if(op=="XINT") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    std::string name;double t,md;if(!(std::cin>>name>>t>>md))throw std::runtime_error("bad rate query");
    jewel::RateChannel c;
    if(name=="QA")c=jewel::RateChannel::QA;else if(name=="QB")c=jewel::RateChannel::QB;
    else if(name=="GA")c=jewel::RateChannel::GA;else if(name=="GB")c=jewel::RateChannel::GB;else throw std::runtime_error("bad rate channel");
    std::cout<<rates->integral(t,md,c)<<'\n';
   } else if(op=="XSEC") {
    if(!rates)throw std::runtime_error("XCONFIG required");
    jewel::Momentum p;jewel::SpaceTime v;double mass,lower;char incoming,outgoing;int mode;
    if(!(std::cin>>p.px>>p.py>>p.pz>>p.energy>>mass>>lower>>incoming>>outgoing>>v.x>>v.y>>v.z>>v.t>>mode))throw std::runtime_error("bad cross section query");
    std::cout<<rates->cross_section(p,mass,lower,incoming,outgoing,v,mode)<<'\n';
   } else if(op=="PCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("bad partonic PDF config");
    pdf=std::make_unique<jewel::PartonicPdf>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="PTABLE") {
    if(!pdf)throw std::runtime_error("PCONFIG required");
    for(auto row:pdf->table()){for(double x:row)std::cout<<x<<' ';std::cout<<'\n';}
   } else if(op=="PDF"||op=="PEXACT"||op=="PINT") {
    if(!pdf)throw std::runtime_error("PCONFIG required");
    std::string name;double x,y;if(!(std::cin>>name>>x>>y))throw std::runtime_error("bad PDF query");
    jewel::Channel c;
    if(name=="QQ")c=jewel::Channel::QQ;else if(name=="QG")c=jewel::Channel::QG;
    else if(name=="GQ")c=jewel::Channel::GQ;else if(name=="GG")c=jewel::Channel::GG;else throw std::runtime_error("invalid PDF channel");
    std::cout<<(op=="PDF"?pdf->density(x,y,c):op=="PEXACT"?pdf->integrated_exact(x,c):pdf->integrated(x,c))<<'\n';
   } else if(op=="FCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("bad final splitting config");
    final_table=std::make_unique<jewel::FinalSplittingTable>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="FDUMP") {
    if(!final_table)throw std::runtime_error("FCONFIG required");
    std::string filename;std::getline(std::cin>>std::ws,filename);std::ofstream out(filename,std::ios::binary);
    for(double x:final_table->scales())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(double x:final_table->fractions())out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    for(auto row:final_table->values())for(double x:row)out.write(reinterpret_cast<const char*>(&x),sizeof(x));
    if(!out)throw std::runtime_error("table dump failed");
   } else if(op=="FSPLIT"||op=="FSUDA") {
    if(!final_table)throw std::runtime_error("FCONFIG required");
    std::string channel;double parent,q,zeta,energy,high=0;int angular;
    std::cin>>channel;if(op=="FSUDA")std::cin>>high;
    if(!(std::cin>>parent>>q>>zeta>>energy>>angular))throw std::runtime_error("bad final splitting query");
    jewel::Channel c;
    if(channel=="QQ")c=jewel::Channel::QQ;else if(channel=="QG")c=jewel::Channel::QG;
    else if(channel=="GG")c=jewel::Channel::GG;else if(channel=="GC")c=jewel::Channel::GC;else throw std::runtime_error("unsupported final channel");
    std::cout<<(op=="FSPLIT"?final_table->splitting(parent,q,zeta,energy,c,angular):final_table->sudakov(high,parent,q,zeta,energy,c,angular))<<'\n';
   } else if(op=="RCONFIG") {
    jewel::Kernels k;double q0,emax,scale;int boost;
    if(!(std::cin>>k.nf>>q0>>k.lambda_shower>>emax>>scale>>boost))throw std::runtime_error("invalid radiation configuration");
    radiation=std::make_unique<jewel::InitialRadiation>(k,q0);
    sudakov=std::make_unique<jewel::InitialSudakovTable>(k,q0,emax,scale,medium->maximum_scatterer(boost!=0));
   } else if(op=="RTABLE") {
    if(!sudakov)throw std::runtime_error("RCONFIG required");
    for(auto row:sudakov->table()) { for(double x:row)std::cout<<x<<' ';std::cout<<'\n'; }
   } else if(op=="RSPLIT"||op=="RSUDA"||op=="RRED"||op=="RFAST") {
    if(!sudakov)throw std::runtime_error("RCONFIG required");
    std::string channel;double x,y;if(!(std::cin>>channel>>x>>y))throw std::runtime_error("invalid radiation probe");
    jewel::Channel c;
    if(channel=="QQ")c=jewel::Channel::QQ;else if(channel=="QG")c=jewel::Channel::QG;
    else if(channel=="GG")c=jewel::Channel::GG;else if(channel=="GC")c=jewel::Channel::GC;
    else if(channel=="GQ")c=jewel::Channel::GQ;else throw std::runtime_error("unknown channel");
    double result=op=="RSPLIT"?radiation->splitting(x,c):op=="RSUDA"?radiation->sudakov(x,y,c):op=="RRED"?sudakov->reduced(x,c):sudakov->ratio(x,y,c);
    std::cout<<result<<'\n';
   } else if(op=="BOUNDS") {
    auto v=medium->bounds();std::cout<<v.temperature_max<<' '<<v.debye_max<<' '<<v.debye_min<<' '<<v.scatterer_mass_max<<' '<<v.density_at_debye_min<<' '<<v.lifetime_max<<' '<<v.density_max<<'\n';
   } else if(op=="AVERAGE"||op=="MAXIMUM") {
    jewel::SpaceTime v;int boost;
    if(op=="AVERAGE" && !(std::cin>>v.x>>v.y>>v.z>>v.t)) throw std::runtime_error("bad position");
    if(!(std::cin>>boost)) throw std::runtime_error("bad boost");
    auto a=op=="AVERAGE"?medium->average_scatterer(v,boost!=0):medium->maximum_scatterer(boost!=0);
    std::cout<<a.momentum.px<<' '<<a.momentum.py<<' '<<a.momentum.pz<<' '<<a.momentum.energy<<' '<<a.mass<<'\n';
   } else if(op=="EVENT") {
     jewel::GeometrySampler sampler(medium->cross_section_table());auto event=sampler.sample(a,b,random);
     *medium=medium->with_impact_parameter(event.impact_parameter);
     std::cout<<event.impact_parameter<<' '<<event.centrality_fraction;
    } else if(op=="VERTEX") {
     *medium=medium->with_impact_parameter(a);auto v=medium->sample_vertex(random,b);
     std::cout<<v.x<<' '<<v.y;
    } else {
     auto v=medium->sample_scatterer(point,random,boost!=0);
     std::cout<<v.type<<' '<<v.momentum.px<<' '<<v.momentum.py<<' '<<v.momentum.pz<<' '<<v.momentum.energy<<' '<<v.mass;
    }
    std::cout<<' '<<random.checkpoint()<<'\n';
   }
   else if(op=="CROSS") { for(auto row:medium->cross_section_table()) std::cout<<row[0]<<' '<<row[1]<<' '<<row[2]<<'\n'; }
   else if(op=="TABLE") { for(auto row:medium->thickness_table()) std::cout<<row[0]<<' '<<row[1]<<'\n'; }
   else if(op=="THICK") { double x,y; if(!(std::cin>>x>>y)) throw std::runtime_error("bad point"); std::cout<<medium->thickness(x,y)<<'\n'; }
   else if(op=="PART") { double x,y,z,t; if(!(std::cin>>x>>y>>z>>t)) throw std::runtime_error("bad pair"); std::cout<<medium->participants(x,y,z,t)<<'\n'; }
   else if(op=="POINT") {
    jewel::SpaceTime v; if(!(std::cin>>v.x>>v.y>>v.z>>v.t)) throw std::runtime_error("bad position");
    auto s=medium->sample(v); std::cout<<s.temperature<<' '<<s.debye_mass<<' '<<s.scatterer_mass<<' '<<s.density<<'\n';
   } else throw std::runtime_error("unknown operation");
  }
 }
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 2; }
}
