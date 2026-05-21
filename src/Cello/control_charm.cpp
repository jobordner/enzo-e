// See LICENSE_CELLO file for license and copyright information

/// @file     control_charm.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2014-02-13
/// @brief    Functions controling control flow of charm entry functions
/// @ingroup  Control

#include "simulation.hpp"
#include "mesh.hpp"
#include "control.hpp"

#include "charm_simulation.hpp"
#include "charm_mesh.hpp"

//----------------------------------------------------------------------

void Block::initial_exit_()
{
  PERF_START(perf_rindex_initial);

  bool initial_restart = cello::config()->initial_restart;

  if (initial_restart) {
    control_sync_barrier (CkIndex_Block::r_restart_enter(NULL));
  } else {
    control_sync_barrier (CkIndex_Block::r_adapt_enter(NULL));
  }
  PERF_STOP(perf_rindex_initial);
}

//----------------------------------------------------------------------

void Block::adapt_exit_()
{
  //  verify_neighbors();

  control_sync_quiescence(CkIndex_Main::p_output_enter());
}

//----------------------------------------------------------------------

void Block::output_exit_()
{
  PERF_START(perf_rindex_output);

  cello::simulation()->monitor_output();

  control_sync_barrier (CkIndex_Block::r_stopping_enter(NULL));

  PERF_STOP(perf_rindex_output);
}

//----------------------------------------------------------------------

void Block::stopping_exit_()
{
  cello::simulation()->monitor_clear();

  if (state_->stopping()) {

    control_sync_barrier (CkIndex_Block::r_exit(NULL));

  } else {

    if (cello::simulation()->cycle_changed()) {
      if (state_->cycle() > cello::simulation()->initial_cycle()) {
        // stop if any previous cycle
        PERF_STOP(perf_rindex_cycle);
      }
      // start 
      PERF_START(perf_rindex_cycle);
    }
    compute_enter_();

  }
}

//----------------------------------------------------------------------

void Block::compute_exit_ ()
{
  cello::simulation()->compute_advance_state();
}

//----------------------------------------------------------------------

void Simulation::compute_advance_state()
{
  // Transition from Block to Simulation parallelism
  if (sync_advance_state_.next()) {
    // Advance Simulation state
    state_->advance(cello::level_top());
    // barrier before exiting compute
    auto callback = CkCallback
      (CkIndex_Simulation::r_advance_state_exit(nullptr),thisProxy);
    contribute(callback);
  }
}

//----------------------------------------------------------------------

void Simulation::r_advance_state_exit(CkReductionMsg * msg)
{
  delete msg;
  if (CkMyPe() == 0) cello::block_array().p_compute_exit_continue();
}

//----------------------------------------------------------------------

void Block::r_compute_exit_continue (CkReductionMsg * msg)

{
  delete msg;
  compute_exit_continue_();
}

//----------------------------------------------------------------------

void Block::p_compute_exit_continue ()

{
  compute_exit_continue_();
}

//----------------------------------------------------------------------

void Block::compute_exit_continue_ ()
{
  if (cello::simulation()->state()->state_type() == State::Type::Level) {
    int ir_cycle_end = cello::simulation()->ir_cycle_end();
    Refresh * refresh = cello::refresh(ir_cycle_end);

    refresh->add_all_fields();
    refresh->add_all_particles();
    refresh->set_global();
    refresh->set_active (is_leaf());
    refresh->set_adaptive_timestep (true);
    refresh->set_callback(CkIndex_Block::p_adapt_enter());
    refresh_start (ir_cycle_end,CkIndex_Block::p_adapt_enter());
  } else {
    adapt_enter_();
  }
}

//----------------------------------------------------------------------

void Block::control_sync (int entry_point, int sync_type, int id_sync,
			  int min_face_rank, int neighbor_type, int root_level,
                          int level_lower, int level_upper,
                          DirType dir_type)
{
  if (sync_type == sync_quiescence) {

    control_sync_quiescence (entry_point);

  } else if (sync_type == sync_neighbor) {

    control_sync_neighbor
      (entry_point,id_sync,min_face_rank, neighbor_type,root_level,
       level_lower,level_upper, dir_type);

  } else if (sync_type == sync_face) {
 
    control_sync_face (entry_point,id_sync,min_face_rank);

  } else if (sync_type == sync_barrier) {

    control_sync_barrier (entry_point);

  } else {

     ERROR2 ("Block::control_sync()",
	     "Unknown sync type %d neighbor type %d",
	     sync_type,neighbor_type);    

  }
}

//----------------------------------------------------------------------

void Block::control_sync_quiescence (int entry_point)
{
  if (index_.is_root())
    CkStartQD(CkCallback (entry_point,proxy_main));
}

//----------------------------------------------------------------------

void Block::control_sync_barrier (int entry_point)
{
  contribute(CkCallback (entry_point,thisProxy));
}

//----------------------------------------------------------------------

void Block::control_sync_neighbor(int entry_point, int id_sync,
                                  int min_face_rank,
                                  int neighbor_type,
                                  int root_level,
                                  int level_lower,
                                  int level_upper,
                                  DirType dir_type)
{
  if ( ! is_leaf() ) {

    CkCallback(entry_point,CkArrayIndexIndex(index_),thisProxy).send(NULL);

    return;
  }

  ASSERT1("control_sync()",
	  "id %d must be specified for neighbor sync and >= 0",
	  id_sync,
	  (id_sync >= 0));

  int num_neighbors = 0;

  ItNeighbor it_neighbor = this->it_neighbor
    (index_,min_face_rank,neighbor_type,root_level,
     level_lower, level_upper, dir_type);

  int of3[3];  // ignored
  while (it_neighbor.next(of3)) {

    ++num_neighbors;

    Index index_neighbor = it_neighbor.index();

    thisProxy[index_neighbor].p_control_sync_count(entry_point,id_sync,0);

  }

  control_sync_count (entry_point, id_sync,num_neighbors + 1);

}

//----------------------------------------------------------------------

void Block::control_sync_face(int entry_point, int id_sync, int min_face_rank)
{

  int num_faces = 0;

  ItFace it_face = this->it_face(min_face_rank,index_);

  int of3[3];
  while (it_face.next(of3)) {

    // Only count face if a Block exists in the level
    if (face_level(of3) >= level()) {
      ++num_faces;

      Index index_face = it_face.index();

      thisProxy[index_face].p_control_sync_count(entry_point,id_sync,0);
    }

  }
  control_sync_count (entry_point,id_sync,num_faces + 1);
}

//----------------------------------------------------------------------

void Block::control_sync_count (int entry_point, int id_sync, int count)
{
  const int n_new = id_sync + 1;
  const int n_now = sync_max_.size();
  if (n_new  > n_now) {
    sync_count_.resize(n_new,0);
    sync_max_.resize(n_new,0);
  }

  if (count != 0)  sync_max_[id_sync] = count;

  ++sync_count_[id_sync];

  // sync_max_ reached: continue and reset counter

  if (sync_max_[id_sync] > 0 && sync_count_[id_sync] >= sync_max_[id_sync]) {

    sync_max_  [id_sync] = 0;
    sync_count_[id_sync] = 0;

    CkCallback(entry_point,CkArrayIndexIndex(index_),thisProxy).send(NULL);

  }
}

//======================================================================

