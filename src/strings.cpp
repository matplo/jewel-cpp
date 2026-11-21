// SPDX-License-Identifier: GPL-2.0-or-later
// MAKESTRINGS_VAC and CLEANUP, JEWEL 2.6.0.
#include "jewel/strings.hpp"
#include <numeric>
#include <stdexcept>
namespace jewel {
namespace {
bool active(int status){return status==1||status==3||status==4||status==5;}
bool diquark(int pdg){return std::abs(pdg)>=1000&&(pdg/10)%10==0;}
}
StringBuildResult make_vacuum_strings(std::vector<CompressionRecord>& records,std::size_t n,bool compress){
 if(n>records.size()||n>22990)throw std::domain_error("invalid string input size");
 std::vector<std::size_t> sources(n);std::iota(sources.begin(),sources.end(),0);
 if(compress){auto result=compress_records(records,n,n);n=result.active;sources=result.source_slots;}
 records.resize(23000);sources.resize(23000);
 const auto first=n;StringBuildResult result;
 auto finish_error=[&]{result.active=n;result.discarded=true;return result;};
 // Source increments N before checking its capacity.
 auto copy=[&](std::size_t from,int status){
  auto to=n++;if(n>22990)return false;
  auto& out=records[to];auto& in=records[from];
  out.k={status,in.k[1],static_cast<int>(from+1),0,0};out.p=in.p;
  out.colour=in.colour;out.anticolour=in.anticolour;sources[to]=sources[from];
  in.k[3]=in.k[4]=static_cast<int>(to+1);return true;
 };
 for(std::size_t i=0;i<first;++i)if(records[i].k[0]==4&&records[i].colour==0&&records[i].anticolour==0){
  if(!copy(i,11))return finish_error();records[i].k[0]=17;
 }
 const auto old=n;
 for(;;){
  std::size_t endpoint=old;
  for(std::size_t i=0;i<old;++i){auto& r=records[i];
   if(r.k[0]>=11&&r.k[0]<=14)r.k[0]=17;
   if(active(r.k[0])&&(r.k[1]<6||diquark(r.k[1]))){endpoint=i;break;}
  }
  if(endpoint==old)break;
  if(!copy(endpoint,2))return finish_error();records[endpoint].k[0]=16;
  bool ended=false;
  for(std::size_t step=0;step<10000000;++step){
   std::size_t match=old;const auto last=records[n-1];
   for(std::size_t i=0;i<old;++i){const auto& r=records[i];
    if(active(r.k[0])&&((r.colour!=0&&r.colour==last.anticolour)||(r.anticolour!=0&&r.anticolour==last.colour))){match=i;break;}
   }
   if(match==old){
    // PEVREC's diagnostic side effect is V=MV, including the mass/time column.
    for(std::size_t i=0;i<n;++i)records[i].v=records[i].production;
    return finish_error();
   }
   bool gluon=records[match].k[1]==21;
   if(!copy(match,gluon?2:1))return finish_error();records[match].k[0]=16;
   if(!gluon){ended=true;break;}
  }
  if(!ended)throw std::runtime_error("string traversal limit reached");
 }
 // The source's inner status check refers to I, not J. Retain that literal
 // behavior: even an inactive colour partner prevents its loose-gluon rejection.
 for(std::size_t i=0;i<old;++i)if(active(records[i].k[0])){
  bool matched=false;for(std::size_t j=0;j<old;++j)if(records[i].anticolour==records[j].colour){matched=true;break;}
  if(!matched)return finish_error();
 }
 for(std::size_t i=0;i<old;++i)if(active(records[i].k[0]))result.unconsumed_sources.push_back(sources[i]);
 const auto last=n;n=last-first;
 for(std::size_t i=0;i<n;++i){
  auto from=first+i;result.output_sources.push_back(sources[from]);
  records[i].k=records[from].k;records[i].p=records[from].p;records[i].v=records[from].v;records[i].k[2]=0;
 }
 result.active=n;return result;
}
}
