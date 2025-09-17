// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodATS.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2019-06-13

#include "problem.hpp"

#define TOL 10.0
//----------------------------------------------------------------------

void MethodATS::compute( Block * block) throw()
{
  const int level = block->level();

  if (block->is_leaf()) {

    Field field = block->data()->field();
    int it = field.field_id("test_ats");
    cello_float * array_curr = (cello_float *) field.values(it);

    int mx,my,mz;
    int gx,gy,gz;
    field.dimensions  (it,&mx,&my,&mz);
    field.ghost_depth (it,&gx,&gy,&gz);

    const double dt_level   = block->state()->dt(level);
    const double time_level = block->state()->time(level);
    const double dt   = block->state()->dt();
    const double time = block->state()->time();

    advance_field_(array_curr,mx,my,mz,gx,gy,gz,dt_level);

  }

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
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_field("test_ats");
}

//----------------------------------------------------------------------

void MethodATS::advance_field_(cello_float * array,
                               int mx, int my, int mz,
                               int gx, int gy, int gz,
                               double dt)
{
  for (int iz=gz; iz<mz-gz; iz++) {
    for (int iy=gy; iy<my-gy; iy++) {
      for (int ix=gx; ix<mx-gx; ix++){
        const int i=ix + mx*(iy + my*iz);
        array[i] += dt;
      }
    }
  }
}

