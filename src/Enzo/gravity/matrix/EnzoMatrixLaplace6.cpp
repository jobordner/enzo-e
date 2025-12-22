// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMatrixlaplace6.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @author   Daniel R. Reynolds (reynolds@smu.edu)
/// @date     2015-04-02
/// @brief    Implementation of the discrete 6th order Laplace operator

#include "Enzo/enzo.hpp"
#include "Enzo/gravity/gravity.hpp"

// #define DEBUG_MATRIX

//======================================================================

void EnzoMatrixLaplace6::matvec
(int i_y, int i_x,
 Field field, double hx, double hy, double hz,
 int g0) throw()
{
  enzo_float * X = (enzo_float * ) field.values(i_x);
  enzo_float * Y = (enzo_float * ) field.values(i_y);
  
  matvec_(Y,X,field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace6::matvec
(precision_type precision,
 void * y, void * x,
 Field field, double hx, double hy, double hz,
 int g0) throw()
{
  matvec_((enzo_float *)(y),(enzo_float *)(x),field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace6::diagonal
(int i_x,
 Field field, double hx, double hy, double hz,
 int g0) throw()
{
  enzo_float * X = (enzo_float * ) field.values(i_x);

  diagonal_(X,field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

double EnzoMatrixLaplace6::stencil_value
(int ix, int iy, int iz,
 double hx, double hy, double hz) const
{
  const int rank = cello::rank();
  const double dx = (rank >= 1) ? 1.0 / (1080.0*hx*hx) : 0.0;
  const double dy = (rank >= 2) ? 1.0 / (1080.0*hy*hy) : 0.0;
  const double dz = (rank >= 3) ? 1.0 / (1080.0*hz*hz) : 0.0;
  const enzo_float c0 = -2720.0;
  const enzo_float c1 = 1455.0;
  const enzo_float c2 = -96.0;
  const enzo_float c3 = 1.0;
  const int px = std::abs(ix);
  const int py = std::abs(iy);
  const int pz = std::abs(iz);
  if (ix==0 && iy==0 && iz==0) {
    return c0*(dx + dy + dz);
  } else if ((px==1) && (iy==0) && (iz==0)) {
    return c1*dx;
  } else if ((ix==0) && (py==1) && (iz==0)) {
    return c1*dy;
  } else if ((ix==0) && (iy==0) && (pz==1)) {
    return c1*dz;
  } else if ((px==2) && (iy==0) && (iz==0)) {
    return c2*dx;
  } else if ((ix==0) && (py==2) && (iz==0)) {
    return c2*dy;
  } else if ((ix==0) && (iy==0) && (pz==2)) {
    return c2*dz;
  } else if ((px==3) && (iy==0) && (iz==0)) {
    return c3*dx;
  } else if ((ix==0) && (py==3) && (iz==0)) {
    return c3*dy;
  } else if ((ix==0) && (iy==0) && (pz==3)) {
    return c3*dz;
  }

  return 0.0;
}

//======================================================================

void EnzoMatrixLaplace6::matvec_
(enzo_float * Y, enzo_float * X,
 Field field, double hx, double hy, double hz,
 int g0) const throw()
{
  const int rank = cello::rank();

  int mx,my,mz;
  field.dimensions(0,&mx,&my,&mz);

  const int idx = 1;
  const int idy = mx;
  const int idz = mx*my;


  const int idx2 = 2*idx;
  const int idy2 = 2*idy;
  const int idz2 = 2*idz;
  const int idx3 = 3*idx;
  const int idy3 = 3*idy;
  const int idz3 = 3*idz;

  g0 = std::max(3,g0);

  const enzo_float c0 = -2720.0;
  const enzo_float c1 = 1455.0;
  const enzo_float c2 = -96.0;
  const enzo_float c3 = 1.0;
  const enzo_float dx = (rank >= 1) ? 1.0/(1080.0*hx*hx) : 0.0;
  const enzo_float dy = (rank >= 2) ? 1.0/(1080.0*hy*hy) : 0.0;
  const enzo_float dz = (rank >= 3) ? 1.0/(1080.0*hz*hz) : 0.0;

  if (rank == 1) {

    for (int ix=g0; ix<mx-g0; ix++) {
      const int i = ix;
      Y[i] = (c0*(X[i]) +
              c1*(X[i-idx] +X[i+idx]) +
              c2*(X[i-idx2]+X[i+idx2]) +
              c3*(X[i-idx3]+X[i+idx3])) * dx;
    }

  } else if (rank == 2) {

    for   (int iy=g0; iy<my-g0; iy++) {
      for (int ix=g0; ix<mx-g0; ix++) {
        const int i = ix + mx*iy;
        Y[i] = (c0*(X[i]) +
                c1*(X[i-idx] +X[i+idx]) +
                c2*(X[i-idx2]+X[i+idx2]) +
                c3*(X[i-idx3]+X[i+idx3])) * dx
          +    (c0*(X[i]) +
                c1*(X[i-idy] +X[i+idy]) +
                c2*(X[i-idy2]+X[i+idy2]) +
                c3*(X[i-idy3]+X[i+idy3])) * dy;
      }
    }

  } else if (rank == 3) {

    for     (int iz=g0; iz<mz-g0; iz++) {
      for   (int iy=g0; iy<my-g0; iy++) {
        for (int ix=g0; ix<mx-g0; ix++) {
          const int i = ix + mx*(iy + my*iz);
          Y[i] = (c0*(X[i]) +
                  c1*(X[i-idx] +X[i+idx]) +
                  c2*(X[i-idx2]+X[i+idx2]) +
                  c3*(X[i-idx3]+X[i+idx3])) * dx
            +    (c0*(X[i]) +
                  c1*(X[i-idy] +X[i+idy]) +
                  c2*(X[i-idy2]+X[i+idy2]) +
                  c2*(X[i-idy3]+X[i+idy3])) * dy
            +    (c0*(X[i]) +
                  c1*(X[i-idz] +X[i+idz]) +
                  c2*(X[i-idz2]+X[i+idz2]) +
                  c3*(X[i-idz3]+X[i+idz3])) * dz;
        }
      }
    }
  }
}

//----------------------------------------------------------------------

void EnzoMatrixLaplace6::diagonal_
(enzo_float * X,
 Field field, double hx, double hy, double hz,
 int g0) const throw()
{
  const int rank = cello::rank();

  int mx,my,mz;
  field.dimensions(0,&mx,&my,&mz);

  g0 = std::max(3,g0);

  // Sixth-order 19-point discretization

  const enzo_float c0 = -2720.0;
  const enzo_float dx = (rank >= 1) ? 1.0/(1080.0*hx*hx) : 0.0;
  const enzo_float dy = (rank >= 2) ? 1.0/(1080.0*hy*hy) : 0.0;
  const enzo_float dz = (rank >= 3) ? 1.0/(1080.0*hz*hz) : 0.0;

  if (rank == 1) {

    for (int ix=g0; ix<mx-g0; ix++) {
      int i = ix;
      X[i] = c0 * dx;
    }
  } else if (rank == 2) {
    for   (int iy=g0; iy<my-g0; iy++) {
      for (int ix=g0; ix<mx-g0; ix++) {
        int i = ix + mx*iy;
        X[i] = c0 * dx
          +    c0 * dy;
      }
    }
  } else if (rank == 3) {
    for     (int iz=g0; iz<mz-g0; iz++) {
      for   (int iy=g0; iy<my-g0; iy++) {
        for (int ix=g0; ix<mx-g0; ix++) {
          int i = ix + mx*(iy + my*iz);
          X[i] = c0 * dx
            +    c0 * dy
            +    c0 * dz;
        }
      }
    }
  }
}

