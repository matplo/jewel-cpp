// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/core.hpp"
#include <array>
#include <functional>
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
class ExponentialIntegral {
 std::array<std::array<double,3>,1000> table_{};
public:
 ExponentialIntegral();
 const auto& table() const { return table_; }
 double intpqglow(double z,double q,const Kernels& kernels) const;
 double intpqghigh(double z,double q,const Kernels& kernels) const;
 double scatprimfunc(double t,double debye,const Kernels& kernels) const;
 double operator()(double x) const; // Stock interpolation, including below first knot.
};
}
