// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodOrderRotate.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2028-08-17
/// @brief

#include "problem.hpp"

//----------------------------------------------------------------------

MethodOrderRotate::MethodOrderRotate() throw ()
  : Method("order_rotate")
{
  Refresh * refresh = cello::refresh(ir_post_);
}

//======================================================================

void MethodOrderRotate::compute (Block * block) throw()
{

  long long index;
  long long count;
  block->get_order(&index,&count);

  block->set_order( (index+block->state()->cycle()) % count, count, block->index());

  //  block->set_order(CkMyPe(),num_blocks,block->index());

  // if (block->index().is_root()) {
  //   cello::hierarchy()->print();
  // }
  block->compute_done();
}

//----------------------------------------------------------------------
