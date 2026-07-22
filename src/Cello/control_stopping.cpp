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

//----------------------------------------------------------------------

void Block::stopping_enter_()
{
  stopping_begin_();
}

//----------------------------------------------------------------------

void Block::stopping_begin_()
{
  PERF_START(iperf_stopping);

  cello::simulation()->set_phase(phase_stopping);

  //    allocate reduction vector for stopping criteria plus method dt

  // Evaluate local stopping criteria

  const int nm = cello::problem()->num_methods();
  const int nl = cello::max_level() + 1;
  const int n = 1 + nm*nl;

  std::vector<double> min_reduce(n,std::numeric_limits<double>::max());

  // Determine whether stopping criteria satisfied
  const int stop_block = cello::stopping()->complete
    (state_->cycle(),state_->time());

  min_reduce[0] = stop_block ? 1.0 : 0.0;

  // Evaluate dt for each method and update for method and this level
  if (is_leaf()) {
    const int il = level();
    for (int im=0; im<nm; im++) {
      const int k = 1+im+nm*(il);
      min_reduce[k] = cello::method(im)->timestep(this);
    }
  }

  CkCallback callback (CkIndex_Block::r_stopping_compute_timestep(NULL),
                       thisProxy);
  contribute
    (n*sizeof(double), min_reduce.data(), CkReduction::min_double, callback);

  PERF_STOP(iperf_stopping);
}

//----------------------------------------------------------------------

void Block::r_stopping_compute_timestep(CkReductionMsg * msg)
{
  /* PERF_REDUCE_STOP(iperf_reduce_stopping); */
  PERF_START(iperf_stopping);

  ++age_;

  double * min_reduce = (double * )msg->getData();

  auto & state_global = cello::simulation()->state();

  state_       -> set_stopping(min_reduce[0] == 1.0);
  state_global -> set_stopping(min_reduce[0] == 1.0);

  // Compute global and level timesteps

  const int nm = cello::problem()->num_methods();
  const int nl = cello::max_level() + 1;

  // Extract global timestep, timestep per level, and timestep per method

  auto dt_level =    stopping_dt_level_ (min_reduce,nl,nm);
  auto dt_method =   stopping_dt_method_(min_reduce,nl,nm);
  double dt_global = stopping_dt_global_(min_reduce,nl,nm);

  for (int k=cello::level_root();
       k <=  cello::level_top();
       k++) {
    if (dt_level[k] > 0) {
      dt_global = std::min(dt_global,dt_level[k]);
    }
  }

  delete msg;

  // Update method timesteps for supercycling
  stopping_update_method_state_(dt_method,dt_global);

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

  PERF_STOP(iperf_stopping);
}

//----------------------------------------------------------------------

