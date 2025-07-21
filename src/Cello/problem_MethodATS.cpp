// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodATS.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2019-06-13

#include "problem.hpp"

//----------------------------------------------------------------------

void MethodATS::compute( Block * block) throw()
{

  CkPrintf ("TRACE_METHOD_ATS compute() level %d\n",block->level());

  const int level = block->level();

  if (block->is_leaf()) {

    Field field = block->data()->field();
    int jt = field.field_id("test_ats");
    int jf = field.field_id("face_ats");

    int mx,my,mz;
    int gx,gy,gz;
    field.dimensions  (jt,&mx,&my,&mz);
    field.ghost_depth (jt,&gx,&gy,&gz);
    cello_float * array_curr = (cello_float *) field.values(jt);
    cello_float * face_curr = (cello_float *) field.values(jf);

    // Set field = (time + dt)
    const double time = block->state()->time(level);
    const double dt   = block->state()->dt(level);
    const double value = time + dt;

    test_constant_(array_curr,mx,my,mz,gx,gy,gz);

    test_ghosts_(block,array_curr,mx,my,mz,gx,gy,gz);

    advance_field_(array_curr,mx,my,mz,gx,gy,gz,dt);
    advance_field_(face_curr,mx,my,mz,gx,gy,gz,dt);

    update_face_(face_curr,mx,my,mz,dt);

    cello_float * array_prev = (cello_float *) field.values(jt,1);
    test_history_(array_curr,array_prev,mx,my,mz,gx,gy,gz,level,dt);

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
  refresh->add_field("face_ats");
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

//----------------------------------------------------------------------

void MethodATS::test_constant_(cello_float * array_curr,
                             int mx, int my, int mz,
                             int gx, int gy, int gz)
{
  for (int iz=gz; iz<mz-gz; iz++) {
    for (int iy=gy; iy<my-gy; iy++) {
      for (int ix=gx; ix<mx-gx; ix++){
      }
    }
  }
}
  //----------------------------------------------------------------------

void MethodATS::test_ghosts_(Block * block,
                             cello_float * array_curr,
                             int mx, int my, int mz,
                             int gx, int gy, int gz)
{
  const int level = block->level();
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
    if ( t0 != txm )
      CkPrintf ("DEBUG_METHOD xm mismatch  %d:%8.6g %d:%8.6g\n",
                level,t0,block->face_level(0,-1),txm);
    if ( t0 != txp )
      CkPrintf ("DEBUG_METHOD xp mismatch  %d:%8.6g %d:%8.6g\n",
                level,t0,block->face_level(0,+1),txp);
  }
  if (cello::rank() >= 2) {
    tym = array_curr[ix0 + mx * (iym + my*iz0)];
    typ = array_curr[ix0 + mx * (iyp + my*iz0)];
    if ( t0 != tym )
      CkPrintf ("DEBUG_METHOD ym mismatch  %d:%8.6g %d:%8.6g\n",
                level,t0,block->face_level(1,-1),tym);
    if ( t0 != typ )
      CkPrintf ("DEBUG_METHOD yp mismatch  %d:%8.6g %d:%8.6g\n",
                level,t0,block->face_level(1,+1),typ);
  }
  if (cello::rank() >= 3) {
    tzm = array_curr[ix0 + mx * (iy0 + my*izm)];
    tzp = array_curr[ix0 + mx * (iy0 + my*izp)];
    if ( t0 != tzm )
      CkPrintf ("DEBUG_METHOD zm mismatch  %d:%8.6g %d:%8.6g\n",
                level,t0,block->face_level(2,-1),tzm);
    if ( t0 != tzp )
      CkPrintf ("DEBUG_METHOD zp mismatch  %d:%8.6g %d:%8.6g\n",
                level,t0,block->face_level(2,+1),tzp);
  }

}

//----------------------------------------------------------------------

void MethodATS::update_face_
(cello_float * face_curr, int mx, int my, int mz, double dt)
{
  const int dx = (cello::rank() >= 1) ? 1 : 0;
  const int dy = (cello::rank() >= 2) ? mx : 0;
  const int dz = (cello::rank() >= 3) ? mx*my : 0;

  const int KX = (cello::rank() >= 1) ? 1 : 0;
  const int KY = (cello::rank() >= 2) ? 1 : 0;
  const int KZ = (cello::rank() >= 3) ? 1 : 0;

  const int ix0 = (cello::rank() >= 1) ? mx/2 : 0;
  const int iy0 = (cello::rank() >= 2) ? my/2 : 0;
  const int iz0 = (cello::rank() >= 3) ? mz/2 : 0;

  const int i0 = ix0 + mx * (iy0 + my*iz0);

  for (int kz=-KZ; kz<=KZ; kz++) {
    for (int ky=-KY; ky<=KY; ky++) {
      for (int kx=-KX; kx<=KX; kx++) {
        if (kx || ky || kz ) {
          face_curr[i0 + dx*kx
                    +    dy*ky
                    +    dz*kz] =
            face_curr[i0 + dx*kx*(ix0-1)
                      +    dy*ky*(iy0-1)
                      +    dz*kz*(iz0-1)] + dt;
        }
      }
    }
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
