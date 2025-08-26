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

// #define DEBUG_ADAPT
// #define DEBUG_ATS
// #define DEBUG_CONTROL
// #define DEBUG_REFRESH
// #define TRACE_ATS
// #define TRACE_CONTRIBUTE
// #define TRACE_CONTROL

// #define BLOCK  "B0:100_0:101"


#ifdef TRACE_CONTROL
# define TRACE_BLOCK (state()->cycle()>=0)
# undef TRACE_CONTROL
# define TRACE_CONTROL(A)                               \
  if (TRACE_BLOCK) {                                    \
    CkPrintf ("%d %s:%d %s TRACE_CONTROL %s \n",        \
              CkMyPe(),__FILE__,__LINE__,               \
              name_.c_str(), A);                        \
    fflush(stdout);                                     \
  }
# define TRACE_SYNC(A)                                          \
  if (TRACE_BLOCK) {                                            \
    CkPrintf ("%d %s:%d %s TRACE_SYNC %s entry %d id %d\n",	\
              CkMyPe(),__FILE__,__LINE__,                       \
              name_.c_str(), A,entry_point,id_sync);            \
    fflush(stdout);                                             \
  }
#else
# define TRACE_CONTROL(A) ;
# define TRACE_SYNC(A) ;
#endif


//----------------------------------------------------------------------

void Block::initial_exit_()
{
  performance_start_(perf_initial);
  TRACE_CONTROL("initial_exit_");

#ifdef TRACE_CONTRIBUTE  
  CkPrintf ("%s %s:%d DEBUG_CONTRIBUTE calling r_adapt_enter\n",
	    name().c_str(),__FILE__,__LINE__);
  fflush(stdout);
#endif

  bool initial_restart = cello::config()->initial_restart;

  if (initial_restart) {
    control_sync_barrier (CkIndex_Block::r_restart_enter(NULL));
  } else {
    control_sync_barrier (CkIndex_Block::r_adapt_enter(NULL));
  }
  performance_stop_(perf_initial);
}

//----------------------------------------------------------------------

void Block::adapt_exit_()
{
#ifdef DEBUG_ADAPT
  CkPrintf ("DEBUG_ADAPT %s A adapt_begin_\n",name().c_str());
  fflush(stdout);
#endif  
  TRACE_CONTROL("adapt_exit");

  //  verify_neighbors();

  control_sync_quiescence(CkIndex_Main::p_output_enter());
}

//----------------------------------------------------------------------

void Block::output_exit_()
{
  performance_start_(perf_output);

  TRACE_CONTROL("output_exit");

  if (index_.is_root()) {
    cello::simulation()->monitor_output();
  }

  performance_stop_(perf_output);

#ifdef TRACE_CONTRIBUTE  
  CkPrintf ("%s %s:%d DEBUG_CONTRIBUTE calling r_stopping_enter()\n",
	    name().c_str(),__FILE__,__LINE__);
  fflush(stdout);
#endif  
  control_sync_barrier (CkIndex_Block::r_stopping_enter(NULL));

}

//----------------------------------------------------------------------

void Block::stopping_exit_()
{
  TRACE_CONTROL("stopping_exit");

  if (cello::simulation()->cycle_changed()) {
    // if performance counters haven't started yet for this cycle
    int cycle_initial = cello::config()->initial_cycle;
    if (state_->cycle() > cycle_initial) {
      // stop if any previous cycle
      performance_stop_(perf_cycle,__FILE__,__LINE__);
    }
    // start 
    performance_start_ (perf_cycle,__FILE__,__LINE__);
  }

  if (state_->stopping()) {

#ifdef TRACE_CONTRIBUTE  
    CkPrintf ("%s %s:%d DEBUG_CONTRIBUTE calling r_exit()\n",
	    name().c_str(),__FILE__,__LINE__);
  fflush(stdout);
#endif  
    control_sync_barrier (CkIndex_Block::r_exit(NULL));

  } else {

    compute_enter_();

  }
}

//----------------------------------------------------------------------

void Block::compute_exit_ ()
{
  control_sync_barrier(CkIndex_Block::r_compute_exit_continue(nullptr));
}

//----------------------------------------------------------------------

