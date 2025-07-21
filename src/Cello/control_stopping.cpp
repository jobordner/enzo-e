// See LICENSE_CELLO file for license and copyright information

/// @file     control_stopping.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2013-04-26
/// @brief    Charm-related functions associated with initialization
/// @ingroup  Control
///
///    STOPPING
///
///    Block::stopping()
///       update_boundary_()
///       compute dt
///       compute stopping
///       contribute( >>>>> Block::r_output() >>>>> )

#include "simulation.hpp"
#include "mesh.hpp"
#include "control.hpp"

#include "charm_simulation.hpp"
#include "charm_mesh.hpp"

// #define DEBUG_STOPPING

// #define TRACE_DT

// #define DEBUG_STATE

#ifdef DEBUG_STOPPING
#   define TRACE_STOPPING(A)					\
  CkPrintf ("%d %s:%d %s TRACE %s\n",					\
	    CkMyPe(),__FILE__,__LINE__,name_.c_str(),A);
#else
#   define TRACE_STOPPING(A) ;
#endif


//----------------------------------------------------------------------

void Block::stopping_enter_()
{
  stopping_begin_();
}

//----------------------------------------------------------------------

void Block::stopping_begin_()
{

  TRACE_STOPPING("Block::stopping_begin_");

  Simulation * simulation = cello::simulation();

  simulation->set_phase(phase_stopping);

  Problem * problem = simulation->problem();

  //    allocate reduction vector for stopping criteria plus method dt

  // Evaluate local stopping criteria

  const int nm = problem->num_methods();
  const int nl = cello::max_level() + 1;
  const int n = 1 + nm*nl;

  std::vector<double> min_reduce(n,std::numeric_limits<double>::max());

  // Determine whether stopping criteria satisfied
  Stopping * stopping = problem->stopping();
  const int stop_block = stopping->complete(state_->cycle(),state_->time());

  min_reduce[0] = stop_block ? 1.0 : 0.0;

  // Evaluate dt for each method and update for method and this level
  if (is_leaf()) {
    const int il = level();
    for (int im=0; im<nm; im++) {
      const int k = 1+im+nm*(il);
      min_reduce[k] = problem->method(im)->timestep(this);
    }
  }

#ifdef TRACE_CONTRIBUTE
  CkPrintf ("%s %s:%d DEBUG_CONTRIBUTE\n",
            name().c_str(),__FILE__,__LINE__); fflush(stdout);
#endif

  CkCallback callback (CkIndex_Block::r_stopping_compute_timestep(NULL),
                       thisProxy);
  contribute
    (n*sizeof(double), min_reduce.data(), CkReduction::min_double, callback);

}

//----------------------------------------------------------------------

void Block::r_stopping_compute_timestep(CkReductionMsg * msg)
{
  performance_start_(perf_stopping);
  TRACE_STOPPING("Block::r_stopping_compute_timestep");
  ++age_;

  double * min_reduce = (double * )msg->getData();

  auto & state_global = cello::simulation()->state();

  state_       -> set_stopping(min_reduce[0] == 1.0);
  state_global -> set_stopping(min_reduce[0] == 1.0);

  // Compute global and level timesteps

  //    global timestep
  double dt_global = stopping_compute_global_dt_(min_reduce);

  //    level timestep
  std::vector<double> dt_level;
  dt_level.resize(cello::max_level()+1);

  stopping_compute_level_dt_(min_reduce,dt_level);

  // Update method timesteps for supercycling
  stopping_update_method_state_(min_reduce,dt_global);

  delete msg;

  // Update Block and Simulation state global and level timesteps

  state_      ->set_dt (dt_global);
  state_global->set_dt (dt_global);

  // Initialize level dt in state() objects
  int level = 0;
  for (auto & dt : dt_level) {
    state_      ->set_dt (dt,level);
    state_global->set_dt (dt,level);
    level++;
  }

  performance_projections_update_logging_();

  stopping_balance_();

  performance_stop_(perf_stopping);
}

//----------------------------------------------------------------------

