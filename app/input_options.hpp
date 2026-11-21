// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include <stdexcept>
#include <cmath>
#include <set>
struct InputOptionState { std::set<std::string> hard_range_seen; };
// Shared parsing for hard-input and full development runners.
inline bool input_option(int argc,char** argv,int& i,jewel::PythiaInputConfig& config,InputOptionState& state){
 std::string option=argv[i];
 if(option=="--ecm"||option=="--pt-min"||option=="--pt-max"){
  if(!state.hard_range_seen.insert(option).second)throw std::invalid_argument("repeated "+option);
  if(i+1>=argc)throw std::invalid_argument("missing value for "+option);
  std::string token=argv[++i];std::size_t used=0;double value=std::stod(token,&used);
  if(used!=token.size()||!std::isfinite(value)||value<=0)throw std::invalid_argument("hard energy and cuts must be finite and positive");
  if(option=="--ecm")config.ecm=value;else if(option=="--pt-min")config.pt_min=value;else config.pt_max=value;
  return true;
 }
 if(option!="--lhapdf"&&option!="--hard-alpha-s"&&option!="--hard-alpha-s-order")return false;
 if(i+1>=argc)throw std::invalid_argument("missing value for "+option);
 std::string value=argv[++i];if(value.empty())throw std::invalid_argument("empty input option");std::size_t used=0;
 if(option=="--lhapdf"){
  if(!config.external_pdf.empty())throw std::invalid_argument("repeated external PDF");config.external_pdf=value;
 }else if(option=="--hard-alpha-s"){
  if(config.hard_alpha_s)throw std::invalid_argument("repeated hard alpha_s");
  config.hard_alpha_s=std::stod(value,&used);if(used!=value.size())throw std::invalid_argument("invalid hard alpha_s");
 }else{
  if(config.hard_alpha_s_order)throw std::invalid_argument("repeated hard alpha_s order");
  config.hard_alpha_s_order=std::stoi(value,&used);if(used!=value.size())throw std::invalid_argument("invalid hard alpha_s order");
 }
 return true;
}
