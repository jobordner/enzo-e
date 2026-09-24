// See LICENSE_CELLO file for license and copyright information

/// @file     compute_Matrix.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @author   Daniel R. Reynolds (reynolds@smu.edu)
/// @date     2015-06-09
/// @brief    [\ref Compute] Implementation of the Matrix class

#include "compute.hpp"

//----------------------------------------------------------------------

void Matrix::residual (int ir, int ib, int ix,
                       Field field, double hx, double hy, double hz,
                       int g0) throw()
{

  matvec(ir,ix,field,hx,hy,hz,g0);

  cello_float * B = field.values(ib);
  cello_float * R = field.values(ir);

  int mx,my,mz;
  field.dimensions(0,&mx,&my,&mz);
  const int ix0 = (mx > 1) ? g0 : 0;
  const int iy0 = (my > 1) ? g0 : 0;
  const int iz0 = (mz > 1) ? g0 : 0;

  for (int iz=iz0; iz<mz-iz0; iz++) {
    for (int iy=iy0; iy<my-iy0; iy++) {
      for (int ix=ix0; ix<mx-ix0; ix++) {

	const int i=ix + mx*(iy + my*iz);

	R[i] = B[i] - R[i];
      }
    }
  }
}

