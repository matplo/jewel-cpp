// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <optional>
#include <functional>
#include <array>
#include "jewel/core.hpp"
namespace jewel {
struct LegacyMpiOverlapState {
 double integral{},overlap_average{},overlap_normalization{},mean_impact{},
        dividing_impact{},k_factor{};
};
struct LegacyMpiImpact {
 double squared_impact{},enhancement{},relative_impact{};
};
// PYMULT double-Gaussian overlap initialization and hard-process sampling.
// The supplied integral is the saved initialization integral, not recomputed
// from the current production screening scale.
class LegacyMpiOverlap {
 double a_,b_,c_,inverse_core_,inverse_mixed_;
 LegacyMpiOverlapState state_;
 double overlap(double squared_impact,double normalization=1) const;
public:
 LegacyMpiOverlap(double core_fraction,double radius_ratio,
                  double integrated_cross_section,double sigma_nondiffractive);
 const LegacyMpiOverlapState& state() const {return state_;}
 LegacyMpiImpact sample_hard(Random&) const;
};

struct LegacyMpiScaleProposal {bool proposed{};double xt_squared{};};
// PYMULT mode 6, MSTP(82)>=2 ordering proposal. Kinematic/PDF acceptance follows
// separately; callers retry using the returned scale even after rejecting it.
class LegacyMpiScaleSampler {
 double screening_,factor_;
public:
 LegacyMpiScaleSampler(double screening_squared,double overlap_average,
   double overlap_enhancement,double cross_section_bound,double sigma_nondiffractive);
 double factor() const {return factor_;}
 LegacyMpiScaleProposal next(double upper_xt_squared,Random&) const;
};
struct LegacyMpiPhaseSpace {
 double tau{},rapidity{},cos_theta{},x1{},x2{};
 bool momentum_available(double remaining_a,double remaining_b) const;
};
// Massless ISUB=96 hadron/hadron phase-space proposal: coefficients 1,8,9.
// Uses five draws even if the resulting momentum fractions are unavailable.
LegacyMpiPhaseSpace sample_legacy_mpi_phase_space(double xt_squared,
 const std::array<double,3>& coefficients,Random&);
// PYSIGH ISTSB=9, screened (MSTP82>=2) common factor, before alpha_s,
// subprocess matrix elements and PDFs. Coefficients are 1,2,8,9,10.
double legacy_mpi_common_factor(double collision_s,double xt_squared,double tau,
 double rapidity,double screening_squared,const std::array<double,5>& coefficients,
 double gev2_to_mb,double k_factor);
struct LegacyMpiHardWeight {
 double bin_coordinate{};
 int bin{}; // Source's one-based bin, 1..20.
 double cross_section_above{},probability{1};
};
// PYMULT modes 4/5 numerical component only, not a complete MPI generator.
// Saved tables deliberately contain no live screening scale.
class LegacyMpiTable {
 std::array<double,20> sums_;
 std::array<int,20> counts_;
 double overlap_normalization_;
public:
 LegacyMpiTable(std::array<double,20> sums,std::array<int,20> counts,
                double overlap_normalization);
 LegacyMpiHardWeight hard_weight(double xt_squared,double screening_squared,
   double overlap_enhancement,double sigma_nondiffractive,
   bool sudakov_already_applied=false,double energy_rescaling=1) const;
};
struct LegacyMpiPair {
 double flavour_draw{},pt{},phi{};
 int pdg_a{},pdg_b{};
 Momentum a,b;
};
// PYMULT mode-6 accept/reject followed by its phenomenological pair choice.
// The channel sum is used only for acceptance, not weighted flavour sampling.
// Returns no pair after one draw on rejection; accepted pairs use 3 or 4 draws.
// Colour attachment, remnant updates and showering are separate operations.
std::optional<LegacyMpiPair> sample_legacy_mpi_pair(double cross_section,
 double bound,double ecm,double xt_squared,double x1,double x2,double cos_theta,
 double attach_probability,double gluon_probability,double strange_weight,
 double two_pi,Random&);

struct LegacyMpiStringPiece {std::size_t first{},second{};};
// Ordered string-piece topology after adding the final two momenta as an MPI
// pair. Indices refer to the supplied momentum vector; no packed PYJETS links.
// Pythia8 colour tags must be assigned by the event adapter separately.
std::vector<LegacyMpiStringPiece> legacy_mpi_attach_strings(
 const std::vector<Momentum>&,std::vector<LegacyMpiStringPiece>,
 double flavour_draw,double attach_probability,int first_pair_pdg);

struct LegacyMpiEvolutionConfig {
 double ecm{},screening{},overlap_average{},enhancement{},bound{},sigma_nd{};
 std::array<double,3> phase_coefficients{};
 double attach_probability{},gluon_probability{},strange_weight{},two_pi{};
};
struct LegacyMpiEvolutionState {
 double upper_xt{},remaining_a{},remaining_b{},used_a{},used_b{},sum_pt{},first_pt{};
 unsigned interactions{1},additional_pairs{};
 bool stopped{};
 std::vector<Momentum> momenta;
 std::vector<int> pdgs;
 std::vector<LegacyMpiStringPiece> strings;
};
enum class LegacyMpiStep { Stopped=1, MomentumRejected=2, CrossSectionRejected=3, Accepted=4 };
// Stateful screened PYMULT mode-6 iteration, without optional MPI showers.
// Cross-section callback receives the newly proposed scale/phase and current
// beam state; it is called only after the momentum-reserve check succeeds.
class LegacyMpiEvolution {
 LegacyMpiEvolutionConfig config_;
 LegacyMpiEvolutionState state_;
 LegacyMpiScaleSampler scales_;
public:
 using CrossSection=std::function<double(double,const LegacyMpiPhaseSpace&,const LegacyMpiEvolutionState&)>;
 LegacyMpiEvolution(LegacyMpiEvolutionConfig,LegacyMpiEvolutionState);
 const LegacyMpiEvolutionState& state() const {return state_;}
 LegacyMpiStep step(Random&,const CrossSection&);
};

// Convert a complete directed outgoing string graph to unique colour tags.
// Types follow Pythia8: 0 singlet, +1 triplet, -1 antitriplet, 2 octet.
// Junctions, sextets and incomplete/history graphs are not supported here.
std::vector<std::array<int,2>> legacy_mpi_colour_tags(
 const std::vector<LegacyMpiStringPiece>&,const std::vector<int>& colour_types,
 int first_tag=101);

struct LegacyMpiIntegralTable {
 std::array<double,20> sums{};
 std::array<int,20> counts{};
 double integral{};
};
// One PYMULT initialization pass, including all twenty stratified xT bins.
// Exactly six draws per point; no production momentum-reserve rejection.
LegacyMpiIntegralTable integrate_legacy_mpi(double screening_squared,int samples_per_bin,
 const std::array<double,3>& phase_coefficients,Random&,
 const std::function<double(double,const LegacyMpiPhaseSpace&)>& cross_section);

struct LegacyMpiScreeningConfig {
 double ecm{},pt0_reference{},reference_energy{},energy_power{},sigma_nd{};
 int samples_per_bin{};
 std::array<double,3> phase_coefficients{};
 unsigned max_passes{100}; // Explicit failure guard; never accepts a failed pass.
};
struct LegacyMpiInitializationPass {
 double pt0_reference{},screening_squared{},scale_shift{};
 LegacyMpiIntegralTable table;
};
using LegacyMpiIntegralEvaluator=std::function<double(double,const LegacyMpiPhaseSpace&)>;
// Factory is rebuilt for each screening value, so PDFs/scales and proposal
// normalization see the same updated parameters. Random draws continue across retries.
std::vector<LegacyMpiInitializationPass> initialize_legacy_mpi(
 const LegacyMpiScreeningConfig&,Random&,
 const std::function<LegacyMpiIntegralEvaluator(double,double,double)>& factory);

using LegacyMpiEvaluatorFactory =
 std::function<LegacyMpiIntegralEvaluator(double,double,double)>;
struct LegacyMpiInitialization {
 std::vector<LegacyMpiInitializationPass> legacy;
 std::vector<LegacyMpiInitializationPass> secondary;
 LegacyMpiOverlap overlap;
 LegacyMpiTable hard_table;
 // Production uses secondary.back() for screening and the legacy table
 // and overlap for the hard weight and scale proposal normalization.
};
LegacyMpiInitialization initialize_legacy_mpi_lifecycle(
 LegacyMpiScreeningConfig,double core_fraction,double radius_ratio,Random&,
 const LegacyMpiEvaluatorFactory& legacy_factory,
 const LegacyMpiEvaluatorFactory& secondary_factory);

}
