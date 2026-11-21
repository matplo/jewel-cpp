// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/pythia8_input.hpp"
#include <stdexcept>
// Shared parsing for hard-input and full development runners.
inline bool input_option(int argc,char** argv,int& i,jewel::PythiaInputConfig& config){
 std::string option=argv[i];
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
