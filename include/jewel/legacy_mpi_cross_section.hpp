// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "jewel/legacy_mpi.hpp"
#include "jewel/legacy_mpi_pdf.hpp"
#include "jewel/legacy_mpi_channels.hpp"
namespace jewel {
enum class LegacyMpiPdfScale { Screened, SecondaryInitialization };
struct LegacyMpiCrossSectionConfig {
 LegacyMpiPdfScale pdf_scale{LegacyMpiPdfScale::Screened};
 double ecm{},screening_squared{},scale_shift{},renormalization_multiplier{1};
 // Initialization may supply a separate PYSIGH pT cut; production derives it.
 std::optional<double> kinematic_pt_cut;
 std::array<double,5> coefficients{};
 double gev2_to_mb{},k_factor{1},asymmetric_factor{1},width_fudge{1};
 int beam_a{2212},beam_b{2212},max_flavour{5},interference{};
 std::array<double,5> masses{};
 std::array<bool,5> enabled{};
 std::vector<LegacyGluonDecay> decays;
};
struct LegacyMpiCrossSectionResult {
 double s{},t{},u{},q2{},alpha_s{},common_factor{},annihilation_weight{};
 std::array<double,11> pdf_a{},pdf_b{};
 LegacyMpiChannelResult channels;
};
// Non-diffractive, massless ISUB=96, screened legacy MPI. Coupling is supplied
// by an explicit policy (e.g. Pythia8 AlphaStrong); no reference values required.
class LegacyMpiCrossSection {
 LegacyMpiCrossSectionConfig config_;
 LegacyMpiPdf pdf_;
 std::function<double(double)> alpha_s_;
public:
 LegacyMpiCrossSection(LegacyMpiCrossSectionConfig,const std::string& pdf_set,
  int member,std::optional<double> q2_floor,std::function<double(double)> alpha_s);
 LegacyMpiCrossSectionResult evaluate(double xt_squared,const LegacyMpiPhaseSpace&,
  double remaining_a,double remaining_b) const;
 // Directly usable as LegacyMpiEvolution's cross-section callback.
 double operator()(double xt_squared,const LegacyMpiPhaseSpace& phase,
  const LegacyMpiEvolutionState& state) const {
  return evaluate(xt_squared,phase,state.remaining_a,state.remaining_b).channels.total;
 }
};
// Initialize both screened MPI stages with native PDFs and channel weights.
LegacyMpiInitialization initialize_legacy_mpi_with_pdfs(
 const LegacyMpiScreeningConfig&,double core_fraction,double radius_ratio,Random&,
 LegacyMpiCrossSectionConfig,const std::string& pdf_set,int member,
 std::optional<double> q2_floor,std::function<double(double)> alpha_s);

}
