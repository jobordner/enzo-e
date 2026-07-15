// See LICENSE_CELLO file for license and copyright information

/// @file      simulation_Simulation.cpp
/// @author    James Bordner (jobordner@ucsd.edu)
/// @date      2010-11-10
/// @brief     Implementation of the Simulation class

#include "cello.hpp"

#include "main.hpp"

#include "simulation.hpp"
#include "charm_simulation.hpp"

// Write curr/high/highest memory statistics for all processes each cycle
// #define TRACE_PROCESS_MEMORY

// #define DEBUG_SIMULATION
// #define DEBUG_MSG_REFINE

Simulation::Simulation
(
 const char *   parameter_file,
 int            n
 )
/// Initialize the Simulation object
:
#if defined(CELLO_DEBUG) || defined(CELLO_VERBOSE)
  fp_debug_(NULL),
#endif
  factory_(NULL),
  parameters_(&g_parameters),
  parameter_file_(parameter_file),
  rank_(0),
  cycle_watch_(-1),
  cycle_initial_(-1),
  state_(new State (0, 0.0, 0.0, false)),
  phase_(phase_unknown),
  config_(&g_config),
  problem_(NULL),
  timer_(),
  performance_(NULL),
  schedule_balance_(NULL),
  monitor_(NULL),
  hierarchy_(NULL),
  scalar_descr_long_double_(NULL),
  scalar_descr_double_(NULL),
  scalar_descr_int_(NULL),
  scalar_descr_long_long_(NULL),
  scalar_descr_sync_(NULL),
  scalar_descr_void_(NULL),
  scalar_descr_index_(NULL),
  field_descr_(NULL),
  particle_descr_(NULL),
  sync_init_block_count_(),
  sync_advance_state_(),
  sync_output_begin_(),
  sync_output_write_(),
  sync_restart_created_(),
  sync_restart_next_(),
  refresh_list_(),
  refresh_perf_count_(),
  refresh_perf_bytes_(),
  refresh_type_(RefreshType::Unknown),
  index_output_(-1),
  num_solver_iter_(),
  max_solver_iter_(),
  restart_directory_(),
  restart_num_files_(),
  restart_stream_file_list_(),
  ir_cycle_begin_(-1),
  ir_cycle_end_(-1),
  monitor_flag_(true),
  perf_method_base_rindex_(0),
  perf_solver_base_rindex_(0),
  perf_refresh_base_rindex_(0)

{
  for (int i=0; i<256; i++) dir_checkpoint_[i] = '\0';
#ifdef DEBUG_SIMULATION
  CkPrintf ("%d DEBUG_SIMULATION Simulation(parameter_file,n)\n",CkMyPe());
  fflush(stdout);
  char name[40];
  snprintf (name,sizeof(name),"parameters-%02d.text",CkMyPe());
  parameters_->write(name);
#endif
  
  debug_open();

  monitor_ = Monitor::instance();
#ifdef CELLO_DEBUG
  monitor_->set_mode(monitor_mode_all);
#else
  monitor_->set_mode(monitor_mode_root);
#endif

}

//----------------------------------------------------------------------

Simulation::Simulation()
  :
#if defined(CELLO_DEBUG) || defined(CELLO_VERBOSE)
  fp_debug_(NULL),
#endif
  factory_(NULL),
  parameters_(&g_parameters),
  parameter_file_(""),
  rank_(0),
  cycle_watch_(-1),
  cycle_initial_(-1),
  state_(new State (0, 0.0, 0.0, false)),
  phase_(phase_unknown),
  config_(&g_config),
  problem_(NULL),
  timer_(),
  performance_(NULL),
  schedule_balance_(NULL),
  monitor_(NULL),
  hierarchy_(NULL),
  scalar_descr_long_double_(NULL),
  scalar_descr_double_(NULL),
  scalar_descr_int_(NULL),
  scalar_descr_long_long_(NULL),
  scalar_descr_sync_(NULL),
  scalar_descr_void_(NULL),
  scalar_descr_index_(NULL),
  field_descr_(NULL),
  particle_descr_(NULL),
  sync_init_block_count_(),
  sync_advance_state_(),
  sync_output_begin_(),
  sync_output_write_(),
  sync_restart_created_(),
  sync_restart_next_(),
  refresh_list_(),
  refresh_perf_count_(),
  refresh_perf_bytes_(),
  refresh_type_(RefreshType::Unknown),
  index_output_(-1),
  num_solver_iter_(),
  max_solver_iter_(),
  restart_directory_(),
  restart_num_files_(),
  restart_stream_file_list_(),
  ir_cycle_begin_(-1),
  ir_cycle_end_(-1),
  monitor_flag_(true),
  perf_method_base_rindex_(0),
  perf_solver_base_rindex_(0),
  perf_refresh_base_rindex_(0)
{
  for (int i=0; i<256; i++) dir_checkpoint_[i] = '\0';
#ifdef DEBUG_SIMULATION
  CkPrintf ("%d DEBUG_SIMULATION Simulation()\n",CkMyPe());
  fflush(stdout);
#endif
  TRACE("Simulation()");
}

//----------------------------------------------------------------------

Simulation::Simulation (CkMigrateMessage *m)
  : CBase_Simulation(m),
#if defined(CELLO_DEBUG) || defined(CELLO_VERBOSE)
    fp_debug_(NULL),
