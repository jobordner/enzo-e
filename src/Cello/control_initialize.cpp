// See LICENSE_CELLO file for license and copyright information

/// @file     control_initialize.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2013-04-26
/// @brief    Charm-related functions associated with initialization
/// @ingroup  Control

#include "simulation.hpp"
#include "mesh.hpp"
#include "control.hpp"

#include "charm_simulation.hpp"
#include "charm_mesh.hpp"

//----------------------------------------------------------------------

void Simulation::initialize() throw()
{
  set_phase(phase_initial);

  initialize_config_();

  initialize_monitor_();
  initialize_memory_();
  initialize_simulation_();
  initialize_balance_();
  initialize_refresh_();

  initialize_data_();

  problem_->initialize(config_,parameters_,factory());

  cello::finalize_fields();

  initialize_hierarchy_();

  initialize_performance_();

  // initialize_block_array() is called in charm_initialize
  // using QD to ensure that initialize_hierarchy() is called
  // on all processors before Blocks are created

  // Create the Block chare array and distribute proxy to all other processes

  CkCallback callback
    (CkIndex_Simulation::r_initialize_next(nullptr), thisProxy);
  
  contribute(callback);
}

//----------------------------------------------------------------------

void Simulation::r_initialize_next(CkReductionMsg * msg) 
{  
  if (CkMyPe() == 0) {
    CProxy_Block block_array;
    bool allocate_data = true;
    block_array = hierarchy_->new_block_proxy (allocate_data);
    // broadcast block_array to other Simulation objects
    thisProxy.p_set_block_array(block_array);
  }
}

//----------------------------------------------------------------------

void Simulation::r_initialize_block_array(CkReductionMsg * msg) 
{
  PERF_START(iperf_initial);
  delete msg;
  initialize_block_array_();
}

//======================================================================
// NEW INITIAL
//======================================================================

void  Block::initial_new_begin_()
{
  index_initial_ = 0;

  CkCallback callback 
    (CkIndex_Block::r_initial_new_next(nullptr), thisProxy);

  contribute(0,0,CkReduction::concat,callback);
}

//----------------------------------------------------------------------

void  Block::initial_new_next_()
{
  const bool initial_restart = cello::config()->initial_restart;

  if ((! initial_restart) && (index_initial_ < cello::num_initial())) {

    cello::initial(index_initial_)->enforce_block(this,nullptr);

  } else {
    // Bypass initialization on restart (may want exceptions)
    bool is_first_cycle = (state_->cycle() == cello::config()->initial_cycle);
    bool level_is_valid = level() <= cello::config()->mesh_max_initial_level;
    if (is_first_cycle && level_is_valid) {
      initial_exit_();
    }
  }
}

//----------------------------------------------------------------------

void  Block::initial_done()
{
  if (cello::config()->initial_new) {

    // barrier before continuing

    CkCallback callback
      (CkIndex_Block::r_initial_new_continue(nullptr), thisProxy);

    contribute(0,0,CkReduction::concat,callback);
  }
}

//----------------------------------------------------------------------

void  Block::initial_new_continue_()
{
  index_initial_++;
  initial_new_next_();
}

//----------------------------------------------------------------------


