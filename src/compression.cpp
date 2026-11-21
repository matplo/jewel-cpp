// SPDX-License-Identifier: GPL-2.0-or-later
// COMPRESSEVENT boundary-record port, JEWEL 2.6.0.
#include "jewel/compression.hpp"
#include <stdexcept>
namespace jewel {
CompressionResult compress_records(std::vector<CompressionRecord>& records,std::size_t count,std::size_t selected){
 if(count==0||records.size()>23000||count>records.size()||selected>count)throw std::domain_error("invalid compression record bounds");
 auto disposable=[](int status){return status>=11&&status<=14;};
 CompressionResult result{count,selected,0,false,{}};result.source_slots.reserve(count);
 std::size_t first=1;
 while(first<count&&(!disposable(records[first].k[0])||first==selected))++first;
 for(std::size_t i=0;i<first;++i)result.source_slots.push_back(i);
 if(first==count)return result;
 auto destination=first;
 for(auto source=first;source<count;++source){
  if(disposable(records[source].k[0])&&source!=selected)continue;
  const auto time=records[destination].evolution_time;
  const auto induced=records[destination].medium_induced;
  records[destination]=records[source];
  auto& moved=records[destination];
  moved.evolution_time=time;moved.medium_induced=induced;
  moved.k[2]=moved.k[3]=moved.k[4]=0;
  if(source==selected)result.selected=destination;
  result.source_slots.push_back(source);++destination;
 }
 result.active=destination;result.removed=count-destination;result.room_left=result.removed>10;
 for(auto i=destination;i<count;++i){
  auto time=records[i].evolution_time;auto induced=records[i].medium_induced;
  auto angular=records[i].angular_required;
  records[i]={};records[i].evolution_time=time;records[i].medium_induced=induced;
  // COMPRESSEVENT clears QQBARD but does not clear AOREQ in inactive slots.
  records[i].angular_required=angular;
 }
 return result;
}
}