#endif
    factory_(NULL),
    parameters_(&g_parameters),
    parameter_file_(""),
    rank_(0),
    cycle_watch_(-1),
    cycle_initial_(-1),
    state_(),
    phase_(phase_unknown),
    config_(&g_config),
    problem_(NULL),
    timer_(),
    performance_(NULL),
    schedule_balance_(NULL),
    monitor_(NULL),
    hierarchy_(NULL),
    scalar_descr_long_double_(NULL),
    scalar_descr_double_(NULL),
    scalar_descr_int_(NULL),
    scalar_descr_long_long_(NULL),
    scalar_descr_sync_(NULL),
    scalar_descr_void_(NULL),
    scalar_descr_index_(NULL),
    field_descr_(NULL),
    particle_descr_(NULL),
    sync_init_block_count_(),
    sync_advance_state_(),
    sync_output_begin_(),
    sync_output_write_(),
    sync_restart_created_(),
    sync_restart_next_(),
    refresh_list_(),
    refresh_perf_count_(),
    refresh_perf_bytes_(),
    refresh_type_(RefreshType::Unknown),
    index_output_(-1),
    num_solver_iter_(),
    max_solver_iter_(),
    restart_directory_(),
    restart_num_files_(),
    restart_stream_file_list_(),
    ir_cycle_begin_(-1),
    ir_cycle_end_(-1),
    monitor_flag_(true),
    perf_method_base_rindex_(0),
    perf_solver_base_rindex_(0),
    perf_refresh_base_rindex_(0)
{
  for (int i=0; i<256; i++) dir_checkpoint_[i] = '\0';
#ifdef DEBUG_SIMULATION
  CkPrintf ("%d DEBUG_SIMULATION Simulation(msg)\n",CkMyPe());
  fflush(stdout);
#endif
  TRACE("Simulation(CkMigrateMessage)");
}

//----------------------------------------------------------------------

Simulation::~Simulation()
{
  deallocate_();
}

//----------------------------------------------------------------------

void Simulation::pup (PUP::er &p)
{
#ifdef DEBUG_SIMULATION
  CkPrintf ("%d DEBUG_SIMULATION Simulation::pup()\n",CkMyPe());
  fflush(stdout);
#endif
  // NOTE: change this function whenever attributes change

  TRACEPUP;

  CBase_Simulation::pup(p);

  bool up = p.isUnpacking();

  if (up) debug_open();

  p | factory_; // PUP::able

  p | config_;

  p | parameter_file_;

  p | rank_; 
  p | cycle_watch_;
  p | cycle_initial_;
  p | *state_;
  p | phase_;

  p | problem_; // PUPable

  if (up) performance_ = new Performance;
  p | *performance_;

  if (up) monitor_ = Monitor::instance();
  p | *monitor_;

  if (up) hierarchy_ = new Hierarchy;
  p | *hierarchy_;

  if (up) scalar_descr_long_double_ = new ScalarDescr;
  p | *scalar_descr_long_double_;
  if (up) scalar_descr_double_ = new ScalarDescr;
  p | *scalar_descr_double_;
  if (up) scalar_descr_int_ = new ScalarDescr;
  p | *scalar_descr_int_;
  if (up) scalar_descr_long_long_ = new ScalarDescr;
  p | *scalar_descr_long_long_;
  if (up) scalar_descr_sync_ = new ScalarDescr;
  p | *scalar_descr_sync_;
  if (up) scalar_descr_void_ = new ScalarDescr;
  p | *scalar_descr_void_;
  if (up) scalar_descr_index_ = new ScalarDescr;
  p | *scalar_descr_index_;

  if (up) field_descr_ = new FieldDescr;
  p | *field_descr_;

  if (up) particle_descr_ = new ParticleDescr;
  p | *particle_descr_;

  if (up && (phase_ == phase_restart)) {
    monitor_->header();
    monitor_->print ("Simulation","restarting");
  }

  p | sync_init_block_count_;
  p | sync_advance_state_;
  p | sync_output_begin_;
  p | sync_output_write_;
  p | sync_restart_created_;
  p | sync_restart_next_;

  if (up) sync_advance_state_.set_stop(0);
  if (up) sync_output_begin_.set_stop(0);
  if (up) sync_output_write_.set_stop(0);

  p | schedule_balance_;

  p | refresh_list_;
  p | refresh_name_;
  p | refresh_perf_count_;
  p | refresh_perf_bytes_;
  p | refresh_type_;

  PUParray(p,dir_checkpoint_,256);

  ASSERT1("Simulation::pup()",
	  "msg_refine_map_ is assumed to be empty but has size %lu",
	  msg_refine_map_.size(),
	  (msg_refine_map_.size() == 0));

  //  p | msg_refine_map_;
  p | index_output_;
  p | num_solver_iter_;
  p | max_solver_iter_;
  p | restart_directory_;
  p | restart_num_files_;
  p | ir_cycle_begin_;
  p | ir_cycle_end_;
  p | monitor_flag_;

  p | perf_method_base_rindex_;
  p | perf_solver_base_rindex_;
  p | perf_refresh_base_rindex_;
}

//----------------------------------------------------------------------

void Simulation::finalize() throw()
{
  TRACE0;

  PERF_STOP(iperf_simulation);

  performance_->end();

}

//----------------------------------------------------------------------

void Simulation::p_refine_create_block(MsgRefine * msg)
{ refine_create_block(msg); }

void Simulation::refine_create_block(MsgRefine * msg)
{
  Index index = msg->index();
  if (msg_refine_map_[index] != NULL) {
    int v3[3];
    index.values(v3);
    ASSERT3 ("Simulation::p_refine_create_block",
	    "index %08x %08x %08x is already in the msg_refine mapping",
	    v3[0],v3[1],v3[2],
	    (msg == NULL));
  }

  msg_refine_map_[index] = msg;

  cello::block_array()[index].insert(MsgType::msg_refine);
}

//======================================================================

void Simulation::initialize_simulation_() throw()
{

#ifdef DEBUG_SIMULATION
  CkPrintf ("%d DEBUG_SIMULATION Simulation::initialize_simulation_()\n",CkMyPe());
  fflush(stdout);
#endif

  rank_ = config_->mesh_root_rank;

  ASSERT ("Simulation::initialize_simulation_()",
	  "Parameter 'Mesh:root_rank' must be specified",
	  rank_ != 0);

  ASSERT ("Simulation::initialize_simulation_()",
	  "Parameter 'Mesh:root_rank' must be 1, 2, or 3",
	  (1 <= rank_) && (rank_ <= 3));

  state_->init(config_->initial_cycle,
              config_->initial_time,
              0.0, false);

  const std::string type = cello::config()->timestep_type;
  const std::string level_type = cello::config()->timestep_level_type;
  const int max_level = cello::max_level();
  state_->set_type ( type, max_level );
  state_->set_level_type ( level_type, max_level );
  cycle_watch_   = config_->initial_cycle - 1;
  cycle_initial_ = config_->initial_cycle;

  if (config_->timestep_refresh_type == "casual") {

    refresh_type_ = RefreshType::Casual;

  } else if (config_->timestep_refresh_type == "eager") {

    refresh_type_ = RefreshType::Eager;

  } else {

    ERROR1 ("Simulation::initialize_simulation_()", 
            "Unrecognized timestep_refresh_type parameter value %s "
            "(must be \"casual\" or \"eager\")",
            config_->timestep_refresh_type.c_str());

  }
}

