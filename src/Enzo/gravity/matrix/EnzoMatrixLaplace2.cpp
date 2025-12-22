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

void EnzoMatrixLaplace2::matvec
(int i_y, int i_x,
 Field field, double hx, double hy, double hz,
 int g0) throw()
{
  enzo_float * X = (enzo_float * ) field.values(i_x);
  enzo_float * Y = (enzo_float * ) field.values(i_y);

  matvec_(Y,X,field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace2::matvec
(precision_type precision, void * y, void * x,
 Field field, double hx, double hy, double hz,
 int g0) throw()
{
  matvec_((enzo_float *)(y),(enzo_float *)(x),field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace2::diagonal
(int i_x,
 Field field, double hx, double hy, double hz,
 int g0) throw()
{
  enzo_float * X = (enzo_float * ) field.values(i_x);

  diagonal_(X,field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

double EnzoMatrixLaplace2::stencil_value
(int ix, int iy, int iz,
 double hx, double hy, double hz) const
{
  const int rank = cello::rank();
  const double dx = (rank >= 1) ? 1.0 / (hx*hx) : 0.0;
  const double dy = (rank >= 2) ? 1.0 / (hy*hy) : 0.0;
  const double dz = (rank >= 3) ? 1.0 / (hz*hz) : 0.0;

  const int px = std::abs(ix);
  const int py = std::abs(iy);
  const int pz = std::abs(iz);

  double retval = 0.0;
  if (ix==0 && iy==0 && iz==0) {
    if (rank >= 1) retval += -2.0*dx;
    if (rank >= 2) retval += -2.0*dy;
    if (rank >= 3) retval += -2.0*dz;
  } else if ((px==1) && (iy==0) && (iz==0)) {
    retval = (rank >= 1) ? dx : 0.0;
  } else if ((ix==0) && (py==1) && (iz==0)) {
    retval = (rank >= 2) ? dy : 0.0;
  } else if ((ix==0) && (iy==0) && (pz==1)) {
    retval = (rank >= 3) ? dz : 0.0;
  }

  return retval;
}

//======================================================================

void EnzoMatrixLaplace2::matvec_
(enzo_float * Y, enzo_float * X,
 Field field, double hx, double hy, double hz,
 int g0) const throw()
{
  int mx,my,mz;
  field.dimensions(0,&mx,&my,&mz);

  const int idx = 1;
  const int idy = mx;
  const int idz = mx*my;

  const int rank = cello::rank();

  g0 = std::max(1,g0);

  double dx = (rank >= 1) ? 1.0 / (hx*hx) : 0.0;
  double dy = (rank >= 2) ? 1.0 / (hy*hy) : 0.0;
  double dz = (rank >= 3) ? 1.0 / (hz*hz) : 0.0;

  if (rank == 1) {
    for (int ix=g0; ix<mx-g0; ix++) {
      const int i = ix;
      Y[i] = ( X[i-idx] - 2.0*X[i] + X[i+idx] ) * dx;
    }

  } else if (rank == 2) {
    for   (int iy=g0; iy<my-g0; iy++) {
      for (int ix=g0; ix<mx-g0; ix++) {
        const int i = ix + mx*iy;
        Y[i] = ( X[i+idx] - 2.0*X[i] + X[i-idx]) * dx
          +    ( X[i+idy] - 2.0*X[i] + X[i-idy]) * dy;
      }
    }

  } else if (rank == 3) {
    for     (int iz=g0; iz<mz-g0; iz++) {
      for   (int iy=g0; iy<my-g0; iy++) {
        for (int ix=g0; ix<mx-g0; ix++) {
          const int i = ix + mx*(iy + my*iz);
          Y[i] = ( X[i+idx] - 2.0*X[i] + X[i-idx]) * dx
            +    ( X[i+idy] - 2.0*X[i] + X[i-idy]) * dy
            +    ( X[i+idz] - 2.0*X[i] + X[i-idz]) * dz;
        }
      }
    }
  }
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace2::diagonal_
(enzo_float * X,
 Field field, double hx, double hy, double hz,
 int g0) const throw()
{
  const int rank = cello::rank();

  int mx,my,mz;
  field.dimensions(0,&mx,&my,&mz);

  g0 = std::max(1,g0);

  // Second-order 7-point discretization

  double dx = (rank >= 1) ? 1.0/(hx*hx) : 0.0;
  double dy = (rank >= 2) ? 1.0/(hy*hy) : 0.0;
  double dz = (rank >= 3) ? 1.0/(hz*hz) : 0.0;

  if (rank == 1) {
    for (int ix=g0; ix<mx-g0; ix++) {
      int i = ix;
      X[i] = - 2.0 * dx;
    }
  } else if (rank == 2) {
    for   (int iy=g0; iy<my-g0; iy++) {
      for (int ix=g0; ix<mx-g0; ix++) {
        int i = ix + mx*iy;
        X[i] = - 2.0 * dx
          -      2.0 * dy;
      }
    }
  } else if (rank == 3) {
    for     (int iz=g0; iz<mz-g0; iz++) {
      for   (int iy=g0; iy<my-g0; iy++) {
        for (int ix=g0; ix<mx-g0; ix++) {
          int i = ix + mx*(iy + my*iz);
          X[i] = - 2.0 * dx
            +    - 2.0 * dy
            +    - 2.0 * dz;
        }
      }
    }
  }
}

