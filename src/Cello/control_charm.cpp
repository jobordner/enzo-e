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

  if (index_.is_root()) {
    cello::simulation()->monitor_output();
  }

#ifdef TRACE_CONTRIBUTE  
  CkPrintf ("%s %s:%d DEBUG_CONTRIBUTE calling r_stopping_enter()\n",
	    name().c_str(),__FILE__,__LINE__);
  fflush(stdout);
#endif  

  control_sync_barrier (CkIndex_Block::r_stopping_enter(NULL));

  PERF_STOP(perf_rindex_output);
}

//----------------------------------------------------------------------

void Block::stopping_exit_()
{
  if (stop_) {

    control_sync_barrier (CkIndex_Block::r_exit(NULL));

  } else {

    if (cello::simulation()->cycle_changed()) {
      if (cycle_ > cello::simulation()->initial_cycle()) {
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
  control_sync_barrier(CkIndex_Block::r_adapt_enter(NULL));
}

//----------------------------------------------------------------------

void Block::control_sync (int entry_point, int sync_type, int id_sync,
			  int min_face_rank, int neighbor_type, int root_level)
{
  if (sync_type == sync_quiescence) {

    control_sync_quiescence (entry_point);

  } else if (sync_type == sync_neighbor) {

    control_sync_neighbor
      (entry_point,id_sync,min_face_rank, neighbor_type,root_level);

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
				  int root_level)
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

  const int min_level = cello::config()->mesh_min_level;

  ItNeighbor it_neighbor = this->it_neighbor
    (index_,min_face_rank,neighbor_type,min_level,root_level);

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

