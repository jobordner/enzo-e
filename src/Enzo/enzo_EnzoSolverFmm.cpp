// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSolverFmm.cpp
/// @author   
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2022-06-27
/// @brief    Implements the EnzoSolverFmm class

#include <iostream>
#include "cello.hpp"
#include "enzo.hpp"

#define PRINT_TRAVERSE_PAIRS

//----------------------------------------------------------------------

EnzoSolverFmm::EnzoSolverFmm (double theta)
  : Solver(),
    A_(nullptr),
    theta_(theta),
    min_level_(cello::hierarchy()->min_level()),
    max_level_(cello::hierarchy()->max_level()),
    is_volume_(-1),
    block_volume_(),
    max_volume_(0)
{
  cello::simulation()->refresh_set_name(ir_post_,name());

  Refresh * refresh = cello::refresh(ir_post_);

  // add a field to avoid deadlock
  refresh->add_field("density");

  // Declare long long Block Scalar for volume and save scalar index
  is_volume_ = cello::scalar_descr_long_long()->new_value("solver_fmm_volume");

  // Allocate and initialize block_volume_[level - min_level] to store
  // weighted volume of blocks in different levels relative to the finest
  const int num_children = cello::num_children();
  block_volume_.resize(max_level_-min_level_+1);
  block_volume_[max_level_-min_level_] = 1;
  for (int i=max_level_-1; i>=min_level_; i--) {
    block_volume_[i-min_level_] = block_volume_[i+1-min_level_] * num_children;
  }

  // Compute volume of the domain relative to finest-level blocks
  int nb3[3] = {1,1,1};
  cello::hierarchy()->root_blocks(nb3,nb3+1,nb3+2);
  max_volume_ = nb3[0]*nb3[1]*nb3[2];
  max_volume_ *= block_volume_[max_level_-min_level_];
}

//----------------------------------------------------------------------

void EnzoSolverFmm::pup (PUP::er &p)
{
  // NOTE: change this function whenever attributes change

  TRACEPUP;

  Solver::pup(p);

  p | theta_;
  p | min_level_;
  p | max_level_;
  p | is_volume_;
  p | block_volume_;
  p | max_volume_;
}

//----------------------------------------------------------------------

