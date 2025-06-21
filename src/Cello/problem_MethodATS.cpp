// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodATS.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2019-06-13

#include "problem.hpp"

//----------------------------------------------------------------------

void MethodATS::compute( Block * block) throw()
{
  const int level = block->level();
  if (block->state()->is_active(level)) {

    Field field = block->data()->field();
    int it = field.field_id("test_ats");
    int mx,my,mz;
    int nx,ny,nz;
    int gx,gy,gz;
    field.dimensions (it,&mx,&my,&mz);
    cello_float * d = (cello_float *) field.values(it);

    for (int iz=0; iz<mz; iz++) {
      for (int iy=0; iy<my; iy++) {
        for (int ix=0; ix<mx; ix++){
          const int i=ix + mx*(iy + my*iz);
          d[i] = block->state()->time(level) + block->state()->dt(level);
        }
      }
    }
  }
  // CkPrintf ("TRACE_METHOD_ATS level %d time %g dt %g active %d\n",
  //           level,
  //           block->state()->time(level),
  //           block->state()->dt(level),
  //           block->state()->is_active(level));

  block->compute_done();
}

//----------------------------------------------------------------------

double MethodATS::timestep ( Block * block) throw()
{
  double retval=1e10;
  const int level = block->level();
  if (0 <= level && level < dt_level_.size())
    retval = dt_level_[level];
  return retval;
}

//======================================================================

void MethodATS::init_refresh_()
{
  cello::simulation()->refresh_set_name(ir_post_,name());
}
