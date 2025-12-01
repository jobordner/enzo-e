// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSolverEnzo.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2018-10-01
/// @brief    Implements the EnzoSolverEnzo class
///
/// @brief [\ref Enzo] Multigrid on the root-level grid using Enzo, then
/// BiCgStab in overlapping subdomains defined by root-level Blocks.
/// An optional final Jacobi step can be applied to smooth the solution
/// along subdomain boundaries.
///
/// 0. restrict b to level 0
/// 1. coarse solve: solve Ax=b on level 0
/// 2. Prolong x to child blocks as xc
/// 3. domain solve: solve Ai xi = bi in each root-grid block
///        use xc for boundary conditions and initial guess
/// 4. final smoother: apply final smoother on A x = b (if any)
///
///

#include "Cello/cello.hpp"
#include "Enzo/enzo.hpp"
#include "Enzo/gravity/gravity.hpp"

/* #define TRACE_SOLVER */

# ifdef TRACE_SOLVER
#   undef TRACE_SOLVER
#   define TRACE_SOLVER(BLOCK,MSG) CkPrintf ("TRACE_SOLVER %s %s\n",BLOCK->name().c_str(),std::string(MSG).c_str());
# else
#   define TRACE_SOLVER(BLOCK,MSG) /*  ...  */
# endif
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
   int index_solve_block)
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
      index_solve_block_(index_solve_block)
{

  Refresh * refresh = cello::refresh(ir_post_);
  cello::simulation()->refresh_set_name(ir_post_,name);

  refresh->add_field (ix_);

  ScalarDescr * scalar_descr_sync = cello::scalar_descr_sync();
  i_sync_restrict_ = scalar_descr_sync->new_value(name + ":restrict");
  i_sync_prolong_  = scalar_descr_sync->new_value(name + ":prolong");

  ScalarDescr * scalar_descr_void = cello::scalar_descr_void();
  i_msg_prolong_ = scalar_descr_void->new_value(name + ":msg_prolong");
  for (int ic=0; ic<cello::num_children(); ic++) {
    i_msg_restrict_[ic] = scalar_descr_void->new_value(name + ":msg_restrict");
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::apply ( std::shared_ptr<Matrix> A, Block * block) throw()
{
  Solver::begin_(block);
  TRACE_SOLVER(block,"1 apply");
  // Initialize linear system matrix and fields
  A_ = A;
  int m;
  Field field ( block->data()->field());
  cello::refresh(ir_post_)->add_field(ix_);
  if ( ! block->is_leaf() ) {
    m = field.dimensions (ib_);
    std::fill_n ((enzo_float*) field.values(ib_), m, 0.0);
  }
  m = field.dimensions (ix_);
  std::fill_n ((enzo_float*) field.values(ix_),  m, 0.0);

  Sync * sync_restrict = psync_restrict_(block);

  sync_restrict->set_stop(1 + cello::num_children()); // self and children

  Sync * sync_prolong = psync_prolong_(block);

  sync_prolong->set_stop(1 + 1); // self and parent

  const int level = block->level();

  if ( ! block->is_leaf() ) {
    std::fill_n ((enzo_float*) field.values(ib_), m, 0.0);
  }

  std::fill_n ((enzo_float*) field.values(ix_),  m, 0.0);

  if (block->is_leaf()) {

    begin_solve(enzo::block(block));

  } else {

    if ( level >= 0 ) {
      restrict_recv(enzo::block(block),nullptr);
    } else {
      call_root_solver(enzo::block(block));
    }
  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::begin_solve(EnzoBlock * enzo_block) throw()
{
  TRACE_SOLVER(enzo_block,"2 begin_solve");
  if (enzo_block->level() == 0) {

    call_root_solver(enzo_block);

  } else {

    do_restrict (enzo_block);

  }
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::do_restrict(EnzoBlock * enzo_block) throw()
{
  TRACE_SOLVER(enzo_block,"3 do_restrict");
  restrict_send(enzo_block);

  call_root_solver(enzo_block);
}

//----------------------------------------------------------------------

void EnzoSolverEnzo::restrict_send(EnzoBlock * enzo_block) throw()
{
  // Pack field
  TRACE_SOLVER(enzo_block,"4 restrict_send");
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

//----------------------------------------------------------------------

void EnzoSolverEnzo::restrict_recv
(EnzoBlock * enzo_block, FieldMsg * msg) throw()
{
  TRACE_SOLVER(enzo_block,"5 restrict_recv");
  // Unpack "B" vector data from children

  // Save field message from child
  if (msg != NULL) *pmsg_restrict_(enzo_block,msg->child_index()) = msg;

  // Continue if all expected messages received
  if (psync_restrict_(enzo_block)->next() ) {

    // Restore saved messages then clear
    for (int i=0; i<cello::num_children(); i++) {
      msg = *pmsg_restrict_(enzo_block,i);
      *pmsg_restrict_(enzo_block,i) = NULL;
      // Unpack field from message then delete message
      unpack_field_(enzo_block,msg,ib_,refresh_coarse);
    }

    begin_solve(enzo_block);
  }
}

// //----------------------------------------------------------------------

// void EnzoBlock::p_solver_enzo_solve_root()
// {
//   CkCallback callback(CkIndex_EnzoBlock::r_solver_enzo_barrier(NULL),
// 		      enzo::block_array());
//   contribute(callback);
// }

//----------------------------------------------------------------------

void EnzoSolverEnzo::call_root_solver(EnzoBlock * enzo_block) throw()
{
  TRACE_SOLVER(enzo_block,"6 call_root_solver");
  Solver * solve_root = cello::solver(index_solve_root_);

  solve_root->set_sync_id (enzo_sync_id_solver_enzo_root);
  solve_root->set_callback(CkIndex_EnzoBlock::p_solver_enzo_root_done());

  solve_root->set_field_x (ix_);
  solve_root->set_field_b (ib_);

  solve_root->apply(A_,enzo_block);
}


//----------------------------------------------------------------------

void EnzoBlock::p_solver_enzo_root_done()
{
  static_cast<EnzoSolverEnzo*> (solver())->end(this);
//  static_cast<EnzoSolverEnzo*> (solver())->call_root_solver(this);
}

//----------------------------------------------------------------------

// void EnzoBlock::p_solver_enzo_solve_root()
// {
//   //  static_cast<EnzoSolverEnzo*> (solver())->call_block_solver(this);
//     static_cast<EnzoSolverEnzo*> (solver())->end(this);
// }

//----------------------------------------------------------------------

// void EnzoSolverEnzo::call_block_solver(EnzoBlock * enzo_block) throw()
// {
//   Solver * solve_block = cello::solver(index_solve_block_);

//   solve_block->set_sync_id (enzo_sync_id_solver_enzo_block);
//   solve_block->set_callback(CkIndex_EnzoBlock::p_solver_enzo_solve_block());

//   solve_block->set_field_x (ix_);
//   solve_block->set_field_b (ib_);

//   solve_block->apply(A_,enzo_block);
// }

// //----------------------------------------------------------------------

// void EnzoBlock::p_solver_enzo_solve_block()
// {
//   static_cast<EnzoSolverEnzo*> (solver())->end(this);
// }


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

// //----------------------------------------------------------------------

void EnzoSolverEnzo::end (Block* block) throw ()
{
  TRACE_SOLVER(enzo_block,"7 end");
  Solver::end_(block);
}

