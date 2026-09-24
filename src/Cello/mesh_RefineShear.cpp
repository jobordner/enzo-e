// See LICENSE_CELLO file for license and copyright information

/// @file     mesh_RefineShear.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     Mon Jul 21 16:02:39 PDT 2014
/// @brief    Implementation of RefineShear class

#include "mesh.hpp"

//----------------------------------------------------------------------

RefineShear::RefineShear(double min_refine,
                         double max_coarsen,
                         int    max_level,
                         bool   include_ghosts,
                         std::string output) throw ()
  : Refine (min_refine, max_coarsen, max_level, include_ghosts, output)
{
}

//----------------------------------------------------------------------

int RefineShear::apply ( Block * block ) throw ()
{

  Field field = block->data()->field();

  bool all_coarsen = true;
  bool any_refine = false;

  int nx,ny,nz;
  field.size(&nx,&ny,&nz);

  int rank = nz > 1 ? 3 : (ny > 1 ? 2 : 1);

  Data * data = block->data();
  double xm[3],xp[3];
  data->lower(&xm[0],&xm[1],&xm[2]);
  data->upper(&xp[0],&xp[1],&xp[2]);

  int id_velocity = field.field_id("velocity_x");

  cello_float * vx = field.values("velocity_x");
  cello_float * vy = field.values("velocity_y"); // nullptr if not defined
  cello_float * vz = field.values("velocity_z"); // nullptr if not defined

  int gx,gy,gz;
  field.ghost_depth(id_velocity, &gx,&gy,&gz);

  const int mx = nx + 2*gx;
  const int my = ny + 2*gy;

  if (rank < 2) gy = 0;
  if (rank < 3) gz = 0;

  cello_float * output = initialize_output_(field.field_data());

  cello_float shear;
  cello_float xdy = 0, ydz = 0, zdx = 0;
  cello_float xdz = 0, ydx = 0, zdy = 0;
  const int kx = 1;
  const int ky = (rank >= 2) ? mx : 0;
  const int kz = (rank >= 3) ? mx*my : 0;

  // Compute inner-product of shear vector.  Note works for
  // rank = 1, 2, 3 since

  for (int iz=gz; iz<nz+gz; iz++) {
    for (int iy=gy; iy<ny+gy; iy++) {
      for (int ix=gx; ix<nx+gx; ix++) {
        int i = ix + mx*(iy + my*iz);
        if (rank >= 2) {
          xdy = vx[i+ky] - vx[i-ky];
          ydx = vy[i+kx] - vy[i-kx];
        }
        if (rank >= 3) {
          xdz = vx[i+kz] - vx[i-kz];
          ydz = vy[i+kz] - vy[i-kz];
          zdx = vz[i+kx] - vz[i-kx];
          zdy = vz[i+ky] - vz[i-ky];
        }
        shear = xdy*xdy + xdz*xdz
          +     ydz*ydz + ydx*ydx
          +     zdx*zdx + zdy*zdy;
        if (shear > min_refine_)  any_refine  = true;
        if (shear > max_coarsen_) all_coarsen = false;
        if (output) {
          if (shear > max_coarsen_) output[i] =  0;
          if (shear > min_refine_)  output[i] = +1;
        }
      }
    }
  }

  int adapt_result =
    any_refine ? adapt_refine : (all_coarsen ? adapt_coarsen : adapt_same);

  // Don't refine if already at maximum level
  adjust_for_level_( &adapt_result, block->level() );

  return adapt_result;

}
