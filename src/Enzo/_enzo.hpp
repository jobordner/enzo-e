// See LICENSE_CELLO file for license and copyright information

/// @file     enzo.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2010-04-02
/// @brief    Include file for the \ref Enzo component

#ifndef ENZO_PRIVATE_HPP
#define ENZO_PRIVATE_HPP

//----------------------------------------------------------------------

#define OMEGA_TOLERANCE 1.0e-5

#ifdef CONFIG_PRECISION_SINGLE
#   define ETA_TOLERANCE 1.0e-5
#   define ENZO_HUGE_VAL HUGE_VALF
#   define type_enzo_float type_float
#endif
#ifdef CONFIG_PRECISION_DOUBLE
#   define ETA_TOLERANCE 1.0e-10
#   define ENZO_HUGE_VAL HUGE_VAL
#   define type_enzo_float type_double
#endif
#ifdef CONFIG_PRECISION_QUAD
#   define ETA_TOLERANCE 1.0e-20
#   define ENZO_HUGE_VAL HUGE_VALL
#endif

//----------------------------------------------------------------------

enum mass_type {
  mass_unknown,
  mass_dark,
  mass_baryon
};

//----------------------------------------------------------------------

enum {
  index_turbulence_vad,
  index_turbulence_aad,
  index_turbulence_vvdot,
  index_turbulence_vvot,
  index_turbulence_vvd,
  index_turbulence_vv,
  index_turbulence_dd,
  index_turbulence_d,
  index_turbulence_dax,
  index_turbulence_day,
  index_turbulence_daz,
  index_turbulence_dvx,
  index_turbulence_dvy,
  index_turbulence_dvz,
  index_turbulence_dlnd,
  index_turbulence_zones,
  index_turbulence_mind,
  index_turbulence_maxd,
  max_turbulence_array };

//----------------------------------------------------------------------

enum enzo_sync_id {
  enzo_sync_id_cg = sync_id_last,
  enzo_sync_id_comoving_expansion,
  enzo_sync_id_method_background_acceleration,
  enzo_sync_id_method_cosmology,
  enzo_sync_id_method_feedback,
#ifdef CONFIG_USE_GRACKLE
  enzo_sync_id_method_grackle,
#endif
  enzo_sync_id_method_gravity,
  enzo_sync_id_method_gravity_continue,
  enzo_sync_id_method_heat,
  enzo_sync_id_method_null,
  enzo_sync_id_method_pm_deposit,
  enzo_sync_id_method_pm_update,
  enzo_sync_id_method_ppm,
  enzo_sync_id_method_ppml,
  enzo_sync_id_method_star_maker,
  enzo_sync_id_method_turbulence,
  enzo_sync_id_method_vlct,
  enzo_sync_id_solver_bicgstab,
  enzo_sync_id_solver_bicgstab_precon_1,
  enzo_sync_id_solver_bicgstab_precon_2,
  enzo_sync_id_solver_bicgstab_loop_25,
  enzo_sync_id_solver_bicgstab_loop_85,
  enzo_sync_id_solver_cg_matvec,
  enzo_sync_id_solver_cg_loop_2,
  enzo_sync_id_solver_dd,
  enzo_sync_id_solver_dd_coarse,
  enzo_sync_id_solver_dd_domain,
  enzo_sync_id_solver_dd_smooth,
  enzo_sync_id_solver_mg0,
  enzo_sync_id_solver_mg0_coarse,
  enzo_sync_id_solver_mg0_last,
  enzo_sync_id_solver_mg0_post,
  enzo_sync_id_solver_mg0_pre,
  enzo_sync_id_solver_jacobi_1,
  enzo_sync_id_solver_jacobi_2,
  enzo_sync_id_solver_jacobi_3
};

//----------------------------------------------------------------------

// #include "macros_and_parameters.h"
#include "enzo_defines.hpp"
#include "enzo_typedefs.hpp"
#include "enzo_fortran.hpp"
#include "enzo_reductions.hpp"

//----------------------------------------------------------------------

enum return_enum {
  return_unknown,
  return_converged,
  return_diverged,
  return_error,
  return_bypass
};

//----------------------------------------------------------------------

const int field_undefined = -1;

//----------------------------------------------------------------------