void Block::r_compute_exit_continue (CkReductionMsg * msg)
{
  delete msg;

  update_global_state_();

  TRACE_CONTROL("compute_exit_continue");

  if (cello::simulation()->state()->state_type() == State::Type::Level) {
    int ir_cycle_end = cello::simulation()->ir_cycle_end();
    Refresh * refresh = cello::refresh(ir_cycle_end);

    refresh->add_all_fields();
    refresh->add_all_particles();
    refresh->set_global();
    refresh->set_active (is_leaf());
    refresh -> set_adaptive_timestep
      (cello::simulation()->state()->state_type() == State::Type::Level);
    //    refresh -> set_advanced_time(true);
    refresh->set_callback(CkIndex_Block::p_adapt_enter());

    refresh_start (ir_cycle_end,CkIndex_Block::p_adapt_enter());
  } else {
    adapt_enter_();
  }
}

//----------------------------------------------------------------------

void Block::update_global_state_()
{

  auto & state_global = cello::simulation()->state();
  // update simulation global state
  state_global->set_cycle(state()->cycle());
  state_global->set_time (state()->time());

  if ( (state()->state_type() == State::Type::Level) &&
       (level_lower_ <= level() && level() < level_upper_)) {

    state_global->set_level_range(level_lower_,level_upper_);
    // update simulation level states using saved level range
    for (int level=level_lower_; level < level_upper_; level++) {
      state_global->set_cycle(state()->cycle(level),level);
      state_global->set_time (state()->time (level),level);
    }
    // extend to finer levels if finest level < max_level
    const int level_top = cello::hierarchy()->finest_level();
    const int level_max = cello::hierarchy()->max_level();
    for (int level=level_top+1; level<=level_max; level++) {
      state_global->set_cycle(state()->cycle(level_top),level);
      state_global->set_time(state()->time(level_top),level);
    }
  }
}

//----------------------------------------------------------------------

void Block::control_sync (int entry_point, int sync_type, int id_sync,
			  int min_face_rank, int neighbor_type, int root_level,
                          int level_lower, int level_upper,
                          DirType dir_type)
{
  TRACE_CONTROL("control_sync()");
  TRACE_SYNC("control_sync()");
#ifdef DEBUG_CONTROL
  CkPrintf ("DEBUG_CONTROL %s entry_point = %d\n",name().c_str(),entry_point);
  fflush(stdout);
#endif  

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
  TRACE_CONTROL("control_sync_neighbor");
  TRACE_SYNC("control_sync_neighbhor()");

  if ( ! is_leaf() ) {

    TRACE_CONTROL("control_sync_neighbor ! is_leaf()");
    CkCallback(entry_point,CkArrayIndexIndex(index_),thisProxy).send(NULL);

    return;
  }

  ASSERT1("control_sync()",
	  "id %d must be specified for neighbor sync and >= 0",
	  id_sync,
	  (id_sync >= 0));

#ifdef DEBUG_REFRESH
  CkPrintf ("%d DEBUG_REFRESH %s neighbor sync id %d\n",
	    CkMyPe(), name().c_str(),id_sync);
  fflush(stdout);
#endif

  int num_neighbors = 0;

  ItNeighbor it_neighbor = this->it_neighbor
    (index_,min_face_rank,neighbor_type,root_level,
     level_lower, level_upper, dir_type);

  int of3[3];  // ignored
  while (it_neighbor.next(of3)) {

    ++num_neighbors;

    Index index_neighbor = it_neighbor.index();

#ifdef DEBUG_CONTROL
    CkPrintf ("%s DEBUG_CONTROL calling p_control_sync_count (%d %d 0)\n",
	      name().c_str(),entry_point,id_sync);
    fflush(stdout);
#endif
    thisProxy[index_neighbor].p_control_sync_count(entry_point,id_sync,0);

  }
#ifdef DEBUG_CONTROL
    CkPrintf ("%s DEBUG_CONTROL calling p_control_sync_count count %d (%d %d 0)\n",
	      name().c_str(),num_neighbors, entry_point,id_sync);
    fflush(stdout);
#endif
    control_sync_count (entry_point, id_sync,num_neighbors + 1);

}

//----------------------------------------------------------------------

void Block::control_sync_face(int entry_point, int id_sync, int min_face_rank)
{

  TRACE_CONTROL("control_sync_face");
  TRACE_SYNC("control_sync_face()");

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
#ifdef DEBUG_CONTROL
  CkPrintf ("%s DEBUG_CONTROL control_sync_count %d %d %d/%d\n",
	    name().c_str(),entry_point,id_sync,sync_count_[id_sync],sync_max_[id_sync]);
  fflush(stdout);
#endif
  
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