//----------------------------------------------------------------------

void Simulation::initialize_memory_() throw()
{
  Memory * memory = Memory::instance();
  if (memory) {
    memory->set_active(config_->memory_active);
    memory->set_warning_mb (config_->memory_warning_mb);
    memory->set_limit_gb (config_->memory_limit_gb);
  }
}
//----------------------------------------------------------------------

void Simulation::initialize_performance_() throw()
{

  performance_ = new Performance (config_);

  Performance * p = performance_;
  p->new_region(iperf_unknown,            "unknown");
  p->new_region(iperf_simulation,         "simulation");
  p->new_region(iperf_cycle,              "cycle");
  p->new_region(iperf_initial,            "initial");

  const bool in_charm = true;
  p->new_region(iperf_adapt,                 "adapt");
  p->new_region(iperf_adapt_post,            "adapt_post",in_charm);
  p->new_region(iperf_adapt_enter,           "adapt_enter");
  p->new_region(iperf_adapt_enter_post,      "adapt_enter_post",in_charm);
  p->new_region(iperf_adapt_end,             "adapt_end");
  p->new_region(iperf_adapt_end_post,        "adapt_end_post",in_charm);
  p->new_region(iperf_adapt_update,          "adapt_update");
  p->new_region(iperf_adapt_update_post,     "adapt_update_post",in_charm);
  p->new_region(iperf_adapt_next,            "adapt_next");
  p->new_region(iperf_adapt_next_post,       "adapt_next_post",in_charm);
  p->new_region(iperf_adapt_called,          "adapt_called");
  p->new_region(iperf_adapt_called_post,     "adapt_called_post",in_charm);
  p->new_region(iperf_adapt_exit,            "adapt_exit");
  p->new_region(iperf_adapt_exit_post,       "adapt_exit_post",in_charm);
  p->new_region(iperf_adapt_delete,          "adapt_delete");
  p->new_region(iperf_adapt_delete_post,     "adapt_delete_post",in_charm);
  p->new_region(iperf_adapt_recv_level,      "adapt_recv_level");
  p->new_region(iperf_adapt_recv_level_post, "adapt_recv_level_post",in_charm);
  p->new_region(iperf_adapt_recv_child,      "adapt_recv_child");
  p->new_region(iperf_adapt_recv_child_post, "adapt_recv_child_post",in_charm);
  p->new_region(iperf_refresh,               "refresh");
  p->new_region(iperf_refresh_post,          "refresh_post",in_charm);
  p->new_region(iperf_refresh_recv,          "refresh_recv");
  p->new_region(iperf_refresh_recv_post,     "refresh_recv_post",in_charm);
  p->new_region(iperf_refresh_exit,          "refresh_exit");
  p->new_region(iperf_refresh_exit_post,     "refresh_exit_post",in_charm);
  p->new_region(iperf_refresh_child,         "refresh_child");
  p->new_region(iperf_refresh_child_post,    "refresh_child_post",in_charm);

  p->new_region(iperf_reduce,                "reduce");
  p->new_region(iperf_reduce_adapt,          "reduce_adapt");
  p->new_region(iperf_reduce_charm,          "reduce_charm");
  p->new_region(iperf_reduce_initialize,     "reduce_initialize");
  p->new_region(iperf_reduce_method_balance, "reduce_method_balance");
  p->new_region(iperf_reduce_method_check,   "reduce_method_check");
  p->new_region(iperf_reduce_method_debug,   "reduce_method_debug");
  p->new_region(iperf_reduce_method_flux_correct,"reduce_method_flux_correct");
  p->new_region(iperf_reduce_method_inference, "reduce_method_inference");
  p->new_region(iperf_reduce_method_m1_closure,"reduce_method_m1_closure");
  p->new_region(iperf_reduce_method_order_hilbert,"reduce_method_order_hilbert");
  p->new_region(iperf_reduce_method_order_morton,"reduce_method_order_morton");
  p->new_region(iperf_reduce_method_output,  "reduce_method_output");
  p->new_region(iperf_reduce_method_turbulence,"reduce_method_turbulence");
  p->new_region(iperf_reduce_output,         "reduce_output");
  p->new_region(iperf_reduce_restart,        "reduce_restart");
  p->new_region(iperf_reduce_balance,        "reduce_balance");
  p->new_region(iperf_reduce_simulation,     "reduce_simulation");
  p->new_region(iperf_reduce_solver_bicgstab,"reduce_solver_bicgstab");
  p->new_region(iperf_reduce_solver_cg,      "reduce_solver_cg");
  p->new_region(iperf_reduce_solver_dd,      "reduce_solver_dd");
  p->new_region(iperf_reduce_solver_mg0,     "reduce_solver_mg0");
  p->new_region(iperf_reduce_stopping,       "reduce_stopping");

#ifdef CONFIG_SMP_MODE
  p->new_region(iperf_smp,                     "smp");
  p->new_region(iperf_smp_field_face,          "smp_field_face");
  p->new_region(iperf_smp_hierarchy,           "smp_hierarchy");
  p->new_region(iperf_smp_initial_music,     "smp_initial_music");
  p->new_region(iperf_smp_initial_value,     "smp_initial_value");
  p->new_region(iperf_smp_method_close_files,"smp_method_close_files");
  p->new_region(iperf_smp_solver_bcg,        "smp_solver_bcg");
#endif
  p->new_region(iperf_method,                "method");
  p->new_region(iperf_solver,                "solver");
  p->new_region(iperf_control,               "control");
  p->new_region(iperf_output,                "output");
  p->new_region(iperf_balance,               "balance");
  p->new_region(iperf_stopping,              "stopping");
  p->new_region(iperf_block,                 "block");
  p->new_region(iperf_exit,                  "exit");

#ifdef CONFIG_USE_GRACKLE
  p->new_region(iperf_grackle,            "grackle");
#endif

  // Initialize Performance monitoring

  const Problem * problem = cello::problem();
  // add Method performance regions
  perf_method_base_rindex_ = p->num_regions();
  for (int i=0; i<problem->num_methods(); i++) {
    Method * method = cello::method(i);
    std::string region_name = std::string("method_") + method->name();
    p->new_region(perf_method_base_rindex_ + i, region_name);
    method->set_perf_index(perf_method_base_rindex_ + i);
  }
  // add Solver performance regions
  perf_solver_base_rindex_ = p->num_regions();
  for (int i=0; i<problem->num_solvers(); i++) {
    Solver * solver = cello::solver(i);
    std::string region_name = std::string("solver_") + solver->name();
    p->new_region(perf_solver_base_rindex_ + i, region_name);
    solver->set_perf_index(perf_solver_base_rindex_ + i);
  }
  // add Refresh performance regions
  perf_refresh_base_rindex_ = p->num_regions();
  for (size_t i=0; i<refresh_list_.size(); i++) {
    std::string region_name = std::string("refresh_") + refresh_name_[i];
    p->new_region(perf_refresh_base_rindex_ + i, region_name);
  }

  timer_.start();

#ifdef CONFIG_USE_PAPI
  for (size_t i=0; i<config_->performance_papi_counters.size(); i++) {
    p->new_counter(counter_type_papi,
		   config_->performance_papi_counters[i]);
  }
#endif

  p->begin();

  PERF_START(iperf_simulation);

}

