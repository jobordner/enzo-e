// See LICENSE_CELLO file for license and copyright information

/// @file     control_compute.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2011-09-01
/// @brief    Functions implementing CHARM++ compute-related functions
/// @ingroup  Control

#include "simulation.hpp"
#include "mesh.hpp"
#include "control.hpp"

#include "charm_simulation.hpp"
#include "charm_mesh.hpp"

// #define DEBUG_COMPUTE

#define CYCLE 0

//======================================================================

void Block::compute_enter_ ()
{
  performance_start_(perf_compute,__FILE__,__LINE__);
  compute_begin_();
  performance_stop_(perf_compute,__FILE__,__LINE__);
}

//----------------------------------------------------------------------

void Block::compute_begin_ ()
{

  cello::simulation()->set_phase(phase_compute);

  // Update old fields

  if (state()->is_active(level()) )
    data()->field().save_history(state()->time(level()));

  index_method_ = 0;
  compute_next_();
}

//----------------------------------------------------------------------

void Block::compute_next_ ()
{
  Method * method = this->method();

  // refresh only scheduled methods [HANGS]
  // const bool is_scheduled = method && method->is_scheduled(this);

  const bool is_scheduled = true;

  if (method) {

    if (is_scheduled) {

#ifdef DEBUG_COMPUTE
      CkPrintf ("DEBUG_REFRESH %s:%d calling refresh_[enter|start]\n",__FILE__,__LINE__);
#endif

      int ir_post = method->refresh_id_post();

      Refresh * refresh = cello::refresh(ir_post);

      refresh -> set_adaptive_timestep (true);
      refresh -> set_level_lower(state()->level_lower());
      refresh -> set_level_upper(state()->level_upper());
      refresh->set_active (is_leaf());

      refresh_start (ir_post,CkIndex_Block::p_compute_continue());

    } else {

      compute_continue_();

    }

  } else {

    compute_end_();

  }
}

//----------------------------------------------------------------------

void Block::compute_continue_ ()
{
  performance_start_(perf_compute,__FILE__,__LINE__);
#ifdef DEBUG_COMPUTE
  if (state()->cycle() >= CYCLE)
    CkPrintf ("%d %s DEBUG_COMPUTE Block::compute_continue_()\n", CkMyPe(),name().c_str());
#endif

#ifdef CONFIG_USE_PROJECTIONS
  //  double time_start = CmiWallTimer();
#endif

  Method * method = this->method();

  const bool is_scheduled = method->is_scheduled(this);
  const bool is_active_level = state()->is_active(level());

  if (is_scheduled && is_active_level) {

    TRACE2 ("Block::compute_continue() method = %d %p\n",
	    index_method_,method); fflush(stdout);

#ifdef DEBUG_COMPUTE
    if (state()->cycle() >= CYCLE)
      CkPrintf ("%d %s DEBUG_COMPUTE applying Method %s\n",
                CkMyPe(),name().c_str(),method->name().c_str());
    CkPrintf ("DEBUG_TRACE_REFRESH Method %s compute()\n",method->name().c_str());
#endif

    method->compute (this);

  } else {

    compute_done();

  }
  performance_stop_(perf_compute,__FILE__,__LINE__);
}

//----------------------------------------------------------------------

void Block::compute_done ()
{
#ifdef DEBUG_COMPUTE
  if (state()->cycle() >= CYCLE)
    CkPrintf ("%d %s DEBUG_COMPUTE Block::compute_done_()\n", CkMyPe(),name().c_str());
#endif
  compute_update_method_state_(index_method_);
  index_method_++;
  compute_next_();
}

//----------------------------------------------------------------------

void Block::compute_update_method_state_(int index_method)
{
  auto & method_state = state()->method(index_method);

  method_state.advance();
//  if (index_method_ < state()->num_methods()) {
//    // Advance method state if any methods super-cycling
//    auto & method_state = state()->method(index_method);
//
//    method_state.advance();
//  }
}

//----------------------------------------------------------------------

void Block::compute_end_ ()
{
#ifdef DEBUG_COMPUTE
  if (state()->cycle() >= CYCLE)
    CkPrintf ("%d %s DEBUG_COMPUTE Block::compute_end_()\n", CkMyPe(),name().c_str());
#endif

  // Save active level range for updating simulation state before advance() changes it

  const int level_lower = state()->level_lower();
  const int level_upper = state()->level_upper();

  // Update block cycle and time

  state()->advance();

  // delete fluxes
  data()->flux_data()->deallocate();

  auto & state_global = cello::simulation()->state();

  // update simulation global state
  state_global->set_cycle(state()->cycle());
  state_global->set_time (state()->time());

  if ( (state()->state_type() == State::Type::Level) &&
       (state()->is_active(level())) ) {

    // update simulation level states
    for (int level=level_lower; level < level_upper; level++) {
      state_global->set_cycle(state()->cycle(level),level);
      state_global->set_time (state()->time (level),level);
    }

  }

  compute_exit_();

  TRACE ("END   PHASE COMPUTE");
}

//----------------------------------------------------------------------



