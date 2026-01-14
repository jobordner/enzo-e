// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodATS.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2019-06-13

#include "problem.hpp"

//----------------------------------------------------------------------

void MethodATS::compute( Block * block) throw()
{
  block->compute_done();
}

//----------------------------------------------------------------------

double MethodATS::timestep ( Block * block) throw()
{
  double retval=1e10;
  const int level = block->level();
  if (0 <= level && level < dt_level_.size())
    retval = dt_level_[level];
  CkPrintf ("TRACE_ATS %s %g\n",block->name().c_str(),retval);
  return retval;
}

//======================================================================

void MethodATS::init_refresh_()
{
  cello::simulation()->refresh_set_name(ir_post_,name());
}