void EnzoSolverFmm::apply ( std::shared_ptr<Matrix> A,  Block * block) throw()
{
  Solver::begin_(block);

  A_ = A;

  *volume_(block) = 0;
  Index index = block->index();
  if (block->level() == min_level_) {
    int type = block->is_leaf() ? 1 : 0;
    enzo::block_array()[index].p_solver_fmm_traverse(index,type);
  }
  if (! block->is_leaf()) {
    // traverse terminates when all leaf blocks done. Since we use a
    // barrier on all blocks, we call the barrier now on all non-leaf
    // blocks
    CkCallback callback(CkIndex_EnzoBlock::r_solver_fmm_traverse_complete(nullptr),
			enzo::block_array());
    enzo::block(block)->contribute(callback);
  }
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_fmm_traverse(Index index, int type)
{
  EnzoSolverFmm * solver = static_cast<EnzoSolverFmm*> (this->solver());
  solver->traverse(this, index, type);
}

//----------------------------------------------------------------------

void EnzoSolverFmm::traverse
(EnzoBlock * enzo_block, Index index_b, int type_b)
{
  Index index_a = enzo_block->index();
  const bool known_leaf_b = (type_b != -1);
  const bool is_leaf_a = enzo_block->is_leaf();

  if (! known_leaf_b) {
    // If unknown wether B is leaf or not, flip arguments and send to B
    enzo::block_array()[index_b].p_solver_fmm_traverse
      (index_a, is_leaf_a ? 1 : 0);
    return;
  }

  // Determine if blocks are "far", and save radii for later
  double ra,rb;
  bool mac = is_far_(enzo_block,index_b,&ra,&rb);

  // is_leaf values: 0 no, 1 yes -1 unknown
  const bool is_leaf_b = (type_b == 1);

  ItChild it_child_a(cello::rank());
  ItChild it_child_b(cello::rank());
  int ica3[3],icb3[3];

  int level_a = index_a.level();
  int level_b = index_b.level();
  int volume_a = block_volume_[level_a-min_level_];
  int volume_b = block_volume_[level_b-min_level_];

  if (mac) {

    traverse_approx_pair (enzo_block,index_a,volume_a,index_b,volume_b);

  } else if (is_leaf_a && is_leaf_b) {

    traverse_direct_pair (enzo_block,index_a,volume_a,index_b,volume_b);

  } else if (is_leaf_a) {

    while (it_child_b.next(icb3)) {

      Index index_b_child = index_b.index_child(icb3,min_level_);
      traverse(enzo_block,index_b_child,-1);
    }

  } else if (is_leaf_b) {

    while (it_child_a.next(ica3)) {

      Index index_a_child = index_a.index_child(ica3,min_level_);
      enzo::block_array()[index_a_child].p_solver_fmm_traverse
        (index_b,type_b);
    }
  } else if (index_a == index_b) {

    // loop 1 through A children
    while (it_child_a.next(ica3)) {
      int ica = ica3[0] + 2*(ica3[1] + 2*ica3[2]);
      Index index_a_child = index_a.index_child(ica3,min_level_);
      // loop 2 through A children
      while (it_child_b.next(icb3)) {
        int icb = icb3[0] + 2*(icb3[1] + 2*icb3[2]);
        Index index_b_child = index_b.index_child(icb3,min_level_);
        // avoid calling both traverse (A,B) and traverse (B,A)
        if (ica <= icb) {
          // call traverse on child 1 and child 2
          enzo::block_array()[index_a_child].p_solver_fmm_traverse
            (index_b_child,-1);
        }
      }
    }
  } else if (ra < rb) {
    while (it_child_b.next(icb3)) {
      Index index_b_child = index_b.index_child(icb3,min_level_);
      traverse(enzo_block,index_b_child,-1);
    }
  } else {
    while (it_child_b.next(icb3)) {
      Index index_b_child = index_b.index_child(icb3,min_level_);
      traverse(enzo_block,index_b_child,-1);
    }
  }
}

//----------------------------------------------------------------------

void EnzoSolverFmm::traverse_approx_pair
(EnzoBlock * enzo_block,
 Index index_a, int volume_a,
 Index index_b, int volume_b)
{

  // update volumes for each block to detect when to terminate traverse
  update_volume (enzo_block,index_b,volume_b);
  enzo::block_array()[index_b].p_solver_fmm_update_volume (index_a,volume_a);

#ifdef PRINT_TRAVERSE_PAIRS
  CkPrintf ("approx_pair %s %s\n",
            enzo_block->name().c_str(),
            enzo_block->name(index_b).c_str());
#endif
}

//----------------------------------------------------------------------

void EnzoSolverFmm::traverse_direct_pair
(EnzoBlock * enzo_block,
 Index index_a, int volume_a,
 Index index_b, int volume_b)
{
  // update volumes for each block to detect when to terminate traverse
  update_volume (enzo_block,index_b,volume_b);
  if (index_a != index_b) {
    // (note blocks may be equal, so don't double-count)
    enzo::block_array()[index_b].p_solver_fmm_update_volume (index_a,volume_a);
  }

#ifdef PRINT_TRAVERSE_PAIRS
  CkPrintf ("direct_pair %s %s\n",
            enzo_block->name().c_str(),
            enzo_block->name(index_b).c_str());
#endif
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_fmm_update_volume (Index index, int volume)
{
  EnzoSolverFmm * solver = static_cast<EnzoSolverFmm*> (this->solver());
  solver->update_volume (this, index, volume);
}

void EnzoSolverFmm::update_volume
(EnzoBlock * enzo_block, Index index, int volume)
{
  if (enzo_block->is_leaf()) {

    *volume_(enzo_block) += volume;

    if (*volume_(enzo_block) == max_volume_) {

      CkCallback callback(CkIndex_EnzoBlock::r_solver_fmm_traverse_complete(nullptr),
			enzo::block_array());
      enzo_block->contribute(callback);

    }
  } else {

    const int min_level = cello::hierarchy()->min_level();
    ItChild it_child(cello::rank());
    int ic3[3];
    while (it_child.next(ic3)) {
      Index index_child = enzo_block->index().index_child(ic3,min_level);
      enzo::block_array()[index_child].p_solver_fmm_update_volume (index,volume);
    }
  }
}


//----------------------------------------------------------------------

void EnzoBlock::r_solver_fmm_traverse_complete(CkReductionMsg * msg)
{
  delete msg;
  EnzoSolverFmm * solver = static_cast<EnzoSolverFmm*> (this->solver());
  solver->traverse_done(this);
}

void EnzoSolverFmm::traverse_done (EnzoBlock * enzo_block)
{
  Solver::end_(enzo_block);
}

//======================================================================

bool EnzoSolverFmm::is_far_ (EnzoBlock * enzo_block,
                             Index index_b, double *ra, double *rb) const
{
  double iam3[3],iap3[3],ibm3[3],ibp3[3],na3[3],nb3[3];
  enzo_block->lower(iam3,iam3+1,iam3+2);
  enzo_block->upper(iap3,iap3+1,iap3+2);
  enzo_block->lower(ibm3,ibm3+1,ibm3+2,&index_b);
  enzo_block->upper(ibp3,ibp3+1,ibp3+2,&index_b);
  *ra=0;
  *rb=0;
  double ra3[3]={0,0,0},rb3[3]={0,0,0},d3[3]={0,0,0};
  double d = 0;
  for (int i=0; i<cello::rank(); i++) {
    ra3[i] = (iap3[i] - iam3[i]);
    rb3[i] = (ibp3[i] - ibm3[i]);
    *ra += ra3[i]*ra3[i];
    *rb += rb3[i]*rb3[i];
    double ca = (iam3[i]+0.5*ra3[i]);
    double cb = (ibm3[i]+0.5*rb3[i]);
    d3[i] = (ca - cb);
    d += d3[i]*d3[i];
  }

  d = sqrt(d);
  *ra = 0.5*sqrt(*ra);
  *rb = 0.5*sqrt(*rb);

  return  (d > theta_ * (*ra + *rb));
}
