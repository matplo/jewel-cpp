// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
#include <array>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
namespace jewel {
// JEWEL's scalar quadrature; derivative depends on x, not on y.
double integrate(const std::function<double(double)>& f,double a,double b,double eps,double first_step);
struct MediumConfig {
 int mass_number{208}, nf{3};
 double tau_initial{0.4}, temperature_initial{0.59}, temperature_cutoff{0.17};
 double nuclear_density{0.17}, skin_depth{0.54}, sigma_nn{double(6.2f)};
 double impact_parameter{0}, max_rapidity{5}, debye_floor{0.45}, debye_scale{1};
 bool woods_saxon{true};
};
struct MediumBounds {
 double temperature_max{},debye_max{},debye_min{},scatterer_mass_max{};
 double density_at_debye_min{},lifetime_max{},density_max{};
};
struct GeometryEvent { double impact_parameter{}, centrality_fraction{}; };
struct Scatterer { int type{}; Momentum momentum; double mass{}; bool synthetic{}; };
class GeometrySampler {
 std::array<std::array<double,3>,200> cross_;
public:
 explicit GeometrySampler(std::array<std::array<double,3>,200> cross);
 GeometryEvent sample(double centrality_min_percent,double centrality_max_percent,Random&) const;
};
class ExpandingMedium final : public Medium {
 MediumConfig config_;
 std::array<std::array<double,2>,100> thickness_{};
public:
 explicit ExpandingMedium(MediumConfig config = {});
 const auto& thickness_table() const { return thickness_; }
 std::array<std::array<double,3>,200> cross_section_table() const;
 double thickness(double x,double y) const;
 double participants(double x1,double y1,double x2,double y2) const;
 ExpandingMedium with_impact_parameter(double b) const;
 SpaceTime sample_vertex(Random&,double sampling_radius=10) const;
 Scatterer sample_scatterer(SpaceTime,Random&,bool longitudinal_boost=true) const;
 MediumBounds bounds() const;
 Scatterer average_scatterer(SpaceTime,bool longitudinal_boost=true) const;
 Scatterer maximum_scatterer(bool longitudinal_boost=true) const;
 double temperature(SpaceTime position) const;
 MediumSample sample(SpaceTime position) const override;
};
// Value-owned environment. Empty optional selects the frozen medium-vac.f
// contract; an expanding environment retains the existing implementation.
class MediumModel final : public Medium {
 std::optional<ExpandingMedium> expanding_;
public:
 MediumModel() = default;
 MediumModel(ExpandingMedium medium):expanding_(std::move(medium)){}
 bool is_vacuum() const {return !expanding_;}
 MediumSample sample(SpaceTime p) const override {return expanding_?expanding_->sample(p):MediumSample{};}
 double temperature(SpaceTime p) const {return sample(p).temperature;}
 MediumBounds bounds() const {return expanding_?expanding_->bounds():MediumBounds{};}
 Scatterer maximum_scatterer(bool boost=true) const {
  return expanding_?expanding_->maximum_scatterer(boost):Scatterer{0,{0,0,0,.5},.5,false};
 }
 SpaceTime sample_vertex(Random& r,double radius=10) const {return expanding_?expanding_->sample_vertex(r,radius):SpaceTime{};}
 MediumModel with_impact_parameter(double b) const {return expanding_?MediumModel(expanding_->with_impact_parameter(b)):MediumModel{};}
 const ExpandingMedium& expanding() const {
  if(!expanding_)throw std::domain_error("operation requires expanding medium");return *expanding_;
 }
 const auto& thickness_table() const {return expanding().thickness_table();}
 auto cross_section_table() const {return expanding().cross_section_table();}
 double thickness(double x,double y) const {return expanding().thickness(x,y);}
 double participants(double a,double b,double c,double d) const {return expanding().participants(a,b,c,d);}
 Scatterer average_scatterer(SpaceTime p,bool boost=true) const {return expanding().average_scatterer(p,boost);}
 Scatterer sample_scatterer(SpaceTime p,Random& r,bool boost=true) const {return expanding().sample_scatterer(p,r,boost);}
};
class ExponentialIntegral {
 std::array<std::array<double,3>,1000> table_{};
public:
 ExponentialIntegral();
 const auto& table() const { return table_; }
 double intpqglow(double z,double q,const Kernels& kernels) const;
 double intpqghigh(double z,double q,const Kernels& kernels) const;
 double scatprimfunc(double t,double debye,const Kernels& kernels) const;
 double extrapolated(double x) const; // Stock linear tail extrapolation; not exact Ei.
 double operator()(double x) const; // Stock interpolation, including below first knot.
};
}