//----------------------------------------------------------------------

void Simulation::initialize_config_() throw()
{
  TRACE("BEGIN Simulation::initialize_config_");
  TRACE("END   Simulation::initialize_config_");
}

//----------------------------------------------------------------------

void Simulation::initialize_monitor_() throw()
{

  monitor_->set_include_proc(config_->monitor_proc);
  monitor_->set_include_time(config_->monitor_time);
  monitor_->set_verbose     (config_->monitor_verbose);

  int index = config_->monitor_schedule_index;

  for (auto component : config_->monitor_mute_list) {
    monitor_->mute_component_(component);
  }
  for (auto component : config_->monitor_only_list) {
    monitor_->only_component_(component);
  }
  Schedule * schedule = (index == -1) ? nullptr : Schedule::create
    ( config_->schedule_var[index],
      config_->schedule_type[index],
      config_->schedule_start[index],
      config_->schedule_stop[index],
      config_->schedule_step[index],
      config_->schedule_list[index]);
  monitor_->set_schedule_(schedule);

}

//----------------------------------------------------------------------

void Simulation::initialize_data_descr_() throw()
{
  scalar_descr_long_double_ = new ScalarDescr;
  scalar_descr_double_      = new ScalarDescr;
  scalar_descr_int_         = new ScalarDescr;
  scalar_descr_long_long_   = new ScalarDescr;
  scalar_descr_sync_        = new ScalarDescr;
  scalar_descr_void_        = new ScalarDescr;
  scalar_descr_index_       = new ScalarDescr;

  //--------------------------------------------------
  // parameter: Field : list
  //--------------------------------------------------

  field_descr_ = new FieldDescr;

  // Add data fields

  for (size_t i=0; i<config_->field_list.size(); i++) {
    field_descr_->insert_permanent (config_->field_list[i]);
  }

  // Define default ghost zone depth for all fields, default value of 1

  int gx = config_->field_ghost_depth[0];
  int gy = config_->field_ghost_depth[1];
  int gz = config_->field_ghost_depth[2];

  field_descr_->set_default_ghost_depth (gx,gy,gz);

  // Default precision

  for (int i=0; i<field_descr_->field_count(); i++) {
    field_descr_->set_precision(i,config_->field_precision);
  }

  //--------------------------------------------------
  // parameter: Field : alignment
  //--------------------------------------------------

  int alignment = config_->field_alignment;

  ASSERT1 ("Simulation::initialize_data_descr_",
	  "Illegal Field:alignment parameter value %d",
	   alignment,
	   1 <= alignment );

  field_descr_->set_alignment (alignment);

  field_descr_->set_padding (config_->field_padding);

  field_descr_->set_history (config_->field_history);

  for (int i=0; i<field_descr_->field_count(); i++) {

    std::string field_name = field_descr_->field_name(i);

    const int cx = config_->field_centering[0][i];
    const int cy = config_->field_centering[1][i];
    const int cz = config_->field_centering[2][i];

    field_descr_->set_centering(i,cx,cy,cz);

  }

  // field groups

  int num_fields = config_->field_group_list.size();
  for (int index_field=0; index_field<num_fields; index_field++) {
    std::string field = config_->field_list[index_field];
    int num_groups = config_->field_group_list[index_field].size();

    for (int index_group=0; index_group<num_groups; index_group++) {
      std::string group = config_->field_group_list[index_field][index_group];
      field_descr_->groups()->add(field,group);
    }
  }

  //--------------------------------------------------
  // parameter: Particle : list
  //--------------------------------------------------

  particle_descr_ = new ParticleDescr;

  // Set particle batch size
  particle_descr_->set_batch_size(config_->particle_batch_size);

  // Add particle types

  // ... first map attribute scalar type name to type_enum int
  std::map<std::string,int> type_val;
  for (int i=0; i<NUM_TYPES; i++) {
    type_val[cello::type_name[i]] = i;
  }
#ifdef CONFIG_PRECISION_SINGLE	
  type_val["default"] = type_float;
#endif
#ifdef CONFIG_PRECISION_DOUBLE
  type_val["default"] = type_double;
#endif
  for (size_t it=0; it<config_->particle_list.size(); it++) {

    particle_descr_->new_type (config_->particle_list[it]);

    // Add particle constants
    int nc = config_->particle_constant_name[it].size();
    for (int ic=0; ic<nc; ic++) {
      std::string name = config_->particle_constant_name[it][ic];
      int         type = type_val[config_->particle_constant_type[it][ic]];
      ASSERT3 ("Simulation::initialize_data_descr_()",
	       "Unknown Particle type \"%s\" constant \"%s\" "
	       "has unknown type \"%s\"",
	       config_->particle_list[it].c_str(),
	       name.c_str(),
	       config_->particle_attribute_type[it][ic].c_str(),
	       cello::type_is_valid(type));
      particle_descr_->new_constant(it,name,type);
      union {
	char * c;
	long long * ill;
	float * f4;
	double * f8;
	long double * f16;
	int8_t * i8;
	int16_t * i16;
	int32_t * i32;
	int64_t * i64;
      };
      c = particle_descr_->constant_value(it,ic);
      if (type == type_default) type = default_type;
      switch (type) {
      case type_single:     *f4 = config_->particle_constant_value[it][ic];
	break;
      case type_double:     *f8 = config_->particle_constant_value[it][ic];
	break;
      case type_quadruple:  *f16 = config_->particle_constant_value[it][ic];
	break;
      case type_int8:       *i8 = config_->particle_constant_value[it][ic];
	break;
      case type_int16:      *i16 = config_->particle_constant_value[it][ic];
	break;
      case type_int32:      *i32 = config_->particle_constant_value[it][ic];
	break;
      case type_int64:      *i64 = config_->particle_constant_value[it][ic];
	break;
      default:
	ERROR3 ("Simulation::initialize_data_descr_()",
		"Unrecognized type %d for particle constant %s in type %s",
		type,name.c_str(),config_->particle_list[it].c_str());
	break;
      }
    }

    // Add particle attributes
    int na = config_->particle_attribute_name[it].size();
    for (int ia=0; ia<na; ia++) {
      std::string name = config_->particle_attribute_name[it][ia];
      int type         = type_val[config_->particle_attribute_type[it][ia]];
      ASSERT3 ("Simulation::initialize_data_descr_()",
	       "Unknown Particle type \"%s\" attribute \"%s\" has unknown type \"%s\"",
	       config_->particle_list[it].c_str(),
	       name.c_str(),
	       config_->particle_attribute_type[it][ia].c_str(),
	       cello::type_is_valid(type));
      particle_descr_->new_attribute(it,name,type);
    }

    // position and velocity attributes
    particle_descr_->set_position
      (it,
       config_->particle_attribute_position[0][it],
       config_->particle_attribute_position[1][it],
       config_->particle_attribute_position[2][it]);
    particle_descr_->set_velocity
      (it,
       config_->particle_attribute_velocity[0][it],
       config_->particle_attribute_velocity[1][it],
       config_->particle_attribute_velocity[2][it]);
  }

  // particle groups

  int num_particles = config_->particle_group_list.size();
  for (int index_particle=0; index_particle<num_particles; index_particle++) {
    std::string particle = config_->particle_list[index_particle];
    int num_groups = config_->particle_group_list[index_particle].size();

    for (int index_group=0; index_group<num_groups; index_group++) {
      std::string group = config_->particle_group_list[index_particle][index_group];
      particle_descr_->groups()->add(particle,group);
    }
  }

}
//----------------------------------------------------------------------

