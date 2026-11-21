// SPDX-License-Identifier: GPL-2.0-or-later
#include "jewel/legacy_mpi_pdf.hpp"
#include <LHAPDF/LHAPDF.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jewel {
namespace {void check(bool v){if(!v)throw std::domain_error("invalid legacy MPI PDF input");}}
struct LegacyMpiPdf::Impl {std::unique_ptr<LHAPDF::PDF> pdf;double q2min;
 Impl(const std::string& set,int member,std::optional<double> floor):pdf(LHAPDF::mkPDF(set,member)){
  double qmin=pdf->info().get_entry_as<double>("QMin",1.);q2min=floor.value_or(qmin*qmin);
  check(std::isfinite(q2min)&&q2min>=0);
 }};
LegacyMpiPdf::LegacyMpiPdf(const std::string& set,int member,std::optional<double> floor):impl_(std::make_unique<Impl>(set,member,floor)){}
LegacyMpiPdf::~LegacyMpiPdf()=default;
std::array<double,11> LegacyMpiPdf::evaluate(int beam,double x,double remaining,double q2,int nf,double charm,double bottom) const {
 check(beam==2212||beam==-2212||beam==2112||beam==-2112);
 check(std::isfinite(x)&&std::isfinite(remaining)&&remaining>0&&remaining<=1&&std::isfinite(q2)&&q2>=0);
 check(nf>=0&&nf<=6&&std::isfinite(charm)&&charm>=0&&std::isfinite(bottom)&&bottom>=0);
 std::array<double,11> out{};double scaled=x/remaining;
 if(scaled<=0||scaled>=1)return out;
 double q=std::sqrt(std::max({0.,impl_->q2min,q2}));
 auto value=[&](int id){return impl_->pdf->xfxQ(id,scaled,q);};
 double dsea=value(-1),usea=value(-2),dnv=value(1)-dsea,upv=value(2)-usea;
 out[4]=dsea;out[3]=usea;out[6]=dnv+dsea;out[7]=upv+usea;out[5]=value(21);
 for(int id=3;id<=5;++id){double v=impl_->pdf->hasFlavor(id)?value(id):0;out[5+id]=v;out[5-id]=v;}
 if(std::abs(beam)==2112){std::swap(out[6],out[7]);std::swap(out[4],out[3]);}
 if(beam<0)for(int id=1;id<=5;++id)std::swap(out[5+id],out[5-id]);
 for(int i=0;i<11;++i){check(std::isfinite(out[i]));out[i]=std::max(0.,out[i]);if(i!=5&&std::abs(i-5)>nf)out[i]=0;}
 // The source literal 1.1 is default REAL, promoted to double.
 if(q2<double(1.1f)*(charm*charm)){out[1]=0;out[9]=0;}
 if(q2<double(1.1f)*(bottom*bottom)){out[0]=0;out[10]=0;}
 return out;
}
}