struct enzo_fluxes
{
  long_int LeftFluxStartGlobalIndex [MAX_DIMENSION][MAX_DIMENSION];
  long_int LeftFluxEndGlobalIndex   [MAX_DIMENSION][MAX_DIMENSION];
  long_int RightFluxStartGlobalIndex[MAX_DIMENSION][MAX_DIMENSION];
  long_int RightFluxEndGlobalIndex  [MAX_DIMENSION][MAX_DIMENSION];
  enzo_float *LeftFluxes [MAX_NUMBER_OF_BARYON_FIELDS][MAX_DIMENSION];
  enzo_float *RightFluxes[MAX_NUMBER_OF_BARYON_FIELDS][MAX_DIMENSION];
};

//----------------------------------------------------------------------
// Cello include file
//----------------------------------------------------------------------

#include "cello.hpp"

//----------------------------------------------------------------------
// Component class includes
//----------------------------------------------------------------------

#ifdef CONFIG_USE_GRACKLE
#include <stdlib.h>
extern "C" {
  #include <grackle.h>
}
#endif

//----------------------------------------------------------------------

#include "fortran.h" /* included so scons knowns to install fortran.h */

#include "fortran_types.h" /* included so scons knowns to install fortran.h */

#include "enzo_constants.hpp"

#include "cosmology/EnzoPhysicsCosmology.hpp"

#include "enzo_EnzoDualEnergyConfig.hpp"
#include "enzo_EnzoFluidFloorConfig.hpp"
#include "enzo_EnzoPhysicsFluidProps.hpp"

#include "enzo_EnzoUnits.hpp"

#include "enzo_EnzoFactory.hpp"

#include "enzo_EnzoSimulation.hpp"

#include "enzo_EnzoProblem.hpp"

#include "enzo_EnzoConfig.hpp"

#include "enzo_EnzoBlock.hpp"

#include "enzo_IoEnzoBlock.hpp"
#include "enzo_IoEnzoReader.hpp"
#include "enzo_IoEnzoWriter.hpp"

#include "enzo_EnzoBoundary.hpp"

#include "initial/EnzoInitialBCenter.hpp"
#include "initial/EnzoInitialCloud.hpp"
#include "initial/EnzoInitialCollapse.hpp"
#include "initial/EnzoInitialCosmology.hpp"
#include "initial/EnzoInitialFeedbackTest.hpp"
#include "initial/EnzoInitialGrackleTest.hpp"
#include "initial/EnzoInitialHdf5.hpp"
#include "initial/EnzoInitialImplosion2.hpp"
#include "initial/EnzoInitialInclinedWave.hpp"
#include "initial/EnzoInitialMusic.hpp"
#include "initial/EnzoInitialPm.hpp"
#include "initial/EnzoInitialPpmlTest.hpp"
#include "initial/EnzoInitialSedovArray2.hpp"
#include "initial/EnzoInitialSedovArray3.hpp"
#include "initial/EnzoInitialSedovRandom.hpp"
#include "initial/EnzoInitialShockTube.hpp"
#include "initial/EnzoInitialSoup.hpp"
#include "initial/EnzoInitialTurbulence.hpp"
#include "initial/EnzoInitialIsolatedGalaxy.hpp"
#include "initial/EnzoInitialBurkertBodenheimer.hpp"
#include "initial/EnzoInitialMergeSinksTest.hpp"
#include "initial/EnzoInitialAccretionTest.hpp"
#include "initial/EnzoInitialShuCollapse.hpp"
#include "initial/EnzoInitialBBTest.hpp"

#include "amr/EnzoRefineShock.hpp"
#include "amr/EnzoRefineParticleMass.hpp"
#include "amr/EnzoRefineMass.hpp"

// [order dependencies:]
#include "feedback-particle/EnzoSinkParticle.hpp"
#include "feedback-particle/EnzoBondiHoyleSinkParticle.hpp"
#include "feedback-particle/EnzoFluxSinkParticle.hpp"

// [order dependencies:]
#include "enzo_EnzoEFltArrayMap.hpp"
#include "enzo_EnzoEquationOfState.hpp"
#include "enzo_EnzoEOSIdeal.hpp"

