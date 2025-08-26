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
    cello_float * error_curr = (cello_float *) field.values(ie);

    const int m = mx*my*mz;

    // Set field = (time + dt)
    const double time = block->state()->time(level);
    const double dt   = block->state()->dt(level);
    const double value = time + dt;

    test_values_(block,array_curr,error_curr,mx,my,mz,gx,gy,gz,time);

    test_ghosts_(block,array_curr,error_curr,mx,my,mz,gx,gy,gz);

    // cello_float * array_prev = (cello_float *) field.values(it,1);
    // test_history_(block,array_curr,array_prev,error_curr,mx,my,mz,gx,gy,gz,level,dt);

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

void MethodCheckATS::test_values_(Block * block,
                                  cello_float * array_curr,
                                  cello_float * error_curr,
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
          ++error_curr[i];
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
    CkPrintf ("%d METHOD_CHECK_ATS %s %d values mismatch e.g. %12.10g != %12.10g\n",
              CkMyPe(), block->name().c_str(),count_err,value,time);
    if (count++ >= max_count) {
      ASSERT1("MethodCheckATS","Value mismatch count exceeded %d; exiting!",
              max_count , (err != true));
    }
  }
}

//----------------------------------------------------------------------

void MethodCheckATS::test_ghosts_(Block * block,
                                  cello_float * array_curr,
                                  cello_float * error_curr,
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
          ++error_curr[i];
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
        CkPrintf ("%d METHOD_CHECK_ATS %s %c%c ghost mismatch "
                  "%20.16g != %20.16g level %d:%d\n",
                  CkMyPe(),name,('x'+axis),(face==0?'m':'p'),
                  value,time,
                  level,block->face_level(axis,face));
      }
    }
  }

  bool halt_on_err = false;
  if (num_err_total && (count++ >= max_count)) {
    ASSERT1("MethodCheckATS","Ghost zone mismatch count >= %d",
            max_count , (! halt_on_err));
  }
}

//----------------------------------------------------------------------

void MethodCheckATS::test_history_
(Block * block,
 cello_float * array_curr,
 cello_float * array_prev,
 cello_float * error_curr,
 int mx, int my, int mz,
 int gx, int gy, int gz,
 int level, double dt)
{

  const int i0=gx + mx*(gy + my*gz);

  const double diff = array_curr[i0] - array_prev[i0];

  const double mach = cello::machine_epsilon(precision_default);

  bool is_const = true;
  int num_err_face[3][2] = {0};
  int num_err_total = 0;
  double value_prev,value_curr;
  int i3[3];
  int g3[3] = {gx,gy,gz};
  int m3[3] = {mx,my,mz};
  for (int iz=0; iz<mz; iz++) {
    i3[2]=iz;
    for (int iy=0; iy<my; iy++) {
      i3[1]=iy;
      for (int ix=0; ix<mx; ix++){
        i3[0]=ix;
        const int i=ix + mx*(iy + my*iz);
        const double diff_i = array_curr[i] - array_prev[i];
        if ( (diff != diff_i) &&
             cello::err_rel(diff,diff_i) > TOL*mach ) {
          ++num_err_total;
          error_curr[i] ++;
          is_const = false;
          value_curr = diff;
          value_prev = diff_i;
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

  for (int axis=0; axis<cello::rank(); axis++) {
    for (int face=0; face<2; face++) {
      if (num_err_face[axis][face]) {
        CkPrintf ("%d METHOD_CHECK_ATS %s %c%c prev/curr mismatch "
                  "%20.16g != %20.16g level %d:%d equal %d\n",
                  CkMyPe(),name,('x'+axis),(face==0?'m':'p'),
                  value_prev,value_curr,
                  level,block->face_level(axis,face),value_prev == value_curr);
      }
    }
  }

  bool halt_on_err = false;
  if (num_err_total && (count++ >= max_count)) {
    ASSERT1("MethodCheckATS","Ghost zone mismatch count >= %d",
            max_count , (! halt_on_err));
  }
}