void Simulation::initialize_hierarchy_() throw()
{
#ifdef DEBUG_SIMULATION
  CkPrintf ("%d DEBUG_SIMULATION Simulation::initialize_hierarchy_()\n",CkMyPe());
  fflush(stdout);
#endif

  ASSERT("Simulation::initialize_hierarchy_",
	 "data must be initialized before hierarchy",
	 field_descr_ != NULL);

  //----------------------------------------------------------------------
  // Create and initialize Hierarchy
  //----------------------------------------------------------------------

  const int refinement = 2;

  hierarchy_ = factory()->create_hierarchy
    (refinement,
     cello::min_level(),
     cello::max_level());

  hierarchy_->set_lower
    (config_->domain_lower[0],
     config_->domain_lower[1],
     config_->domain_lower[2]);
  hierarchy_->set_upper
    (config_->domain_upper[0],
     config_->domain_upper[1],
     config_->domain_upper[2]);

  //----------------------------------------------------------------------
  // Create and initialize root Patch in Hierarchy
  //----------------------------------------------------------------------

  //--------------------------------------------------
  // parameter: Mesh : root_size
  // parameter: Mesh : root_blocks
  //--------------------------------------------------

  hierarchy_->set_root_size(config_->mesh_root_size[0],
			    config_->mesh_root_size[1],
			    config_->mesh_root_size[2]);

  hierarchy_->set_blocking(config_->mesh_root_blocks[0],
			   config_->mesh_root_blocks[1],
			   config_->mesh_root_blocks[2]);

  bool lp3[3] = { false, false, false };
  auto & root_blocks = config_->mesh_root_blocks;

  for (size_t k = 0; k < cello::num_boundary(); k++) {
    cello::boundary(k)->periodicity(lp3);
  }

  int p3[3] = {lp3[0] ? root_blocks[0] : 0,
               lp3[1] ? root_blocks[1] : 0,
               lp3[2] ? root_blocks[2] : 0 };
  hierarchy_->set_periodicity (p3[0],p3[1],p3[2]);
  hierarchy_->set_refined_regions_lower(config_->refined_regions_lower);
  hierarchy_->set_refined_regions_upper(config_->refined_regions_upper);
}

//----------------------------------------------------------------------

void Simulation::initialize_balance_() throw()
{
  int index = config_->balance_schedule_index;

  schedule_balance_ = (index == -1) ? NULL : Schedule::create
    ( config_->schedule_var[index],
      config_->schedule_type[index],
      config_->schedule_start[index],
      config_->schedule_stop[index],
      config_->schedule_step[index],
      config_->schedule_list[index]);

}

