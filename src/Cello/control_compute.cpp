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

#ifdef DEBUG_COMPUTE
#define TRACE_COMPUTE(FUN,NUM)                                          \
  if (state()->cycle() >= CYCLE)                                        \
    CkPrintf ("%02d %d %d %s %s %d DEBUG_COMPUTE %s()\n",               \
              state()->cycle(),                                         \
              NUM,                                                      \
              CkMyPe(),                                                 \
              name().c_str(),                                           \
              this->method()?this->method()->name().c_str():"null",     \
              this->method()?index_method_ : 0,                         \
              FUN);
#else
#define TRACE_COMPUTE(FUN,NUM) /* ... */
#endif

//======================================================================

void Block::compute_enter_ ()
{
  int ir_cycle_begin = cello::simulation()->ir_cycle_begin();

  TRACE_COMPUTE("compute_enter",0);

  if (cello::simulation()->hierarchy()->num_blocks_changed() > 0) {
    Refresh * refresh = cello::refresh(ir_cycle_begin);

    refresh->add_all_fields();
    refresh->add_all_particles();
    refresh->set_global();
    refresh->set_active (is_leaf());
    refresh -> set_adaptive_timestep
      (cello::simulation()->state()->state_type() == State::Type::Level);
    refresh->set_callback(CkIndex_Block::p_compute_begin());

    refresh_start (ir_cycle_begin,CkIndex_Block::p_compute_begin());
  } else {
    compute_begin_();
  }
}

//----------------------------------------------------------------------

void Block::compute_begin_ ()
{
  cello::simulation()->set_phase(phase_compute);

  // Update old fields

  if (state()->is_active(level()) ) {
    data()->field().save_history(state()->time(level()));
  }

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

      TRACE_COMPUTE("compute_next",1);

      int ir_post = method->refresh_id_post();

      Refresh * refresh = cello::refresh(ir_post);

      refresh->set_active (is_leaf());

      refresh -> set_level_lower(std::max(0,state()->level_lower()-1));
      refresh -> set_level_upper(state()->level_upper());
      refresh -> set_adaptive_timestep
        (cello::simulation()->state()->state_type() == State::Type::Level);
      refresh_start (ir_post,CkIndex_Block::p_compute_continue());

    } else {

      compute_continue_();

    }

  } else {

    // Final refresh
    compute_end_();

  }
}

//----------------------------------------------------------------------

void Block::compute_continue_ ()
{
  TRACE_COMPUTE("Block::compute_continue",2);

#ifdef CONFIG_USE_PROJECTIONS
  //  double time_start = CmiWallTimer();
#endif

  Method * method = this->method();

  PERF_METHOD_START(method);
  const bool is_scheduled = method->is_scheduled(this);
  const bool is_active = method->is_active(state(),level());

  if (is_scheduled && is_active) {

    TRACE2 ("Block::compute_continue() method = %d %p\n",
	    index_method_,method); fflush(stdout);

    TRACE_COMPUTE("applying method",3);

    method->compute (this);

  } else {

    compute_done();

  }
}

//----------------------------------------------------------------------

void Block::compute_done ()
{
  TRACE_COMPUTE("Block::compute_done",4);

  PERF_METHOD_STOP(method());
  compute_update_method_state_(index_method_);

  index_method_++;
  compute_next_();
}

//----------------------------------------------------------------------

void Block::compute_update_method_state_(int index_method)
{
  if (index_method_ < state_->num_methods()) {
    auto & method_state = state()->method(index_method);  
    method_state.advance(state_->dt());
  }
}

//----------------------------------------------------------------------

void Block::compute_end_ ()
{
  TRACE_COMPUTE("Block::compute_end",5);

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



