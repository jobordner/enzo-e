// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSolverEnzo.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2018-10-01
/// @brief    Implements the EnzoSolverEnzo class
///
/// @brief [\ref Enzo] Multigrid on the root-level grid using Enzo, then
/// BiCgStab in overlapping subdomains defined by root-level Blocks.
/// An optional final Jacobi step can be applied to smooth the solution
/// along subdomain boundaries.
///
/// 0. restrict b to level 0
/// 1. coarse solve: solve Ax=b on level 0
/// 2. Prolong x to child blocks as xc
/// 3. domain solve: solve Ai xi = bi in each root-grid block
///        use xc for boundary conditions and initial guess
/// 4. final smoother: apply final smoother on A x = b (if any)
///
///

#include "Cello/cello.hpp"
#include "Enzo/enzo.hpp"
#include "Enzo/gravity/gravity.hpp"

//======================================================================

EnzoSolverEnzo::EnzoSolverEnzo
  (std::string name,
   std::string field_x,
   std::string field_b,
   int monitor_iter,
   int restart_cycle,
   int solve_type,
   int index_prolong,
   int index_restrict,
   int min_level,
   int max_level)

    : Solver(name,
	     field_x,
	     field_b,
	     monitor_iter,
	     restart_cycle,
	     solve_type,
             index_prolong,
             index_restrict,
	     min_level,
	     max_level)
{

  Refresh * refresh = cello::refresh(ir_post_);
  cello::simulation()->refresh_set_name(ir_post_,name);

  refresh->add_field (ix_);

}

//----------------------------------------------------------------------

void EnzoSolverEnzo::apply ( std::shared_ptr<Matrix> A, Block * block) throw()
{
  Solver::begin_(block);

  CkCallback callback(CkIndex_EnzoBlock::r_solver_enzo_barrier(nullptr),
		      enzo::block_array());
  block->contribute(callback);
}

//----------------------------------------------------------------------

void EnzoBlock::r_solver_enzo_barrier(CkReductionMsg * msg)
{
  delete msg;
  static_cast<EnzoSolverEnzo*> (solver())->end(this);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::end (Block* block) throw ()
{
  Solver::end_(block);
}
