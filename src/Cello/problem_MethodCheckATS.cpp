// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodCheckATS.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2025-08-21

#include "problem.hpp"

#define TOL 10.0
#define MAX_ERROR_COUNT 64

//----------------------------------------------------------------------

void MethodCheckATS::compute( Block * block) throw()
{
  const int level = block->level();

  if (block->is_leaf()) {

    Field field = block->data()->field();
    int it = field.field_id(field_name_);

    int mx,my,mz;
    int gx,gy,gz;
    field.dimensions  (it,&mx,&my,&mz);
    field.ghost_depth (it,&gx,&gy,&gz);
    cello_float * array_curr = (cello_float *) field.values(it);
    cello_float * array_prev = (cello_float *) field.values(it,1);
    cello_float * error_curr = (cello_float *) field.values(error_curr_);
    cello_float * error_prev = (cello_float *) field.values(error_prev_);

    // Set field = (time + dt)
    const double time_curr = block->state()->time(level);
    const double time_prev = block->state()->time_prev(level);

    test_field_(block,array_curr,error_curr,time_curr,mx,my,mz,gx,gy,gz);
    test_field_(block,array_prev,error_prev,time_prev,mx,my,mz,gx,gy,gz);

  }

  block->compute_done();
}

//----------------------------------------------------------------------

double MethodCheckATS::timestep ( Block * block) throw()
{
  return std::numeric_limits<double>::max();
}

//======================================================================



void MethodCheckATS::init_refresh_()
{
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_field(field_name_);
  // Don't refresh error field: want to keep ghost zones intact
}

//----------------------------------------------------------------------

void MethodCheckATS::test_field_(Block * block,
                                cello_float * array,
                                cello_float * error,
                                cello_float time,
                                int mx, int my, int mz,
                                int gx, int gy, int gz)
{
  int count_err = 0;
  std::map<std::string,int> region_count;
  std::string region = {"000"};
  char * data = region.data();
  for (int iz=0; iz<mz; iz++) {
    data[0] = (iz<gx) ? '-' : (iz<mx-gx) ? '0' : '+';
    for (int iy=0; iy<my; iy++) {
      data[1] = (iz<gx) ? '-' : (iz<mx-gx) ? '0' : '+';
      for (int ix=0; ix<mx; ix++) {
        data[2] = (iz<gx) ? '-' : (iz<mx-gx) ? '0' : '+';
        const int i=ix + mx*(iy + my*iz);
        if (compare_(time,array[i])) {
          int static count = 0;
          if (count < MAX_ERROR_COUNT) {
            count++;
            CkPrintf ("DEBUG_CHECK_ATS mismatch cycle %d %g != %g\n",
                      block->state()->cycle(),time,array[i]);
          }
          if (error) ++error[i];
          ++count_err;
          region_count[region]++;
        }
      }
    }
  }
}

//======================================================================
  
bool MethodCheckATS::compare_ (const cello_float & a, const cello_float & b) const
{
  const double mach = cello::machine_epsilon(precision_default);
  return ((a != b) &&
          (cello::err_rel(a,b) > TOL*mach));
}
