/// @file     enzo_EnzoSolverEnzo.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2025-11-24
/// @brief    Implements the EnzoSolverEnzo class
///
/// @brief [\ref Enzo] Root-grid solve (e.g. using mg0), then extended
/// block solves (e.g. using CG) on progressively finer levels. This 
/// mimicks ENZO's PM gravity solver, which uses an FFT on the root grid
/// and MG on extended patches.
///
/// 1. root grid solve
/// 2. prolong level 0 -> 1
/// 3. refresh level 1
/// 4. solve level-1 blocks
/// 5. prolong level 1 -> 2
/// etc.
/// 6. end after all blocks call global barrier

#include "Cello/cello.hpp"
#include "Enzo/enzo.hpp"
#include "Enzo/gravity/gravity.hpp"

//======================================================================

EnzoSolverEnzo::EnzoSolverEnzo
  (std::string name,
   std::string field_x,
   std::string field_b,
   int monitor_iter,
   int restart_cycle,
   int solve_type,
   int index_prolong,
   int index_restrict,
   int force_global_timestep,
   int min_level,
   int max_level,
   int index_solve_root,
   int index_solve_block,
   int index_solve_smooth)
: Solver(name,
         field_x,
         field_b,
         monitor_iter,
         restart_cycle,
         solve_type,
         index_prolong,
         index_restrict,
         force_global_timestep,
         min_level,
         max_level),
  index_solve_root_(index_solve_root),
  index_solve_block_(index_solve_block),
  index_solve_smooth_(index_solve_smooth)
{

  // Create solver refresh
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_field (ix_);

  // Create restrict and prolong sync counters
  ScalarDescr * scalar_descr_sync = cello::scalar_descr_sync();
  i_sync_restrict_ = scalar_descr_sync->new_value(name + ":restrict");
  i_sync_prolong_  = scalar_descr_sync->new_value(name + ":prolong");

  // Create Prolong and restrict message queues
  ScalarDescr * scalar_descr_void = cello::scalar_descr_void();
  i_msg_prolong_ = scalar_descr_void->new_value(name + ":msg_prolong");
  for (int ic=0; ic<cello::num_children(); ic++) {
    i_msg_restrict_[ic] = scalar_descr_void->new_value(name + ":msg_restrict");
  }

  // Create level_refresh scalar to save active refresh level
  ScalarDescr * scalar_descr_int = cello::scalar_descr_int();
  i_level_refresh_ = scalar_descr_int->new_value(name + ":level_refresh");

  // Create level refresh
  const int * g3 = cello::config()->field_ghost_depth;
  const int ghost_depth = std::max({g3[0],g3[1],g3[2]});
  const int min_face_rank = cello::rank() - 1;

  ir_level_list_.resize(max_level + 1);
  int level = 0;
  for (auto & ir_level : ir_level_list_) {
    // Create new refresh object
    Refresh * refresh_level = Refresh::create
      (ghost_depth,min_face_rank, neighbor_level, sync_face, 0);
    refresh_level->set_callback
      (CkIndex_EnzoBlock::p_solver_enzo_refresh_level_end());
    refresh_level->add_field (ix_);
    refresh_level->set_level(level);
    refresh_level->set_prolong(index_prolong);
    refresh_level->set_restrict(index_restrict);
    refresh_level->set_final_sync(false);

    std::string refresh_name =
      std::string("solver_")+this->name()+":level:"+std::to_string(level);
    ir_level = cello::simulation()->new_register_refresh
      (refresh_level,refresh_name);
    ++level;
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::apply
( std::shared_ptr<Matrix> A, Block * block) throw()
{
  Solver::begin_(block);

  EnzoBlock * enzo_block = enzo::block(block);
  Field field ( enzo_block->data()->field());

  // Initialize linear system matrix and fields
  A_ = A;
  const int m = field.dimensions (ib_);
  cello::refresh(ir_post_)->add_field(ix_);

  // Initialize synchronization counters

  Sync * sync_restrict = psync_restrict_(enzo_block);
  Sync * sync_prolong = psync_prolong_(enzo_block);

  sync_restrict->set_stop(1 + cello::num_children()); // self and children
  sync_prolong->set_stop (1 + 1); // self and parent

  const int level = enzo_block->level();
  const int level_root = cello::level_root();
  const int level_lower = level_lower_(block);
  const int level_base = std::max(level_root,level_lower-1);
  const int level_upper = cello::level_top();

  // Initialize X and B vectors
  if ( level >= level_lower) {
    std::fill_n ((enzo_float*) field.values(ix_),  m, 0.0);
  }

  if ( ! enzo_block->is_leaf() ) {
    std::fill_n ((enzo_float*) field.values(ib_), m, 0.0);
  }

  //====================
  // BEGIN SOLVER
  //====================

  wait_at_start(enzo_block);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::wait_at_start(EnzoBlock * enzo_block)
{
  CkCallback callback (CkIndex_EnzoBlock::r_solver_enzo_wait_at_start(nullptr),
                       enzo_block->proxy_array());
  enzo_block->contribute (callback);
}

//----------------------------------------------------------------------

void EnzoBlock::r_solver_enzo_wait_at_start(CkReductionMsg *msg)
{
  delete msg;
  static_cast<EnzoSolverEnzo*> (solver())->begin_solve(this);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::begin_solve(EnzoBlock * enzo_block)
{
  const bool is_leaf = enzo_block->is_leaf();
  const int level = enzo_block->level();
  const int level_lower = level_lower_(enzo_block);

  // CALL ROOT-LEVEL SOLVER

  // all blocks must call solver; leaf blocks and blocks coarser than
  // lower level ready to call now
  if (is_leaf || (level < level_lower)) {
    root_solve_begin(enzo_block);
  }

  // RESTRICT RHS TO LOWER LEVEL

  // call restrict_send starting with non-lower level leaf blocks
  if (is_leaf && (level > level_lower)) {
    restrict_send(enzo_block);
  }

  // call restrict_recv if expecting receive for self-synchronization
  if ((! is_leaf) && (level >= level_lower)) {
    restrict_recv(enzo_block,nullptr);
  }

}

//----------------------------------------------------------------------

void EnzoSolverEnzo::restrict_send(EnzoBlock * enzo_block)
{
  // Pack field
  Index index = enzo_block->index();
  const int level = index.level();
  int ic3[3];
  index.child(level,&ic3[0],&ic3[1],&ic3[2],min_level_);

  FieldMsg * msg = pack_field_(enzo_block,ib_,-1,ic3);

  // Send packed field to parent
  Index index_parent = enzo_block->index().index_parent(min_level_);
  enzo::block_array()[index_parent].p_solver_enzo_restrict_recv(msg);
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_restrict_recv(FieldMsg * msg)
{
  static_cast<EnzoSolverEnzo*> (solver())->restrict_recv(this,msg);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::restrict_recv(EnzoBlock * enzo_block,
                                   FieldMsg * msg)
{
  // Buffer field message from child
  if (msg != nullptr) {
    const int ic = msg->child_index();
    *pmsg_restrict_(enzo_block,ic) = msg;
  }

  // Continue if all expected messages received
  if (psync_restrict_(enzo_block)->next() ) {

    // Restore buffered messages
    for (int i=0; i<cello::num_children(); i++) {

      msg = *pmsg_restrict_(enzo_block,i);
      *pmsg_restrict_(enzo_block,i) = nullptr;

      // Unpack field from message then delete message
      unpack_field_(enzo_block,msg,ib_,-1);
    }

    const int level = enzo_block->level();
    const int level_lower = level_lower_(enzo_block);

    if (level > level_lower) {

      // If level > lower level, continue restricting
      restrict_send(enzo_block);

    }

    // done with restricting; call root solver
    root_solve_begin(enzo_block);
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::root_solve_begin(EnzoBlock * enzo_block)
{
  const int level_root = cello::level_root();

  if (level_lower_(enzo_block) == level_root) {

    // If root level an active level, perform root-level solve

    call_root_solve_(enzo_block);

  } else {

    // bypass root solve if lower active level > root level
    root_solve_end(enzo_block);

  }
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_root_solve_end()
{
  static_cast<EnzoSolverEnzo*> (solver())->root_solve_end(this);
}
void EnzoSolverEnzo::root_solve_end(EnzoBlock * enzo_block)
{
  const int level = enzo_block->level();
  const int level_lower = level_lower_(enzo_block);
  const int level_upper = cello::level_top();
  const int level_root =  cello::level_root();
  const bool is_leaf = enzo_block->is_leaf();

  // level_root = 1
  // level_lower = 1
  // level_base = 1
  const int level_base = std::max(level_root,level_lower-1);

  // Prolong
  if ((level == level_base) && (! is_leaf)) {
    prolong_send(enzo_block);
    if (level==level_lower) {
      wait_at_end(enzo_block);
    }
  }
  if (level > level_base) {
    prolong_recv(enzo_block,nullptr);
  }

  // Call refresh level
  if (level == level_base && level < level_upper) {
    refresh_level_begin(enzo_block,level+1);
  }

  // Synchronize at end if done
  if ((level < level_lower) ||
      ((level == level_lower) && (level_lower == level_root) && is_leaf) ) {
    wait_at_end(enzo_block);
  } 
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::prolong_send(EnzoBlock * enzo_block)
{
  ItChild it_child(cello::rank());

  int ic3[3];
  while (it_child.next(ic3)) {

    FieldMsg * msg = pack_field_(enzo_block,ix_,+1,ic3);

    Index index_child = enzo_block->index().index_child(ic3,min_level_);

    enzo::block_array()[index_child].p_solver_enzo_prolong_recv(msg);

  }
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_prolong_recv(FieldMsg * msg)
{
  static_cast<EnzoSolverEnzo*> (solver())->prolong_recv(this,msg);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::prolong_recv(EnzoBlock * enzo_block,
                                  FieldMsg * msg)
{
  // Buffer field message from parent
  if (msg != nullptr) *pmsg_prolong_(enzo_block) = msg;
  // Continue if all expected messages received
  if (psync_prolong_(enzo_block)->next() ) {

    // Restore buffered message then clear
    msg = *pmsg_prolong_(enzo_block);
    *pmsg_prolong_(enzo_block) = nullptr;

    // Unpack field from message then delete message
    unpack_field_(enzo_block,msg,ix_,+1);

    refresh_level_begin(enzo_block,enzo_block->level());
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::refresh_level_begin
(EnzoBlock * enzo_block, int level_refresh)
{
  if (do_call_refresh_(enzo_block,level_refresh)) {

    auto index_refresh = ir_level_list_[level_refresh];
    enzo_block->refresh_start
      (index_refresh,  CkIndex_EnzoBlock::p_solver_enzo_refresh_level_end());

  }
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_refresh_level_end()
{
  static_cast<EnzoSolverEnzo*> (solver())->refresh_level_end(this);
}
void EnzoSolverEnzo::refresh_level_end(EnzoBlock * enzo_block)
{
  if (enzo_block->level() == *plevel_refresh_(enzo_block)) {
    block_solve_begin(enzo_block);
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::block_solve_begin(EnzoBlock * enzo_block)
{
  call_block_solve_(enzo_block);
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_block_solve_end()
{
  EnzoSolverEnzo * enzo_solver (static_cast<EnzoSolverEnzo*> (solver()));
  enzo_solver->block_solve_end(this);
}

void EnzoSolverEnzo::block_solve_end (EnzoBlock * enzo_block)
{
  const int level = enzo_block->level();
  const int level_lower = cello::level_top();
  const int level_upper = cello::level_top();

  if (level < level_upper ) {
    refresh_level_begin(enzo_block, level+1);
  }

  if ( ! enzo_block->is_leaf() ) {
    prolong_send(enzo_block);
  }
  wait_at_end(enzo_block);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::wait_at_end(EnzoBlock * enzo_block)
{
  CkCallback callback (CkIndex_EnzoBlock::r_solver_enzo_wait_at_end(nullptr),
                       enzo_block->proxy_array());
  enzo_block->contribute (callback);
}

//----------------------------------------------------------------------

void EnzoBlock::r_solver_enzo_wait_at_end(CkReductionMsg *msg)
{
  delete msg;
  static_cast<EnzoSolverEnzo*> (solver())->last_smooth(this);
  //  static_cast<EnzoSolverEnzo*> (solver())->end(this);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::last_smooth(EnzoBlock * enzo_block)
{
  if (index_solve_smooth_ >= 0) {
    Solver * smooth_last = cello::solver(index_solve_smooth_);
    call_last_smooth_(enzo_block);

  } else {

    end(enzo_block);

  }
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_last_smooth_end()
{
  EnzoSolverEnzo * enzo_solver (static_cast<EnzoSolverEnzo*> (solver()));
  enzo_solver->end(this);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::end (Block* block)
{
  Field field = block->data()->field();
  const int m = field.dimensions(ix_);

  enzo_float * X = (enzo_float *) field.values(ix_);

  Solver::end_(block);
}

//======================================================================

void EnzoSolverEnzo::call_root_solve_(EnzoBlock * enzo_block)
{
  Solver * solve_root = cello::solver(index_solve_root_);

  solve_root->set_sync_id (enzo_sync_id_solver_enzo_root);
  solve_root->set_callback
    (CkIndex_EnzoBlock::p_solver_enzo_root_solve_end());

  solve_root->set_field_x (ix_);
  solve_root->set_field_b (ib_);
  solve_root->set_max_level(cello::level_root());
  solve_root->apply(A_,enzo_block);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::call_block_solve_(EnzoBlock * enzo_block)
{
  Solver * solve_block = cello::solver(index_solve_block_);

  solve_block->set_min_level(enzo_block->level());
  solve_block->set_max_level(enzo_block->level());
  solve_block->set_sync_id (enzo_sync_id_solver_enzo_block);
  solve_block->set_callback(CkIndex_EnzoBlock::p_solver_enzo_block_solve_end());
  solve_block->set_include_ghosts(true);

  solve_block->set_field_x (ix_);
  solve_block->set_field_b (ib_);

  solve_block->apply(A_,enzo_block);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::call_last_smooth_(EnzoBlock * enzo_block)
{
  Solver * smooth_last = cello::solver(index_solve_smooth_);
  smooth_last->set_sync_id (enzo_sync_id_solver_enzo_smooth);
  smooth_last->set_callback(CkIndex_EnzoBlock::p_solver_enzo_last_smooth_end());

  smooth_last->set_field_x(ix_);
  smooth_last->set_field_b(ib_);

  smooth_last->apply(A_,enzo_block);
}

//----------------------------------------------------------------------

bool EnzoSolverEnzo::do_call_refresh_
(EnzoBlock * enzo_block, int level_refresh)
{
  bool call_refresh = false;

  const int level_block = enzo_block->level();

  Refresh * refresh { cello::refresh(ir_level_list_[level_refresh]) };

  // Do refresh if in level_refresh

  if (level_block == level_refresh) {

    call_refresh = true;

  }

  // Or do refresh if a leaf in next-coarser level and adjacent to any
  // block in level_refresh

  if (enzo_block->is_leaf() && (level_block == level_refresh - 1)) {

    Refresh * refresh { cello::refresh(ir_level_list_[level_refresh]) };

    ItNeighbor it_neighbor =
      enzo_block->it_neighbor
      (enzo_block->index(),
       refresh->min_face_rank(),
       refresh->neighbor_type(),
       min_level_, refresh->root_level());

    int if3[3];
    while (it_neighbor.next(if3)) {
      if (it_neighbor.face_level() == level_refresh) {
        call_refresh = true;
      }
    }

  }

  if (call_refresh) {
    // Initialize refresh if called
    *plevel_refresh_(enzo_block) = level_refresh;
    refresh -> set_active (true);
    refresh -> add_field (ix_);
    refresh -> set_level(level_refresh);
  }

  return call_refresh;
}

//----------------------------------------------------------------------

FieldMsg * EnzoSolverEnzo::pack_field_(EnzoBlock * enzo_block,
				     int index_field,
				     int refresh_type,
				     int * ic3)
{
  Field field = enzo_block->data()->field();
  return field.pack_field_msg
    (index_field, refresh_type, enzo_block->level(),
     index_prolong_, index_restrict_, ic3);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::unpack_field_
(EnzoBlock * enzo_block,
 FieldMsg * msg,
 int index_field,
 int refresh_type)
{
  Field field = enzo_block->data()->field();
  field.unpack_field_msg
    (msg, index_field, refresh_type, enzo_block->level(), index_prolong_, index_restrict_);
}

//----------------------------------------------------------------------