void Block::performance_projections_update_logging_()
{

#ifdef CONFIG_USE_PROJECTIONS
  bool was_off = (simulation->projections_tracing() == false);
  bool was_on  = (simulation->projections_tracing() == true);
  Schedule * schedule_on = simulation->projections_schedule_on();
  Schedule * schedule_off = simulation->projections_schedule_off();
  bool turn_on  = schedule_on ?
    schedule_on->write_this_cycle(cycle_,time_) : false;
  bool turn_off = schedule_off ?
    schedule_off->write_this_cycle(cycle_,time_) : false;

  static bool active = false;
  if (!active && turn_on) {
    active = true;
    simulation->monitor()->print
      ("Performance","turning projections logging ON\n");

    simulation->set_projections_tracing(true);

    traceBegin();

  } else if (active && turn_off) {
    active = false;

    simulation->monitor()->print
      ("Performance","turning projections logging OFF\n");

    simulation->set_projections_tracing(false);

    traceEnd();

  }
#endif

}

//----------------------------------------------------------------------

double Block::stopping_compute_global_dt_ (double min_reduce[])
{
  Problem * problem = cello::simulation()->problem();

  // compute minimum timestep dt_global over all methods and all levels

  double dt_global = std::numeric_limits<double>::max();

  const int nm = problem->num_methods();
  const int nl = cello::max_level() + 1;

  for (int il=0; il<nl; il++) {
    for (int im=0; im<nm; im++) {
      const int k = 1+im+nm*(il);
      dt_global = std::min(dt_global,min_reduce[k]);
    }
  }

  // Adjust timestep dt for global courant condition
  dt_global *= Method::courant_global;

  // adjust timestep dt to align with any scheduled output times
  double time_curr = state_->time();
  int index_output=0;
  while (Output * output = problem->output(index_output++)) {
    Schedule * schedule = output->schedule();
    dt_global = schedule->update_timestep(time_curr,dt_global);
  }

  // Reduce timestep to not overshoot final time from stopping criteria

  double time_stop = problem->stopping()->stop_time();

  dt_global = std::min (dt_global, (time_stop - time_curr));

  return dt_global;
}

//----------------------------------------------------------------------

void Block::stopping_compute_level_dt_(double min_reduce[], std::vector <double> & dt_level)
{
  Problem * problem = cello::simulation()->problem();

  // compute minimum timestep dt_level[] for each level over all methods

  for (auto & dt : dt_level) dt = std::numeric_limits<double>::max();

  const int nm = problem->num_methods();
  const int nl = cello::max_level() + 1;

  for (int il=0; il<nl; il++) {
    for (int im=0; im<nm; im++) {
      const int k = 1+im+nm*(il);
      dt_level[il] = std::min(dt_level[il],min_reduce[k]);
    }
  }

  // Adjust dt for global courant condition
  for (auto & dt : dt_level) dt *= Method::courant_global;

  // Apply max_level_dt_ratio to limit dt ratios between levels
  double max_ratio = cello::config()->timestep_max_level_dt_ratio;

  //   find level with minimum dt (may not be finest level if not fully refined)
  int level_dt_min = std::distance
    (dt_level.begin(),std::min_element (dt_level.begin(),dt_level.end()));
  double dt_min = *std::min_element (dt_level.begin(),dt_level.end());
  //   enforce ratio limit
  int level = 0;
  for (auto & dt : dt_level) {
    dt = std::min(dt,dt_min*std::pow(max_ratio,level_dt_min-level));
    level++;
  }

  // adjust timesteps to align with any scheduled output times
  int index_output=0;
  while (Output * output = problem->output(index_output++)) {
    Schedule * schedule = output->schedule();
    int level = 0;
    for (auto & dt : dt_level) {
      double time_curr = state_->time(level++);
      dt = schedule->update_timestep(time_curr,dt);
    }
  }

  // Reduce timestep to not overshoot coarser timestep
  for (int level = cello::max_level(); level > 0; level--) {
    if (dt_level[level-1] != std::numeric_limits<double>::max()) 
      dt_level[level] = std::min(dt_level[level],
                                 state_->time(level-1)
                                 - state_->time(level)
                                 + dt_level[level-1]);
  }

  // Reduce timesteps to not overshoot final time from stopping criteria
  double time_stop = problem->stopping()->stop_time();
  level = 0;
  for (auto & dt : dt_level) {
    double time_curr = state_->time(level++);
    dt = std::min (dt, (time_stop - time_curr));
  }
}

//----------------------------------------------------------------------

