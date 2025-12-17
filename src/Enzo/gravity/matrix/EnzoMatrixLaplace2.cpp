// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMatrixLaplace2.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @author   Daniel R. Reynolds (reynolds@smu.edu)
/// @date     2015-04-02
/// @brief    Implementation of the 2nd order discrete Laplace operator

#include "Enzo/enzo.hpp"
#include "Enzo/gravity/gravity.hpp"

// #define DEBUG_MATRIX

//======================================================================

void EnzoMatrixLaplace2::matvec (int i_y, int i_x, Block * block,
				int g0) throw()
{
  Field field = block->data()->field();

  field.dimensions(0,&mx_,&my_,&mz_);
  block->cell_width (&hx_,&hy_,&hz_);

  enzo_float * X = (enzo_float * ) field.values(i_x);
  enzo_float * Y = (enzo_float * ) field.values(i_y);

  matvec_(Y,X,g0);
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace2::matvec
(precision_type precision,
 void * y, void * x, int g0) throw()
{
  matvec_((enzo_float *)(y),(enzo_float *)(x),g0);
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace2::diagonal (int i_x, Block * block, int g0) throw()
{
  Field field = block->data()->field();

  field.dimensions (i_x,&mx_,&my_,&mz_);
  block->cell_width    (&hx_,&hy_,&hz_);

  enzo_float * X = (enzo_float * ) field.values(i_x);

  diagonal_(X,g0);
}

//----------------------------------------------------------------------

double EnzoMatrixLaplace2::stencil_value(int ix, int iy, int iz) const
{
  const int rank = cello::rank();
  const double dx = (rank >= 1) ? 1.0 / (hx_*hx_) : 0.0;
  const double dy = (rank >= 2) ? 1.0 / (hy_*hy_) : 0.0;
  const double dz = (rank >= 3) ? 1.0 / (hz_*hz_) : 0.0;
  const int px = std::abs(ix);
  const int py = std::abs(iy);
  const int pz = std::abs(iz);

  if (ix==0 && iy==0 && iz==0) {
    return -2.0*(dx + dy + dz);
  } else if ((px==1) && (iy==0) && (iz==0)) {
    return dx;
  } else if ((ix==0) && (py==1) && (iz==0)) {
    return dy;
  } else if ((ix==0) && (iy==0) && (pz==1)) {
    return dz;
  }

  return 0.0;
}

//======================================================================

void EnzoMatrixLaplace2::matvec_
(enzo_float * Y, enzo_float * X, int g0) const throw()
{
  const int idx = 1;
  const int idy = mx_;
  const int idz = mx_*my_;

  const int rank = cello::rank();

  g0 = std::max(1,g0);

  double dx = (rank >= 1) ? 1.0 / (hx_*hx_) : 0.0;
  double dy = (rank >= 2) ? 1.0 / (hy_*hy_) : 0.0;
  double dz = (rank >= 3) ? 1.0 / (hz_*hz_) : 0.0;

  if (rank == 1) {
    for (int ix=g0; ix<mx_-g0; ix++) {
      const int i = ix;
      Y[i] = ( X[i-idx] - 2.0*X[i] + X[i+idx] ) * dx;
    }

  } else if (rank == 2) {
    for   (int iy=g0; iy<my_-g0; iy++) {
      for (int ix=g0; ix<mx_-g0; ix++) {
        const int i = ix + mx_*iy;
        Y[i] = ( X[i+idx] - 2.0*X[i] + X[i-idx]) * dx
          +    ( X[i+idy] - 2.0*X[i] + X[i-idy]) * dy;
      }
    }

  } else if (rank == 3) {
    for     (int iz=g0; iz<mz_-g0; iz++) {
      for   (int iy=g0; iy<my_-g0; iy++) {
        for (int ix=g0; ix<mx_-g0; ix++) {
          const int i = ix + mx_*(iy + my_*iz);
          Y[i] = ( X[i+idx] - 2.0*X[i] + X[i-idx]) * dx
            +    ( X[i+idy] - 2.0*X[i] + X[i-idy]) * dy
            +    ( X[i+idz] - 2.0*X[i] + X[i-idz]) * dz;
        }
      }
    }
  }
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace2::diagonal_ (enzo_float * X, int g0) const throw()
{
  const int rank = cello::rank();

  g0 = std::max(1,g0);

  // Second-order 7-point discretization

  double dx = (rank >= 1) ? 1.0/(hx_*hx_) : 0.0;
  double dy = (rank >= 2) ? 1.0/(hy_*hy_) : 0.0;
  double dz = (rank >= 3) ? 1.0/(hz_*hz_) : 0.0;

  if (rank == 1) {
    for (int ix=g0; ix<mx_-g0; ix++) {
      int i = ix;
      X[i] = - 2.0 * dx;
    }
  } else if (rank == 2) {
    for   (int iy=g0; iy<my_-g0; iy++) {
      for (int ix=g0; ix<mx_-g0; ix++) {
        int i = ix + mx_*iy;
        X[i] = - 2.0 * dx
          -      2.0 * dy;
      }
    }
  } else if (rank == 3) {
    for     (int iz=g0; iz<mz_-g0; iz++) {
      for   (int iy=g0; iy<my_-g0; iy++) {
        for (int ix=g0; ix<mx_-g0; ix++) {
          int i = ix + mx_*(iy + my_*iz);
          X[i] = - 2.0 * dx
            +    - 2.0 * dy
            +    - 2.0 * dz;
        }
      }
    }
  }
}