//----------------------------------------------------------------------

void Simulation::initialize_refresh_() throw()
{
  const int ghost_depth = 4;
  const int min_face_rank = 0;

  ir_cycle_begin_ = cello::simulation()->new_register_refresh
    (Refresh::create
     (ghost_depth,min_face_rank, neighbor_leaf, sync_neighbor, 0),
     "cycle_begin");

  ir_cycle_end_ = cello::simulation()->new_register_refresh
    (Refresh::create
     (ghost_depth,min_face_rank, neighbor_leaf, sync_neighbor, 0),"cycle_end");
}

//----------------------------------------------------------------------

void Simulation::initialize_block_array_() throw()
{
  if (CkMyPe() == 0) {
    // Set sync counter for initial blocks;
    int num_initial_blocks = initial_block_count();
    sync_init_block_count_.set_stop(num_initial_blocks);
  }

  // Create the root-level blocks for level = 0
  hierarchy_->create_block_array ();

  // Create the "sub-root" blocks if min_level < 0
  if (hierarchy_->min_level() < 0) {
    hierarchy_->create_subblock_array ();
  }
}

//----------------------------------------------------------------------

int Simulation::initial_block_count() throw() {
  // TODO: Generalise this to arbitrary rank (assumes rank = 3)
  // Count blocks on level 0;
  int nx, ny, nz;
  hierarchy_->root_blocks(&nx, &ny, &nz);
  int block_count = nx * ny * nz;

  // Count blocks below level 0.
  if (hierarchy_->min_level() < 0) {
    while (nx > 0) {
      nx >>= 1; ny >>= 1; nz >>= 1;
      block_count += nx * ny * nz;
    }
  }

  // Count blocks above level 0.
  int lower[3], upper[3];
  int num_refined_levels = hierarchy_->refined_region_lower().size();
  for (int l=0; l<num_refined_levels; l++) {
    hierarchy_->refined_region_lower(lower, l);
    hierarchy_->refined_region_upper(upper, l);
    nx = upper[0] - lower[0];
    ny = upper[1] - lower[1];
    nz = upper[2] - lower[2];

    block_count += 8 * nx * ny * nz;
  }

  return block_count;
}

//----------------------------------------------------------------------

void Simulation::p_initial_block_created() throw() {
  if (sync_init_block_count_.next()) {
    hierarchy_->block_array().doneInserting();
    hierarchy_->block_array().p_initial_begin();
  }
}

//----------------------------------------------------------------------

void Simulation::p_set_block_array(CProxy_Block block_array)
{
  if (CkMyPe() != 0) hierarchy_->set_block_array(block_array);
  CkCallback callback
    (CkIndex_Simulation::r_initialize_block_array(NULL), thisProxy);

  // --------------------------------------------------
  contribute(0,0,CkReduction::concat,callback);
  // --------------------------------------------------
}
//----------------------------------------------------------------------

void Simulation::deallocate_() throw()
{
  delete factory_;       factory_     = 0;
  delete parameters_;    parameters_  = 0;
  delete hierarchy_;     hierarchy_ = 0;
  delete field_descr_;   field_descr_ = 0;
  delete performance_;   performance_ = 0;
}

//----------------------------------------------------------------------

const Factory * Simulation::factory() const throw()
{
  TRACE("Simulation::factory()");
  if (factory_ == NULL) factory_ = new Factory;
  return factory_;
}

//======================================================================

void Simulation::update_state(int cycle, double time, double dt, double stop)
{

  state_->init(cycle,time,dt,(stop != 0));
  monitor_->update_state_(cycle,time);
}

//----------------------------------------------------------------------

void Simulation::p_initialize_state(MsgState * msg)
{
  msg->update(this);
  cycle_initial_ = state_->cycle();
  delete msg;
}

//======================================================================

void Simulation::data_insert_block(Block * block)
{

#ifdef CELLO_DEBUG
  PARALLEL_PRINTF ("%d: ++sync_output_begin_ %d %lu\n",
		   CkMyPe(),sync_output_begin_.stop(),hierarchy_->num_blocks());
#endif
  if (hierarchy_) {
    hierarchy_->insert_block(block);
    hierarchy_->increment_block_count(1,block->level());
  }
  ++sync_advance_state_;
  ++sync_output_begin_;
  ++sync_output_write_;
}

//----------------------------------------------------------------------

void Simulation::data_delete_block(Block * block)
{
  if (hierarchy_) {
    hierarchy_->delete_block(block);
    hierarchy_->increment_block_count(-1,block->level());
  }
  --sync_advance_state_;
  --sync_output_begin_;
  --sync_output_write_;
}

//----------------------------------------------------------------------

void Simulation::data_insert_particles(int64_t count)
{
  if (hierarchy_) hierarchy_->increment_particle_count(count);
}

//----------------------------------------------------------------------

void Simulation::data_delete_particles(int64_t count)
{
  if (hierarchy_) hierarchy_->increment_particle_count(-count);
}

//----------------------------------------------------------------------

void Simulation::monitor_output()
{
  // Only run if called by first block on pe 0
  // monitor_flag_ reset for next cycle in stopping_exit_()

  if (monitor_flag_ && (CkMyPe() == 0)) {
    monitor_flag_ = false;
  } else return;

  Monitor * monitor = this->monitor();

  monitor-> print("", "-------------------------------------");

  const bool in_p = monitor->include_proc();
  const bool in_t = monitor->include_time();
  monitor->set_include_proc(true);
  monitor->set_include_time(true);
  monitor-> print("Simulation", "cycle %04d",      state_->cycle());
  monitor->set_include_proc(in_p);
  monitor->set_include_time(in_t);

  monitor-> print("Simulation", "time-sim %15.12e",state_->time());
  monitor-> print("Simulation", "dt %15.12e",      state_->dt());
  const int level_lower = std::max(cello::level_root(),0);
  const int level_upper = cello::level_top();
  const int level_max = cello::max_level();
  if (state_->state_type() == State::Type::Level) {
    std::string active_levels{""};
    for (int level=level_lower; level<=level_upper; level++) {
      monitor-> print("Simulation", "cycle-level %d %04d",
                        level,state_->cycle(level));
    }
    for (int level=level_lower; level<=level_upper; level++) {
      monitor-> print("Simulation", "time-level %d %15.12e",
                        level,state_->time(level));
    }
    for (int level=level_lower; level<=level_upper; level++) {
      monitor-> print("Simulation", "dt-level %d %15.12e",
                        level,state_->dt_level(level));
    }
    for (int level=0; level<=level_max; level++) {
      static const char digit[] = "0123456789";
      active_levels.push_back(state_->is_active(level) ? digit[level%10] : ' ');
    }
    monitor-> print("Simulation", "active levels [ %s ]", active_levels.c_str());
  }

  thisProxy.p_monitor_performance();
}