void Block::stopping_update_method_state_
(double min_reduce[], double dt_global)
{
  // update Method states for supercycling
  Simulation * simulation = cello::simulation();
  Problem * problem = simulation->problem();
#ifdef DEBUG_STATE
  // Write current state
  if (index().is_root()) {
    state()->print("update_method_state");
  }
#endif
  for (int k=0; k<problem->num_methods(); k++) {
  //  for (int k=0; k<state()->num_methods(); k++) {
    const double dt_method = min_reduce[k+1];
    const int max_super = problem->method(k)->max_supercycle();
    const double max_dt_method = dt_global*max_super;
    const double ratio = dt_method / dt_global;
    const int desired_super = int(std::floor(ratio));
    const int allowed_super = std::min(desired_super,max_super);
    State::MethodState & method_state = state_->method(k);
    const int step = method_state.step();
    const int num_steps = method_state.num_steps();
    bool update_state = (step >= num_steps);
    if (update_state) {
      method_state.set_time(state_->time());
      method_state.set_dt(allowed_super * dt_global);
      method_state.set_num_steps(allowed_super);
      method_state.set_step(0);
    }
  }
}

//----------------------------------------------------------------------

void Block::stopping_balance_()
{
  TRACE_STOPPING("Block::stopping_balance_");

  Schedule * schedule = cello::simulation()->schedule_balance();

  bool do_balance =
    (schedule && schedule->write_this_cycle(state_->cycle(),state_->time()));

  if (do_balance) {

    const std::string balance_type = cello::config()->balance_type;

    if (balance_type == "cello") {

      // See EnzoMethodBalance

    } else if (balance_type == "charm") {

      // Charm++-controlled load balancing
      if (index_.is_root())
        cello::monitor()->print ("Balance","starting load balance step");
    }

    CkCallback callback = CkCallback
      (CkIndex_Block::r_stopping_load_balance(nullptr), proxy_array());

    adapt_ready_ = true;

    contribute(callback);

  } else {

    stopping_exit_();

  }
}

//----------------------------------------------------------------------

void Block::stopping_load_balance_()
{
  performance_start_(perf_stopping);
  TRACE_STOPPING("load_balance begin");
  cello::simulation()->set_phase (phase_balance);

  AtSync();
  performance_stop_(perf_stopping);
}

//----------------------------------------------------------------------

void Block::ResumeFromSync()
{
  TRACE_STOPPING("load_balance exit");

  stopping_exit_();
}

//----------------------------------------------------------------------

void Block::exit_()
{

  TRACE_STOPPING("Block::exit_");
  const int in = cello::index_static();
  if (index().is_root()) {
    if (DataMsg::counter[in] != 0) {
      CkPrintf ("%d Block::exit_() DataMsg::counter = %ld != 0\n",
		CkMyPe(),DataMsg::counter[in]);
      CkPrintf ("%d Block::exit_() ParticleData::counter = %ld != 0\n",
		CkMyPe(),ParticleData::counter[in]);
    }
    if (FieldFace::counter[in] != 0) {
      CkPrintf ("%d Block::exit_() FieldFace::counter = %ld != 0\n",
		CkMyPe(),FieldFace::counter[in]);
    }
    if (MsgCoarsen::counter[in] != 0) {
      CkPrintf ("%d Block::exit_() MsgCoarsen::counter = %ld != 0\n",
		CkMyPe(),MsgCoarsen::counter[in]);
    }
    if (MsgInitial::counter[in] != 0) {
      CkPrintf ("%d Block::exit_() MsgInitial::counter = %ld != 0\n",
		CkMyPe(),MsgInitial::counter[in]);
    }
    if (MsgOutput::counter[in] != 0) {
      CkPrintf ("%d Block::exit_() MsgOutput::counter = %ld != 0\n",
		CkMyPe(),MsgOutput::counter[in]);
    }
    if (MsgRefine::counter[in] != 0) {
      CkPrintf ("%d Block::exit_() MsgRefine::counter = %ld != 0\n",
		CkMyPe(),MsgRefine::counter[in]);
    }
    if (MsgRefresh::counter[in] != 0) {
      CkPrintf ("%d Block::exit_() MsgRefresh::counter = %ld != 0\n",
		CkMyPe(),MsgRefresh::counter[in]);
    }
  }
  if (index_.is_root()) {
    proxy_main.p_exit(1);
  }
}
