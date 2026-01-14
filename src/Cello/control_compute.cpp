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

#define CYCLE 125

//======================================================================

void Block::compute_enter_ ()
{
  compute_begin_();
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

  if (method) {

    //  const bool is_scheduled = method->is_scheduled(this); // HANGS
    const bool is_scheduled = true;

    if (is_scheduled) {

#ifdef DEBUG_COMPUTE
      CkPrintf ("DEBUG_REFRESH %s:%d calling refresh_[enter|start]\n",__FILE__,__LINE__);
#endif

      int ir_post = method->refresh_id_post();

      Refresh * refresh = cello::refresh(ir_post);

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
#ifdef DEBUG_COMPUTE
  if (state()->cycle() >= CYCLE)
    CkPrintf ("%d %s DEBUG_COMPUTE Block::compute_continue_()\n", CkMyPe(),name().c_str());
#endif

#ifdef CONFIG_USE_PROJECTIONS
  //  double time_start = CmiWallTimer();
#endif

  Method * method = this->method();

  PERF_METHOD_START(method);
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
}

//----------------------------------------------------------------------

void Block::compute_done ()
{
#ifdef DEBUG_COMPUTE
  if (state()->cycle() >= CYCLE)
    CkPrintf ("%d %s DEBUG_COMPUTE Block::compute_done_()\n", CkMyPe(),name().c_str());
#endif

  PERF_METHOD_STOP(method());
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

  // Save level range for global state update later

  level_lower_ = state()->level_lower();
  level_upper_ = state()->level_upper();

  // Update block cycle and time

  state()->advance();

  // delete fluxes
  data()->flux_data()->deallocate();

  compute_exit_();

  TRACE ("END   PHASE COMPUTE");
}

//----------------------------------------------------------------------