//----------------------------------------------------------------------

void Simulation::monitor_performance()
{
  // Global performance metrics
  // [0] num_sum
  // [1] num_max
  // [...] metrics

  std::vector<long long> counters_reduce_vector;

  counters_reduce_vector.push_back(0); // place holder for num_sum
  counters_reduce_vector.push_back(0); // place holder for num_max

  // summed metrics

  const int in = cello::index_static();

  counters_reduce_vector.push_back( MsgCoarsen::counter[in] );
  counters_reduce_vector.push_back( MsgRefine::counter[in] );
  counters_reduce_vector.push_back( MsgRefresh::counter[in] );
  counters_reduce_vector.push_back( MsgOrder::counter[in] );
  counters_reduce_vector.push_back( DataMsg::counter[in] );
  counters_reduce_vector.push_back( FieldFace::counter[in] );
  counters_reduce_vector.push_back( hierarchy_->num_particles() );

  // Refresh count, fields, particles, bytes sent/received per refresh object
  for (size_t i=0; i<refresh_list_.size(); i++) {
    counters_reduce_vector.push_back (refresh_perf_count_[i]);
    counters_reduce_vector.push_back (refresh_perf_bytes_[i]);
  }

  const int num_solver = problem()->num_solvers();
  for (int i=0; i<num_solver; i++) {
    counters_reduce_vector.push_back
      ( cello::simulation()->get_solver_num_iter(i) );
  }

  const int min_level = hierarchy_->min_level();

  int num_blocks_total = 0;
  for (int i=min_level; i<=hierarchy_->max_level(); i++) {
    num_blocks_total +=  hierarchy_->num_blocks(i);
    counters_reduce_vector.push_back( hierarchy_->num_blocks(i) );
  }
  counters_reduce_vector.push_back( num_blocks_total );

  // performance region counters
  const int nc =  performance_->num_counters();
  const int nr  = performance_->num_regions();
  std::vector<long long> counters_region;
  counters_region.resize(nc);
  for (int ir = 0; ir < nr; ir++) {
    performance_->region_counters(ir,counters_region.data());
    for (int ic = 0; ic < nc; ic++) {
      counters_reduce_vector.push_back( counters_region[ic] );
    }
  }

  // Block neighbor metrics
  counters_reduce_vector.push_back( hierarchy_->num_neighbors_local() );
  counters_reduce_vector.push_back( hierarchy_->num_neighbors_total() );
  hierarchy_->clear_num_neighbors();

  // Save number of summed metrics
  counters_reduce_vector[0] = counters_reduce_vector.size() - 2;

  // maximum metrics

  counters_reduce_vector.push_back( num_blocks_total );
  counters_reduce_vector.push_back( hierarchy_->num_particles() );
  counters_reduce_vector.push_back( Hierarchy::num_blocks_node );
  counters_reduce_vector.push_back( Hierarchy::num_particles_node );

  for (int i=0; i<num_solver; i++) {
    counters_reduce_vector.push_back
      ( cello::simulation()->get_solver_max_iter(i) );
  }

  // Save number of maxed metrics
  counters_reduce_vector[1] =
    counters_reduce_vector.size() - counters_reduce_vector[0] - 2;

  // --------------------------------------------------

  PERF_REDUCE_START(iperf_reduce_simulation);
  contribute
    (counters_reduce_vector.size()*sizeof(long long),
     counters_reduce_vector.data(),
     r_reduce_performance_type,
     CkCallback (CkIndex_Simulation::r_monitor_performance_reduce(NULL),
		 thisProxy));
  // --------------------------------------------------

}

//----------------------------------------------------------------------

