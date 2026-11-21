// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <memory>
#include <optional>
#include <string>
namespace jewel {
// LHAPDF-backed legacy nucleon PDF policy, not the new-MPI companion model.
class LegacyMpiPdf {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 // Default: PDF metadata QMin squared. Override only for an audited reference
 // convention (some legacy shared-library builds expose a zero common floor).
 LegacyMpiPdf(const std::string& set,int member,std::optional<double> q2_floor=std::nullopt);
 ~LegacyMpiPdf();
 LegacyMpiPdf(const LegacyMpiPdf&)=delete;
 LegacyMpiPdf& operator=(const LegacyMpiPdf&)=delete;
 // Output order: antiquarks -5..-1, gluon, quarks 1..5, as x*f.
 std::array<double,11> evaluate(int beam,double x,double remaining,double q2,
   int max_flavour,double charm_mass,double bottom_mass) const;
};
}
