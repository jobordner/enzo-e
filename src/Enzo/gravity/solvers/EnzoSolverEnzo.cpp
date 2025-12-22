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
         min_level,
         max_level),
  index_solve_root_(index_solve_root),
  index_solve_block_(index_solve_block),
  index_solve_smooth_(index_solve_smooth)
{

  // Create solver refresh
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_field (ix_);
  cello::simulation()->refresh_set_name(ir_post_,name);

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
  const int min_face_rank = cello::config()->adapt_min_face_rank;

  // Create new refresh object
  Refresh refresh_level
    (ghost_depth,min_face_rank, neighbor_level, sync_face, 0);
  refresh_level.set_callback(CkIndex_EnzoBlock::p_solver_enzo_refresh_level_end());

  ir_level_list_.resize(max_level + 1);
  int level = 0;
  for (auto & ir_level : ir_level_list_) {
    refresh_level.add_field (ix_);
    refresh_level.set_level(level);
    refresh_level.set_prolong(index_prolong);
    refresh_level.set_restrict(index_restrict);
    refresh_level.set_final_sync(false);

    ir_level = cello::simulation()->new_register_refresh(refresh_level);
    cello::simulation()->refresh_set_name(ir_level,
                                          name+":level:"+std::to_string(level));
    ++level;
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::apply ( std::shared_ptr<Matrix> A, Block * block) throw()
{
  Solver::begin_(block);

  EnzoBlock * enzo_block = enzo::block(block);
  Field field ( enzo_block->data()->field());

  // Initialize linear system matrix and fields
  A_ = A;
  int m = field.dimensions (ib_);
  cello::refresh(ir_post_)->add_field(ix_);
  if ( ! enzo_block->is_leaf() ) {
    std::fill_n ((enzo_float*) field.values(ib_), m, 0.0);
  }
  // if ( enzo_block->is_leaf() ) {
  //   enzo_float * B = (enzo_float *) field.values(ib_);
  //   enzo_float * B_copy = (enzo_float *) field.values("B_copy");
  //   m = field.dimensions (ib_);
  //   for (int i=0; i<m; i++) B_copy[i] = B[i];
  // }
  std::fill_n ((enzo_float*) field.values(ix_),  m, 0.0);

  // Initialize synchronization counters

  Sync * sync_restrict = psync_restrict_(enzo_block);
  Sync * sync_prolong = psync_prolong_(enzo_block);

  sync_restrict->set_stop(1 + cello::num_children()); // self and children
  sync_prolong->set_stop (1 + 1); // self and parent

  const int level = enzo_block->level();

  if (enzo_block->is_leaf()) {

    // If this is a leaf block, call the root solver if level == 0,
    // else restrict ix down to level == 0

    root_solve_begin(enzo_block);

    if (level > 0) {

      // Initialize restrict on leaf blocks to get root level b
      restrict_send(enzo_block);

    }

  } else {

    if (level >= 0 ) {
      // Non-negative level non-leaves call restrict_recv for self-counter
      restrict_recv(enzo_block,nullptr);
    }
  }

  if (level < 0) {
    root_solve_begin(enzo_block);
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::restrict_send(EnzoBlock * enzo_block)
{
  // Pack field
  Index index = enzo_block->index();
  int level   = index.level();
  int ic3[3];
  index.child(level,&ic3[0],&ic3[1],&ic3[2],min_level_);

  FieldMsg * msg = pack_field_(enzo_block,ib_,refresh_coarse,ic3);

  // Send packed field to parent
  Index index_parent = enzo_block->index().index_parent(min_level_);
  enzo::block_array()[index_parent].p_solver_enzo_restrict_recv(msg);
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_restrict_recv(FieldMsg * msg)
{
  static_cast<EnzoSolverEnzo*> (solver())->restrict_recv(this,msg);
}
void EnzoSolverEnzo::restrict_recv(EnzoBlock * enzo_block,
                                   FieldMsg * msg)
{
  // Unpack "B" vector data from children

  // Save field message from child
  if (msg != nullptr) *pmsg_restrict_(enzo_block,msg->child_index()) = msg;

  // Continue if all expected messages received
  if (psync_restrict_(enzo_block)->next() ) {

    // Restore saved messages
    for (int i=0; i<cello::num_children(); i++) {
      msg = *pmsg_restrict_(enzo_block,i);
      *pmsg_restrict_(enzo_block,i) = nullptr;
      // Unpack field from message then delete message
      unpack_field_(enzo_block,msg,ib_,refresh_coarse);
    }

    if (enzo_block->level() > 0) {

      // If level > root, restrict again
      restrict_send(enzo_block);

    }

    root_solve_begin(enzo_block);
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::root_solve_begin(EnzoBlock * enzo_block)
{
  Solver * solve_root = cello::solver(index_solve_root_);

  solve_root->set_sync_id (enzo_sync_id_solver_enzo_root);
  solve_root->set_callback(CkIndex_EnzoBlock::p_solver_enzo_root_solve_end());

  solve_root->set_field_x (ix_);
  solve_root->set_field_b (ib_);

  solve_root->apply(A_,enzo_block);
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_root_solve_end()
{
  static_cast<EnzoSolverEnzo*> (solver())->root_solve_end(this);
}
void EnzoSolverEnzo::root_solve_end(EnzoBlock * enzo_block)
{
  const int level = enzo_block->level();
  if (level < 0) {
    wait_at_end(enzo_block);
  } else if (level == 0) {
    if (enzo_block->is_leaf()) {
      wait_at_end(enzo_block);
    } else {
      prolong_send(enzo_block);
    }
  } else if (level > 0) {
    // call prolong_recv for self-counter for prolong after block solve
    prolong_recv(enzo_block,nullptr);
  }
  if (enzo_block->is_leaf() && level == 0) {
    refresh_level_begin(enzo_block,level+1);
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::prolong_send(EnzoBlock * enzo_block)
{
  ItChild it_child(cello::rank());
  int ic3[3];

  while (it_child.next(ic3)) {

    FieldMsg * msg = pack_field_(enzo_block,ix_,refresh_fine,ic3);

    Index index_child = enzo_block->index().index_child(ic3,min_level_);

    enzo::block_array()[index_child].p_solver_enzo_prolong_recv(msg);

  }
  if (enzo_block->level() == 0) {
    wait_at_end(enzo_block);
  }
}

void EnzoBlock::p_solver_enzo_prolong_recv(FieldMsg * msg)
{
  static_cast<EnzoSolverEnzo*> (solver())->prolong_recv(this,msg);
}
void EnzoSolverEnzo::prolong_recv(EnzoBlock * enzo_block,
                                  FieldMsg * msg)
{
  int retval = (msg==nullptr ? 0:1);

  // Save field message from parent
  if (msg != NULL) *pmsg_prolong_(enzo_block) = msg;

  // Continue if all expected messages received
  if (psync_prolong_(enzo_block)->next() ) {

    // Restore saved message then clear
    msg = *pmsg_prolong_(enzo_block);
    *pmsg_prolong_(enzo_block) = NULL;

    // Unpack field from message then delete message
    unpack_field_(enzo_block,msg,ix_,refresh_fine);

    refresh_level_begin(enzo_block,enzo_block->level());
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::refresh_level_begin(EnzoBlock * enzo_block, int level_refresh)
{
  *plevel_refresh_(enzo_block) = level_refresh;
  Refresh * refresh { cello::refresh(ir_level_list_[level_refresh]) };
  refresh -> set_active (true);
  refresh -> add_field (ix_);
  refresh -> set_level(level_refresh);

  bool call_refresh = false;
  const int level_block = enzo_block->level();
  if (level_block == level_refresh) {
    call_refresh = true;
  } else if (level_block == level_refresh - 1) {
    ASSERT1 ("refresh_level_begin",
             "block %s must be leaf",
             enzo_block->name().c_str(),
             enzo_block->is_leaf());
    ItNeighbor it_neighbor =
      enzo_block->it_neighbor
      (enzo_block->index(),
       refresh->min_face_rank(),
       refresh->neighbor_type(),
       min_level_,
       refresh->root_level());
    int if3[3];
    while (it_neighbor.next(if3)) {
      if (it_neighbor.face_level() == level_refresh) {
        call_refresh = true;
      }
    }
  }
  if (call_refresh) {
    enzo_block->refresh_start
      (ir_level_list_[level_refresh],
       CkIndex_EnzoBlock::p_solver_enzo_refresh_level_end());
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

void EnzoBlock::p_solver_enzo_block_solve_end()
{
  EnzoSolverEnzo * enzo_solver (static_cast<EnzoSolverEnzo*> (solver()));
  enzo_solver->block_solve_end(this);
}

void EnzoSolverEnzo::block_solve_end (EnzoBlock * enzo_block)
{
  const int level = enzo_block->level();
  if (!enzo_block->is_leaf()) {
    prolong_send(enzo_block);
  } else if (level < max_level_) {
    refresh_level_begin(enzo_block,level+1);
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

void EnzoBlock::r_solver_enzo_wait_at_end(CkReductionMsg *msg)
{
  delete msg;
  static_cast<EnzoSolverEnzo*> (solver())->last_smooth(this);
  //  static_cast<EnzoSolverEnzo*> (solver())->end(this);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::last_smooth(EnzoBlock * enzo_block)
{
  Solver * smooth_last = cello::solver(index_solve_smooth_);

  if (smooth_last != nullptr) {
    smooth_last->set_sync_id (enzo_sync_id_solver_enzo_smooth);
    smooth_last->set_callback(CkIndex_EnzoBlock::p_solver_enzo_last_smooth_end());

    smooth_last->set_field_x(ix_);
    smooth_last->set_field_b(ib_);

    smooth_last->apply(A_,enzo_block);
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
  if ( block->is_leaf() ) {
    Field field = block->data()->field();
    const int m = field.dimensions(ix_);
    enzo_float * X = (enzo_float *) field.values(ix_);
    // enzo_float * X_copy = (enzo_float *) field.values("X_copy");
    // for (int i=0; i<m; i++) X_copy[i] = X[i];
  }
  Solver::end_(block);
}

//======================================================================

FieldMsg * EnzoSolverEnzo::pack_field_(EnzoBlock * enzo_block,
				     int index_field,
				     int refresh_type,
				     int * ic3)
{
  int  if3[3] = {0,0,0};
  int g3[3];
  cello::field_descr()->ghost_depth(index_field,g3,g3+1,g3+2);
  if (refresh_type != refresh_fine)
    for (int i=0; i<3; i++) g3[i]=0;

  Refresh * refresh = new Refresh;
  refresh->set_prolong(index_prolong_);
  refresh->set_restrict(index_restrict_);
  refresh->add_field(index_field);

  FieldFace * field_face = enzo_block->create_face
    (if3, ic3, g3, refresh_type, refresh);

  if (refresh_type == refresh_fine) {
    refresh->set_prolong(index_prolong_);
  } else if (refresh_type == refresh_coarse) {
    refresh->set_restrict(index_restrict_);
  }

  Field field = enzo_block->data()->field();
  int narray;
  char * array;
  field_face->face_to_array(field,&narray,&array);

  delete field_face;

  FieldMsg * msg  = new (narray) FieldMsg;

  msg->n = narray;
  memcpy (msg->a, array, narray);
  delete [] array;

  msg->ic3[0] = ic3[0];
  msg->ic3[1] = ic3[1];
  msg->ic3[2] = ic3[2];

  return msg;

}

//----------------------------------------------------------------------

void EnzoSolverEnzo::unpack_field_
(EnzoBlock * enzo_block,
 FieldMsg * msg,
 int index_field,
 int refresh_type)
{
  int if3[3] = {0,0,0};
  int g3[3];
  cello::field_descr()->ghost_depth(index_field,g3,g3+1,g3+2);
  if (refresh_type != refresh_fine)
    for (int i=0; i<3; i++) g3[i]=0;
  Refresh * refresh = new Refresh;
  refresh->set_prolong(index_prolong_);
  refresh->set_restrict(index_restrict_);
  refresh->add_field(index_field);

  int * ic3 = msg->ic3;

  FieldFace * field_face = enzo_block->create_face
    (if3, ic3, g3, refresh_type, refresh);

  if (refresh_type == refresh_fine) {
    refresh->set_prolong(index_prolong_);
  } else if (refresh_type == refresh_coarse) {
    refresh->set_restrict(index_restrict_);
  }

  Field field = enzo_block->data()->field();

  char * a = msg->a;
  field_face->array_to_face(a, field);
  delete field_face;

  delete msg;
}

//----------------------------------------------------------------------

