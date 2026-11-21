// SPDX-License-Identifier: GPL-2.0-or-later
// Literal MAKESTRINGS_MINL/CLEANUP, including physical-slot metadata behavior.
#include "jewel/strings.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
namespace jewel {
namespace {
bool active(int s){return s==1||s==3||s==4||s==5;}
// Pythia6 PYP(i,17): true rapidity reconstructed from stored mass, not E.
double rapidity(const CompressionRecord& r){
 double pr=std::max(1e-20,r.p[4]*r.p[4]+r.p[0]*r.p[0]+r.p[1]*r.p[1]);
 return std::copysign(std::log(std::min((std::sqrt(pr+r.p[2]*r.p[2])+std::abs(r.p[2]))/std::sqrt(pr),1e20)),r.p[2]);
}
}
StringBuildResult make_minimum_strings(std::vector<CompressionRecord>& records,std::size_t count,
 bool compress,double eta,const BranchingSampler& sampler,Random& random){
 if(count>records.size()||count>22990||!std::isfinite(eta)||eta<0)throw std::domain_error("invalid minimum-string input");
 std::vector<std::size_t> sources(count);std::iota(sources.begin(),sources.end(),0);
 if(compress&&count){auto result=compress_records(records,count,count);count=result.active;sources=result.source_slots;}
 records.resize(23000);sources.resize(23000,std::numeric_limits<std::size_t>::max());
 auto r=[&](int i)->CompressionRecord&{if(i<1||i>23000)throw std::domain_error("minimum-string reference outside record");return records[i-1];};
 int n=static_cast<int>(count),old=n;StringBuildResult result;
 auto fail=[&]{result.active=n;result.discarded=true;return result;};
 auto copy=[&](int from,int status){auto& to=r(n);auto& in=r(from);
  to.k={status,in.k[1],from,0,0};to.p=in.p;sources[n-1]=sources[from-1];
  in.k[0]=16;in.k[3]=in.k[4]=n;
 };
 for(int i=1;i<=n;++i)if(r(i).k[0]<=4&&r(i).colour==0&&r(i).anticolour==0)r(i).k[0]=19;
 for(;;){
  double emax=0;int most=0;
  for(int i=1;i<=old;++i){auto& v=r(i);
   if(v.k[0]>=11&&v.k[0]<=14)v.k[0]=17;
   if(std::abs(rapidity(v))>eta+1)v.k[0]=17;
   if(active(v.k[0])&&v.p[3]>emax){emax=v.p[3];most=i;}
  }
  if(!most)break;
  if(r(most).k[1]==21){
   n+=2;if(n>22990)return fail();
   if(n-2>old)for(int j=old;j<=n-3;++j){
    int to=n+old-j,from=to-2;auto& dest=r(to);auto& src=r(from);
    dest.k={src.k[0],src.k[1],src.k[2]>old?src.k[2]+2:src.k[2],0,0};dest.p=src.p;
    sources[to-1]=sources[from-1];
    if(src.k[2]>0){r(src.k[2]).k[3]+=2;r(src.k[2]).k[4]+=2;}
   }
   old+=2;r(most).k[0]=18;
   double z=sampler.fraction(0,0,1e-3,Channel::QG,random);int pdg=1;
   if(z<=.5){z=1-z;pdg=-1;}
   r(old-1).k={1,pdg,most,0,0};r(old).k={1,-pdg,most,0,0};
   for(int j=0;j<4;++j){r(old-1).p[j]=(1-z)*r(most).p[j];r(old).p[j]=z*r(most).p[j];}
   r(old-1).p[4]=r(old).p[4]=r(most).p[4];
   sources[old-2]=sources[old-1]=sources[most-1];
   r(most).k[3]=old-1;r(most).k[4]=old;most=old;
  }
  ++n;if(n>22990)return fail();copy(most,2);int end=most;
  for(;;){
   double minimum=1e10;int closest=0;
   for(int i=1;i<=old;++i){auto& v=r(i);
    if(active(v.k[0])&&(v.k[1]==21||(static_cast<double>(v.k[1])*r(end).k[1]<0&&v.k[2]!=r(end).k[2]))
       &&std::abs(rapidity(v)-rapidity(r(end)))<4){
     double mass=v.p[3]*r(most).p[3]-v.p[0]*r(most).p[0]-v.p[1]*r(most).p[1]-v.p[2]*r(most).p[2];
     if(mass<minimum&&mass>0&&mass<1e8){minimum=mass;closest=i;}
    }
   }
   ++n;if(n>22990)return fail();
   if(!closest){
    double u=random.uniform();if(!std::isfinite(u)||u<=0||u>=1)throw std::domain_error("invalid endpoint draw");
    r(n).k={1,-r(end).k[1],0,0,0};r(n).p={0,0,u<.5?10.:-10.,10,0};
    sources[n-1]=std::numeric_limits<std::size_t>::max();break;
   }
   bool gluon=r(closest).k[1]==21;copy(closest,gluon?2:1);
   if(!gluon)break;most=closest;
  }
 }
 for(int i=1;i<=old;++i)if(r(i).k[0]==19&&std::abs(rapidity(r(i)))<eta+1){
  ++n;if(n>22990)return fail();
  auto& dest=r(n);auto& src=r(i);dest.k={4,src.k[1],i,0,0};dest.p=src.p;
  src.k[0]=17;src.k[3]=src.k[4]=n;dest.colour=src.colour;dest.anticolour=src.anticolour;
  sources[n-1]=sources[i-1];
 }
 for(int i=1;i<=n-old;++i){r(i).k=r(old+i).k;r(i).p=r(old+i).p;r(i).v=r(old+i).v;r(i).k[2]=0;result.output_sources.push_back(sources[old+i-1]);}
 result.active=n-old;return result;
}
}
