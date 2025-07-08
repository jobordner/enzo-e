// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodATS.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2019-06-13

#include "problem.hpp"

//----------------------------------------------------------------------

void MethodATS::compute( Block * block) throw()
{

   Refresh * refresh = cello::refresh(ir_post_);

   refresh -> set_adaptive_timestep (true);
   refresh -> set_level_lower(block->state()->level_lower());
   refresh -> set_level_upper(block->state()->level_upper());

   if (block->index().is_root())
     CkPrintf ("DEBUG_ATS refresh level range %d %d\n",
               refresh->level_lower(),
               refresh->level_upper());

  const int level = block->level();

  if (block->state()->is_active(level)) {

    Field field = block->data()->field();
    int it = field.field_id("test_ats");

    int mx,my,mz;
    int gx,gy,gz;
    field.dimensions  (it,&mx,&my,&mz);
    field.ghost_depth (it,&gx,&gy,&gz);
    cello_float * array_curr = (cello_float *) field.values(it);
    cello_float * array_prev = (cello_float *) field.values(it,1);

    test_ghosts_(array_curr,mx,my,mz,gx,gy,gz,level);

    // Clear ghosts
    for (int iz=0; iz<mz; iz++) {
      for (int iy=0; iy<my; iy++) {
        for (int ix=0; ix<mx; ix++){
          const int i=ix + mx*(iy + my*iz);
          array_curr[i] = 0.0;
        }
      }
    }

    // Set field = (time + dt)
    const double value = block->state()->time(level)
      +                  block->state()->dt(level);

    for (int iz=gz; iz<mz-gz; iz++) {
      for (int iy=gy; iy<my-gy; iy++) {
        for (int ix=gx; ix<mx-gx; ix++){
          const int i=ix + mx*(iy + my*iz);
          array_curr[i] = value;
        }
      }
    }

    test_history_(array_curr,array_prev,mx,my,mz,gx,gy,gz,level,block->state()->dt(level));

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

void MethodATS::test_ghosts_(cello_float * array_curr,
                             int mx, int my, int mz,
                             int gx, int gy, int gz,
                             int level)
{
  const int ixm = gx - 1;
  const int ix0 = mx/2;
  const int ixp = mx - gx;

  const int iym = gy - 1;
  const int iy0 = my/2;
  const int iyp = my - gy;

  const int izm = gz - 1;
  const int iz0 = mz/2;
  const int izp = mz - gz;

  const int i0 = ix0 + mx * (iy0 + my*iz0);
  double t0 = array_curr[i0];

  int i = 0;
  cello_float txm,tym,tzm;
  cello_float txp,typ,tzp;
  if (cello::rank() >= 1) {
    txm = array_curr[ixm + mx * (iy0 + my*iz0)];
    txp = array_curr[ixp + mx * (iy0 + my*iz0)];
    if ( (t0 != txm) || (t0 != txp) )
      CkPrintf ("DEBUG_METHOD x-axis mismatch level %d: %8.6g |%8.6g |%8.6g\n",
                level,txm,t0,txp);
  }
  if (cello::rank() >= 2) {
    tym = array_curr[ix0 + mx * (iym + my*iz0)];
    typ = array_curr[ix0 + mx * (iyp + my*iz0)];
    if ( (t0 != tym) || (t0 != typ) )
      CkPrintf ("DEBUG_METHOD y-axis mismatch level %d: %8.6g |%8.6g |%8.6g\n",
                level,tym,t0,typ);
  }
  if (cello::rank() >= 3) {
    tzm = array_curr[ix0 + mx * (iy0 + my*izm)];
    tzp = array_curr[ix0 + mx * (iy0 + my*izp)];
    if ( (t0 != tzm) || (t0 != tzp) )
      CkPrintf ("DEBUG_METHOD z-axis mismatch level %d: %8.6g |%8.6g |%8.6g\n",
                level,tzm,t0,tzp);
  }
}

//----------------------------------------------------------------------

void MethodATS::test_history_(cello_float * array_curr,
                              cello_float * array_prev,
                             int mx, int my, int mz,
                             int gx, int gy, int gz,
                              int level, double dt)
{

  const int i0=gx + mx*(gy + my*gz);

  double diff { array_curr[i0] - array_prev[i0] };

  if ( diff != dt ) {
    CkPrintf ("DEBUG_METHOD history mismatch level %d values %g %g\n",
              level,array_curr[i0],array_prev[i0]);
  }

  int errval = 0;
  for (int iz=gz; iz<mz-gz; iz++) {
    for (int iy=gy; iy<my-gy; iy++) {
      for (int ix=gx; ix<mx-gx; ix++){
        const int i=ix + mx*(iy + my*iz);
        if (array_curr[i] - array_prev[i] != diff)
          errval = true;
      }
    }
  }
  if (errval) {
    CkPrintf ("DEBUG_METHOD history mismatch level %d not const\n", level);
  }
}