#include "enzo_EnzoCenteredFieldRegistry.hpp"
#include "enzo_EnzoFieldAdaptor.hpp"
#include "hydro-mhd/EnzoIntegrationQuanUpdate.hpp"
#include "hydro-mhd/EnzoLazyPassiveScalarFieldList.hpp"
#include "hydro-mhd/EnzoPermutedCoordinates.hpp"
#include "hydro-mhd/EnzoSourceGravity.hpp"
#include "hydro-mhd/EnzoSourceInternalEnergy.hpp"

#include "hydro-mhd/EnzoReconstructor.hpp"
#include "hydro-mhd/EnzoReconstructorNN.hpp"
#include "hydro-mhd/EnzoReconstructorPLM.hpp"

// public header for the EnzoRiemann sub-library. This needs to be included
// after the headers for:
//     EnzoEFltArrayMap, EnzoCenteredFieldRegistry, & EnzoEquationOfState
// but before the header for EnzoMethodMHDVlct EnzoBfieldMethod and EnzoBfieldMethodCT
#include "EnzoRiemann/EnzoRiemann.hpp"

// [order dependencies:]
#include "hydro-mhd/EnzoBfieldMethod.hpp"
#include "hydro-mhd/EnzoBfieldMethodCT.hpp"

#include "feedback-particle/EnzoMethodAccretion.hpp"
#include "enzo_EnzoMethodBackgroundAcceleration.hpp"
#include "feedback-particle/EnzoMethodBondiHoyleAccretion.hpp"
#include "enzo_EnzoMethodCheck.hpp"
#include "cosmology/EnzoMethodComovingExpansion.hpp"
#include "cosmology/EnzoMethodCosmology.hpp"
#include "feedback-particle/EnzoMethodDistributedFeedback.hpp"
#include "feedback-particle/EnzoMethodFeedback.hpp"
#include "feedback-particle/EnzoMethodFeedbackSTARSS.hpp"
#include "feedback-particle/EnzoMethodFluxAccretion.hpp"
#include "enzo_EnzoMethodGrackle.hpp"
#include "gravity/EnzoMethodGravity.hpp"
#include "enzo_EnzoMethodHeat.hpp"
#include "feedback-particle/EnzoMethodMergeSinks.hpp"
#include "enzo_EnzoMethodPmDeposit.hpp"
#include "enzo_EnzoMethodPmUpdate.hpp"
#include "feedback-particle/EnzoMethodSinkMaker.hpp"
#include "feedback-particle/EnzoMethodStarMaker.hpp"
#include "feedback-particle/EnzoMethodStarMakerSTARSS.hpp"
#include "feedback-particle/EnzoMethodStarMakerStochasticSF.hpp"
#include "feedback-particle/EnzoMethodThresholdAccretion.hpp"
#include "enzo_EnzoMethodTurbulence.hpp"
#include "hydro-mhd/EnzoMethodHydro.hpp"
#include "hydro-mhd/EnzoMethodMHDVlct.hpp"
#include "hydro-mhd/EnzoMethodPpm.hpp"
#include "hydro-mhd/EnzoMethodPpml.hpp"

#include "gravity/EnzoMatrixDiagonal.hpp"
#include "gravity/EnzoMatrixIdentity.hpp"
#include "gravity/EnzoMatrixLaplace.hpp"

#include "enzo_EnzoMsgCheck.hpp"

#include "enzo_EnzoComputeAcceleration.hpp"
#include "enzo_EnzoComputeCicInterp.hpp"
#include "enzo_EnzoComputePressure.hpp"
#include "enzo_EnzoComputeTemperature.hpp"
#ifdef CONFIG_USE_GRACKLE
  #include "enzo_EnzoComputeCoolingTime.hpp"
#endif

#include "gravity/EnzoSolverBiCgStab.hpp"
#include "gravity/EnzoSolverCg.hpp"
#include "gravity/EnzoSolverDd.hpp"
#include "gravity/EnzoSolverDiagonal.hpp"
#include "gravity/EnzoSolverJacobi.hpp"
#include "gravity/EnzoSolverMg0.hpp"

#include "enzo_EnzoStopping.hpp"

#include "amr/EnzoProlong.hpp"
#include "amr/EnzoProlongMC1.hpp"
#include "amr/EnzoProlongPoisson.hpp"
#include "amr/EnzoRestrict.hpp"

#endif /* ENZO_PRIVATE_HPP */
