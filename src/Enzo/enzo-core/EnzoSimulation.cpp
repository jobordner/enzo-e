// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSimulation.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2011-03-17
/// @brief    Implementation of EnzoSimulation user-dependent class member functions

#include "cello.hpp"

#include "enzo.hpp"

#include "charm_simulation.hpp"
#include "charm_mesh.hpp"

#include "simulation.hpp"

//----------------------------------------------------------------------

CProxy_EnzoSimulation proxy_enzo_simulation;
CProxy_IoEnzoWriter   proxy_io_enzo_writer;
CProxy_IoEnzoReader   proxy_io_enzo_reader;
CProxy_EnzoLevelArray proxy_level_array;

//----------------------------------------------------------------------

EnzoSimulation::EnzoSimulation
(
 const char         parameter_file[],
 int                n)
  : CBase_EnzoSimulation(parameter_file, n),
    infer_count_arrays_(0),
    check_num_files_(0),
    check_ordering_(""),
    check_directory_(),
    restart_level_(0)
{
  // replace Simulation's State with EnzoState
  state_ = std::make_shared<EnzoState> (0, 0.0, 0.0, false);
  factory_ = std::make_shared<EnzoFactory>();

  // Synchronize to ensure all EnzoSimulation objects exist before
  // reading parameters

  CkCallback callback (CkIndex_EnzoSimulation::r_startup_begun(nullptr),
                       thisProxy);
  contribute(callback);

}

EnzoSimulation::EnzoSimulation() : CBase_EnzoSimulation()
{
  // replace Simulation's State with EnzoState
  state_ = std::make_shared<EnzoState> (0, 0.0, 0.0, false);
}

EnzoSimulation::EnzoSimulation(CkMigrateMessage * m) : CBase_EnzoSimulation(m)
{
  // replace Simulation's State with EnzoState
  state_ = std::make_shared<EnzoState> (0, 0.0, 0.0, false);
}

//----------------------------------------------------------------------

EnzoSimulation::~EnzoSimulation()
{
}

//----------------------------------------------------------------------

void EnzoSimulation::pup (PUP::er &p)
{
  // NOTE: change this function whenever attributes change

  CBase_EnzoSimulation::pup(p);

  TRACEPUP;

  p | sync_check_writer_created_;
  p | sync_check_done_;
  p | sync_infer_count_;
  p | sync_infer_create_;
  p | sync_infer_done_;
  p | infer_count_arrays_;
  p | check_num_files_;
  p | check_ordering_;
  p | check_directory_;
  p | restart_level_;
}

//----------------------------------------------------------------------
void EnzoSimulation::p_refine_create_block(MsgRefine * msg)
{ refine_create_block(msg); }

void EnzoSimulation::refine_create_block(MsgRefine * msg)
{
  Index index = msg->index();
  check_msg_state_(index,"p_refine_create_block()",
                    (msg_refine_map_[index] == nullptr));

  msg_refine_map_[index] = msg;

  int ip = CkMyPe();
  enzo::block_array()[index].insert(MsgType::msg_refine,ip);
}

//----------------------------------------------------------------------

void EnzoSimulation::initialize_from_checkpoint
(Index index, EnzoMsgCheck * msg)
{
  check_msg_state_(index,"initialize_from_checkpoint()",
                    (msg_check_map_[index] == nullptr));
  msg_check_map_[index] = msg;
}

//----------------------------------------------------------------------

EnzoMsgCheck * EnzoSimulation::get_msg_check(Index index)
{
  EnzoMsgCheck * msg = msg_check_map_[index];
  check_msg_state_(index,"get_msg_check()", (msg != nullptr));
  msg_check_map_.erase(index);
  return msg;
}

//----------------------------------------------------------------------

void EnzoSimulation::r_startup_begun (CkReductionMsg *msg)
{
  delete msg;

  initialize();
}

//----------------------------------------------------------------------

void EnzoSimulation::initialize() throw()
{
  problem_ = new EnzoProblem;
  config_ = static_cast<EnzoConfig*>(&g_enzo_config);

  // Call initialize() on base Simulation class
  Simulation::initialize();
}

//----------------------------------------------------------------------

void EnzoSimulation::monitor_output()
{
  if (! (monitor_flag_ && (CkMyPe() == 0))) return;

  Simulation::monitor_output();

  if (enzo::cosmology() && (CkMyPe() == 0)) {
    cello::monitor()->print
      ("Simulation", "redshift %.8f", enzo::cosmology()->current_redshift());
  }
}

//======================================================================


