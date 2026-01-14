// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodCheckATS.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2025-08-21

#include "problem.hpp"

#define TOL 10.0
//----------------------------------------------------------------------

void MethodCheckATS::compute( Block * block) throw()
{
  const int level = block->level();

  if (block->is_leaf()) {

    Field field = block->data()->field();
    int it = field.field_id("test_ats");
    int ie = field.field_id("error_ats");

    int mx,my,mz;
    int gx,gy,gz;
    field.dimensions  (it,&mx,&my,&mz);
    field.ghost_depth (it,&gx,&gy,&gz);
    cello_float * array_curr = (cello_float *) field.values(it);
    cello_float * array_prev = (cello_float *) field.values(it,1);
    cello_float * error = (cello_float *) field.values(ie);

    const int m = mx*my*mz;

    // Set field = (time + dt)
    const double time_curr = block->state()->time(level);
    const double time_prev = block->state()->time_prev(level);
    const double dt   = block->state()->dt(level);

    test_curr_(block,array_curr,error,mx,my,mz,gx,gy,gz,time_curr);
    test_prev_(block,array_prev,error,mx,my,mz,gx,gy,gz,time_curr);

    test_ghosts_(block,array_curr,error,mx,my,mz,gx,gy,gz);

  }

  block->compute_done();
}

//----------------------------------------------------------------------

double MethodCheckATS::timestep ( Block * block) throw()
{
  double retval=1e10;
  return retval;
}

//======================================================================



void MethodCheckATS::init_refresh_()
{
  cello::simulation()->refresh_set_name(ir_post_,name());
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_field("test_ats");
}

//----------------------------------------------------------------------

void MethodCheckATS::test_curr_(Block * block,
                                cello_float * array_curr,
                                cello_float * error,
                                int mx, int my, int mz,
                                int gx, int gy, int gz,
                                double time)
{
  const double mach = cello::machine_epsilon(precision_default);
  bool err = false;
  cello_float value{0};
  int count_err = 0;
  for (int iz=gz; iz<mz-gz; iz++) {
    for (int iy=gy; iy<my-gy; iy++) {
      for (int ix=gx; ix<mx-gx; ix++) {
        const int i=ix + mx*(iy + my*iz);
        if (time != array_curr[i] &&
            cello::err_rel(time,double(array_curr[i])) > TOL*mach ) {
          error[i] += 1;
          ++count_err;
          err = true;
          value=array_curr[i];
        }
      }
    }
  }
  const int max_count=100000;
  static int count = 0;
  if (err) {
    CkPrintf ("%d METHOD_CHECK_ATS %d curr mismatch e.g. %g != %g\n",
              CkMyPe(), count_err,value,time);
    if (count++ >= max_count) {
      ASSERT1("MethodCheckATS","curr mismatch count exceeded %d; exiting!",
              max_count , (err != true));
    }
  }
}

//----------------------------------------------------------------------

void MethodCheckATS::test_prev_(Block * block,
                                cello_float * array_prev,
                                cello_float * error,
                                int mx, int my, int mz,
                                int gx, int gy, int gz,
                                double time)
{
  const double mach = cello::machine_epsilon(precision_default);
  bool err = false;
  cello_float value{0};
  int count_err = 0;
  for (int iz=gz; iz<mz-gz; iz++) {
    for (int iy=gy; iy<my-gy; iy++) {
      for (int ix=gx; ix<mx-gx; ix++) {
        const int i=ix + mx*(iy + my*iz);
        if (time != array_prev[i] &&
            cello::err_rel(time,double(array_prev[i])) > TOL*mach ) {
          error[i] += 2;
          ++count_err;
          err = true;
          value=array_prev[i];
        }
      }
    }
  }
  const int max_count=100000;
  static int count = 0;
  if (err) {
    CkPrintf ("%d METHOD_CHECK_ATS %d prev mismatch e.g. %g != %g\n",
              CkMyPe(), count_err,value,time);
    if (count++ >= max_count) {
      ASSERT1("MethodCheckATS","prev mismatch count exceeded %d; exiting!",
              max_count , (err != true));
    }
  }
}

//----------------------------------------------------------------------

void MethodCheckATS::test_ghosts_(Block * block,
                                  cello_float * array_curr,
                                  cello_float * error,
                                  int mx, int my, int mz,
                                  int gx, int gy, int gz)
{
  const int ix0 = mx/2;
  const int iy0 = my/2;
  const int iz0 = mz/2;

  const int i0 = ix0 + mx * (iy0 + my*iz0);

  int i = 0;
  cello_float txm,tym,tzm;
  cello_float txp,typ,tzp;

  int g3[3] = {gx,gy,gz};
  int m3[3] = {mx,my,mz};

  const double mach = cello::machine_epsilon(precision_default);

  bool err = false;

  const double time = cello::simulation()->state()->time();
  int num_err_face[3][2] = {0};
  int num_err_total = 0;
  double value;
  int i3[3];
  for (int iz=0; iz<mz; iz++) {
    i3[2]=iz;
    for (int iy=0; iy<my; iy++) {
      i3[1]=iy;
      for (int ix=0; ix<mx; ix++) {
        i3[0]=ix;
        const int i=ix + mx*(iy + my*iz);
        if ( time != array_curr[i] &&
             cello::err_rel(time,double(array_curr[i])) > TOL*mach ) {
          ++num_err_total;
          error[i] += 4;
          value = array_curr[i];
          for (int axis=0; axis<cello::rank(); axis++) {
            if (i3[axis]<g3[axis]) {
              ++num_err_face[axis][0];
            } else if (m3[axis]-g3[axis] <= i3[axis]) {
              ++num_err_face[axis][1];
            }
          }
        }
      }
    }
  }

  const int max_count=1000;
  static int count = 0;

  const char * name = block->name().data();
  const int level = block->level();

  for (int axis=0; axis<cello::rank(); axis++) {
    for (int face=0; face<2; face++) {
      if (num_err_face[axis][face]) {
        CkPrintf ("%d METHOD_CHECK_ATS %c%c ghost mismatch "
                  "%g != %g level %d:%d\n",
                  CkMyPe(),('x'+axis),(face==0?'m':'p'),
                  value,time,
                  level,block->face_level(axis,face));
      }
    }
  }

  bool halt_on_err = false;
  if (num_err_total && (count++ >= max_count)) {
    ASSERT1("MethodCheckATS","Ghost mismatch count >= %d",
            max_count , (! halt_on_err));
  }
}