void Block::stopping_update_method_state_
(const std::vector<double> & dt_method, double dt_global)
{
  // update Method states for supercycling

  for (int k=0; k<dt_method.size(); k++) {
    const int max_super = cello::method(k)->max_supercycle();
    const double max_dt_method = dt_global*max_super;
    const double ratio = dt_method[k] / dt_global;
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

double Block::stopping_dt_global_
(const double min_reduce[],int nl,int nm)
{
  // compute minimum timestep dt_global over all methods and all levels

  double dt_global = std::numeric_limits<double>::max();

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

  for (int k=0; k<cello::num_output(); k++) {
    Schedule * schedule = cello::output(k)->schedule();
    dt_global = schedule->update_timestep(time_curr,dt_global);
  }

  // Reduce timestep to not overshoot final time from stopping criteria

  double time_stop = cello::stopping()->stop_time();

  dt_global = std::min (dt_global, (time_stop - time_curr));

  return dt_global;
}

//----------------------------------------------------------------------

std::vector<double> Block::stopping_dt_method_
(const double min_reduce[], int nl, int nm)
{
  std::vector<double> dt_method;
  dt_method.resize(nm);

  for (int im=0; im<nm; im++) {
    dt_method[im] = std::numeric_limits<double>::max();
    for (int il=0; il<nl; il++) {
      const int k = 1+im+nm*(il);
      dt_method[im] = std::min(dt_method[im],min_reduce[k]);
    }
  }
  return dt_method;
}
//----------------------------------------------------------------------

std::vector<double> Block::stopping_dt_level_
(const double min_reduce[], int nl, int nm)
{
  std::vector<double> dt_level;
  dt_level.resize(nl);

  // compute minimum timestep dt_level[] for each level over all methods

  double max = std::numeric_limits<double>::max();
  for (auto & dt : dt_level) dt = max;

  // Initialize level timesteps
  for (int il=0; il<nl; il++) {
    for (int im=0; im<nm; im++) {
      const int k = 1+im+nm*(il);
      dt_level[il] = std::min(dt_level[il],min_reduce[k]);
    }
  }

  // Adjust level timesteps for global courant condition
  for (auto & dt : dt_level) dt *= Method::courant_global;

  // Apply max_level_dt_ratio to limit timestep ratios between levels
  double max_ratio = cello::config()->timestep_max_level_dt_ratio;
  int level_dt_min = std::distance
    (dt_level.begin(),std::min_element (dt_level.begin(),dt_level.end()));
  double dt_min = *std::min_element (dt_level.begin(),dt_level.end());
  int level = 0;
  for (auto & dt : dt_level) {
    dt = std::min(dt,dt_min*std::pow(max_ratio,level_dt_min - level));
    level++;
  }

  // adjust level timesteps to align with any scheduled output times
  for (int k=0; k<cello::num_output(); k++) {
    Schedule * schedule = cello::output(k)->schedule();
    int level = 0;
    for (auto & dt : dt_level) {
      double time_curr = state_->time(level++);
      dt = schedule->update_timestep(time_curr,dt);
    }
  }

  const int level_lower = state_->level_lower();
  const int level_upper = state_->level_upper();

  // Adjust coarser dt to be k(1-e)*dt_h for integer k and small e to
  // reduce sliver timesteps at finest level
  double tol = cello::config()->timestep_adjust_tolerance;
  std::string adjust_type = cello::config()->timestep_adjust_type;
  if (adjust_type != "none") {
    bool l_prev = false;;
    if (adjust_type == "previous") {
      l_prev = true;
    } else if (adjust_type == "finest") {
      l_prev = false;
    } else {
      ERROR1 ("Block::stopping_dt_level_()",
              "Unknown Timestep:adjust_type parameter value %s: "
              "must be [\"none\"|\"previous\"|\"finest\"]",
              adjust_type.c_str());
    }
    const double dth = dt_level[cello::max_level()];
    for (int level=level_upper - 2; level >= level_lower; level--) {
      double dtp = dt_level[level+1];
      double dt = l_prev ? dtp : dth;
      // double dt = dtp; // dt previous level
      dt_level[level] = dt*std::max (1.0, tol*std::floor(dt_level[level]/dt));
    }
  }

  // Reduce level timesteps to not overshoot next-coarser timestep
  if (state_->state_next() == State::Next::Sequential) {
    for (int level = cello::level_root()+1;
         level <= cello::max_level(); level++) {
      if (state_->is_active(level)) {
        if (dt_level[level-1] != std::numeric_limits<double>::max()) {
          dt_level[level] = std::min
            (dt_level[level], state_->time(level-1) - state_->time(level));
        }
      }
    }
  } else if (state_->state_next() == State::Next::Concurrent) {
    if (level_lower > cello::level_root()) {
      for (int level = cello::level_root()+1;
           level <= cello::max_level(); level++) {
        if (state_->is_active(level)) {
          if (dt_level[level-1] != std::numeric_limits<double>::max()) {
            dt_level[level] = std::min
              (dt_level[level], state_->time(level_lower-1) - state_->time(level));
          }
        }
      }
    }
  }

  // Reduce level timesteps to not overshoot time stopping criteria
  double time_stop = cello::stopping()->stop_time();
  level = 0;
  for (auto & dt : dt_level) {
    double time_curr = state_->time(level++);
    dt = std::min (dt, (time_stop - time_curr));
  }

  return dt_level;
}

//----------------------------------------------------------------------

void Block::stopping_balance_()
{
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
    /*    PERF_REDUCE_START(iperf_reduce_balance); */
    contribute(callback);

  } else {

    stopping_exit_();

  }
}

//----------------------------------------------------------------------

void Block::r_stopping_load_balance(CkReductionMsg *msg)
{
  delete msg;
  /*  PERF_REDUCE_STOP(iperf_reduce_balance); */
  cello::simulation()->set_phase (phase_balance);

  AtSync();
}

//----------------------------------------------------------------------

void Block::ResumeFromSync()
{
  //  PERF_STOP(iperf_balance);
  adapt_ready_ = true;
  /*  PERF_REDUCE_START(iperf_reduce_balance); */

  CkCallback callback = CkCallback
    (CkIndex_Block::r_stopping_exit(nullptr), proxy_array());

  contribute(callback);
}

//----------------------------------------------------------------------

void Block::performance_projections_update_logging_()
{

#ifdef CONFIG_USE_PROJECTIONS
  Performance * performance = cello::performance();
  bool was_off = (performance->projections_tracing() == false);
  bool was_on  = (performance->projections_tracing() == true);
  Schedule * schedule_on = performance->projections_schedule_on();
  Schedule * schedule_off = performance->projections_schedule_off();
  bool turn_on  = schedule_on ?
    schedule_on->write_this_cycle(state()->cycle(),state()->time()) : false;
  bool turn_off = schedule_off ?
    schedule_off->write_this_cycle(state()->cycle(),state()->time()) : false;

  static bool active = false;
  if (!active && turn_on) {
    active = true;
    cello::monitor()->print
      ("Performance","turning projections logging ON\n");

    performance->set_projections_tracing(true);

    traceBegin();

  } else if (active && turn_off) {
    active = false;

    cello::monitor()->print
      ("Performance","turning projections logging OFF\n");

    performance->set_projections_tracing(false);

    traceEnd();

  }
#endif

}

//----------------------------------------------------------------------

void Block::exit_()
{
  const int in = cello::index_static();
  if (index().is_root()) {
    if (DataMsg::counter[in]) {
      CkPrintf ("%d Block::exit_() DataMsg::counter = %lld != 0\n",
		CkMyPe(),DataMsg::counter[in]);
    }
    if (ParticleData::counter[in]) {
      CkPrintf ("%d Block::exit_() ParticleData::counter = %lld != 0\n",
		CkMyPe(),ParticleData::counter[in]);
    }
    if (FieldFace::counter[in]) {
      CkPrintf ("%d Block::exit_() FieldFace::counter = %lld != 0\n",
		CkMyPe(),FieldFace::counter[in]);
    }
    if (MsgCoarsen::counter[in]) {
      CkPrintf ("%d Block::exit_() MsgCoarsen::counter = %lld != 0\n",
		CkMyPe(),MsgCoarsen::counter[in]);
    }
    if (MsgInitial::counter[in]) {
      CkPrintf ("%d Block::exit_() MsgInitial::counter = %lld != 0\n",
		CkMyPe(),MsgInitial::counter[in]);
    }
    if (MsgOutput::counter[in]) {
      CkPrintf ("%d Block::exit_() MsgOutput::counter = %lld != 0\n",
		CkMyPe(),MsgOutput::counter[in]);
    }
    if (MsgRefine::counter[in]) {
      CkPrintf ("%d Block::exit_() MsgRefine::counter = %lld != 0\n",
		CkMyPe(),MsgRefine::counter[in]);
    }
    if (MsgRefresh::counter[in]) {
      CkPrintf ("%d Block::exit_() MsgRefresh::counter = %lld != 0\n",
		CkMyPe(),MsgRefresh::counter[in]);
    }
  }

  cello::performance()->end();

  if (index_.is_root()) {
    proxy_main.p_exit(1);
  }
}