void Simulation::r_monitor_performance_reduce(CkReductionMsg * msg)
{
  PERF_REDUCE_STOP(iperf_reduce_simulation);

  const Monitor * monitor = this->monitor();

  long long * counters_reduce = (long long *)msg->getData();

  int index_region_cycle = performance_->region_index("cycle");

  int m = 0;
  const int num_sum = counters_reduce[m++];
  const int num_max = counters_reduce[m++];

  monitor->print("perf:counter","msg-coarsen %lld", counters_reduce[m++]);
  monitor->print("perf:counter","msg-refine %lld",  counters_reduce[m++]);
  monitor->print("perf:counter","msg-refresh %lld", counters_reduce[m++]);
  monitor->print("perf:counter","msg-order %lld",   counters_reduce[m++]);
  monitor->print("perf:counter","data-msg %lld",    counters_reduce[m++]);
  monitor->print("perf:counter","field-face %lld",  counters_reduce[m++]);

  const int num_particles = counters_reduce[m++];
  monitor->print("perf:data","num-particles total %lld",num_particles);

  for (size_t i=0; i<refresh_list_.size(); i++) {
    long long value;
    if ((value = counters_reduce[m++]))
      monitor->print ("perf:refresh","refresh-count %s %lld",refresh_name_[i].c_str(),value);
    if ((value = counters_reduce[m++]))
      monitor->print
        ("perf:refresh","refresh-bytes %s %lld",refresh_name_[i].c_str(),value);
  }
  // Solver iterations

  const int num_solver = problem()->num_solvers();
  for (int i=0; i<num_solver; i++) {
    const long long num_solver_iter = counters_reduce[m++];
    if (num_solver_iter > 0) {
      monitor->print ("perf:solver","num-%s-iter %lld",
                      problem()->solver(i)->name().c_str(),
                      num_solver_iter);
    }
  }


  // compute total blocks and leaf blocks
  long long num_total_blocks = 0;
  long long num_leaf_blocks = 0;
  for (int i=hierarchy_->min_level(); i<=hierarchy_->max_level(); i++) {
    const long long num_blocks_level = counters_reduce[m++];
    hierarchy()->set_blocks_global(i,num_blocks_level);

    if (i>=0) {
      monitor->print("perf:mesh","blocks-level_%d %lld",
                     i,num_blocks_level);
    }

    num_total_blocks += num_blocks_level;
    // compute leaf blocks given number of blocks per level
    // (NOTE: num_blocks_level (i>0) is evenly divisible by num_children
    if (i==0) {
      num_leaf_blocks = num_blocks_level;
    } else if (i>0) {
      num_leaf_blocks +=
        (num_blocks_level - num_blocks_level/cello::num_children());
    }
  }

  monitor->print ("perf:mesh","leaf-blocks %lld",  num_leaf_blocks);
  monitor->print ("perf:mesh","total-blocks %lld", num_total_blocks);

  const long long num_blocks_total   = counters_reduce[m++];

  if (num_total_blocks != num_blocks_total) {
    WARNING2 ("Simulation::r_monitor_performance_reduce()",
              "num_blocks_total %lld does not match computed value %lld",
              num_total_blocks,num_blocks_total);
  }

  const int num_regions  = performance_->num_regions();
  const int num_counters =  performance_->num_counters();

  for (int ir = 0; ir < num_regions; ir++) {
    for (int ic = 0; ic < num_counters; ic++, m++) {
      bool do_print =
        (ir != iperf_unknown) &&
        ((performance_->counter_type(ic) != PerfCounterType::Absolute) ||
         (ir == index_region_cycle)) &&
        (counters_reduce[m] != 0);
      if (do_print) {
        monitor->print("perf:region","%s %s %lld",
                       performance_->region_name(ir).c_str(),
                       performance_->counter_name(ic).c_str(),
                       counters_reduce[m]);
        const int multiplicity = performance_->region_multiplicity(ir);
        if (! (0 <= multiplicity && multiplicity <= 1)) {
          CkPrintf ("%d WARNING: perf:region %s %d multiplicity %d\n",
                    CkMyPe(),performance_->region_name(ir).c_str(),ir,
                    multiplicity);
        }
      }
    }
  }

  const long long num_neighbors_local = counters_reduce[m++];
  const long long num_neighbors_total = counters_reduce[m++];

  const long long max_proc_blocks    = counters_reduce[m++];
  const long long max_proc_particles = counters_reduce[m++];

  // Block load balance metrics

  //    block process load-balance
  const double avg_proc_blocks = 1.0*num_blocks_total/CkNumPes();
  monitor->print ("perf:balance","max-blocks-proc %lld",  max_proc_blocks);
  monitor->print ("perf:balance","avg-blocks-proc %f",  avg_proc_blocks);
  monitor->print ("perf:balance","eff-blocks-core %f",
                  avg_proc_blocks / max_proc_blocks);

  const long long max_node_blocks    = counters_reduce[m++];
  const long long max_node_particles = counters_reduce[m++];

  //    block node load-balance
  if (CkNumPes() != CkNumNodes()) {
    const double avg_node_blocks = 1.0*num_blocks_total/CkNumNodes();
    monitor->print ("perf:balance","max-blocks-node %lld",  max_node_blocks);
    monitor->print ("perf:balance","avg-blocks-node %f",  avg_node_blocks);
    monitor->print ("perf:balance","eff-blocks-node %f",
                    avg_node_blocks / max_node_blocks);
  }

  // Particle load-balance metrics
  if (num_particles > 0) {
    //    particle process load-balance
    const double avg_proc_particles = 1.0*num_particles/CkNumPes();
    monitor->print ("perf:balance","max-particles-proc %lld",
                    max_proc_particles);
    monitor->print ("perf:balance","avg-particles-proc %f",
                    avg_proc_particles);
    monitor->print ("perf:balance","eff-particles-core %f",
                    avg_proc_particles / max_proc_particles );

    //    particle node load-balance
    if (CkNumPes() != CkNumNodes()) {
      const double avg_node_particles = 1.0*num_particles/CkNumNodes();
      monitor->print ("perf:balance","max-particles-node %lld",
                      max_node_particles);
      monitor->print ("perf:balance","avg-particles-node %f",
                      avg_node_particles);
      monitor->print ("perf:balance","eff-particles-node %f",
                      avg_node_particles / max_node_particles );
    }
  }

  // Number of block neighbors local and total
  monitor->print ("perf:balance","num_neighbors_local %lld",
                  num_neighbors_local);
  monitor->print ("perf:balance","num-neighbors-total %lld",
                  num_neighbors_total);
  monitor->print ("perf:balance","eff-neighbors-local %f",
                  1.0*num_neighbors_local/num_neighbors_total);

  // Solver iterations

  for (int i=0; i<num_solver; i++) {
    monitor->print ("perf:solver","max-%s-iter %lld",
                    problem()->solver(i)->name().c_str(),
                    counters_reduce[m++]);
  }
  cello::simulation()->clear_solver_iter();


  ASSERT3("Simulation::monitor_performance()",
          "Actual array length %d != expected array length 2 + %d + %d",
          m,num_sum,num_max,
          (m == 2+num_sum+num_max) );
#ifdef TRACE_PROCESS_MEMORY
  Memory * memory = Memory::instance();
  CkPrintf ("TRACE_PERF proc %d cycle %d bytes curr %lld high %lld highest %lld\n",
            CkMyPe(),cycle_,memory->bytes(),memory->bytes_high(), memory->bytes_highest());
#endif

  delete msg;

  Memory::instance()->reset_high();

}
