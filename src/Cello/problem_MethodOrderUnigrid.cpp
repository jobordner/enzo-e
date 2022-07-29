// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodOrderUnigrid.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2022-07-28
/// @brief

#include "problem.hpp"

//----------------------------------------------------------------------

MethodOrderUnigrid::MethodOrderUnigrid() throw ()
: Method(),
  is_index_(-1),
  is_count_(-1)
{
  Refresh * refresh = cello::refresh(ir_post_);
  cello::simulation()->refresh_set_name(ir_post_,name());
  refresh->add_field("density");

  /// Create Scalar data for ordering index
  const int n = cello::num_children();
  is_index_        = cello::scalar_descr_long_long()->new_value(name() + ":index");
  is_count_        = cello::scalar_descr_long_long()->new_value(name() + ":count");

  cello::hierarchy()->root_blocks(&nx_,&ny_,&nz_);
}

//----------------------------------------------------------------------

void MethodOrderUnigrid::compute (Block * block) throw()
{
  int ix,iy,iz;
  block->index().array(&ix,&iy,&iz);
  *pindex_(block) = ((long long) CkNumPes())*(ix + nx_*(iy + ny_*iz)) / (nx_*ny_*nz_);
  *pcount_(block) = nx_*ny_*nz_;
  block->compute_done();
}


//======================================================================

long long * MethodOrderUnigrid::pindex_(Block * block)
{
  Scalar<long long> scalar(cello::scalar_descr_long_long(),
                     block->data()->scalar_data_long_long());
  return scalar.value(is_index_);
}

//----------------------------------------------------------------------

long long * MethodOrderUnigrid::pcount_(Block * block)
{
  Scalar<long long> scalar(cello::scalar_descr_long_long(),
                     block->data()->scalar_data_long_long());
  return scalar.value(is_count_);
}

