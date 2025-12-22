// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMatrixIdentity.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @author   Daniel R. Reynolds (reynolds@smu.edu)
/// @date     2015-04-02
/// @brief    Implementation of the discrete Identity operator EnzoMatrixIdentity

#include "Enzo/enzo.hpp"
#include "Enzo/gravity/gravity.hpp"

//======================================================================

void EnzoMatrixIdentity::matvec 
(int id_y, int id_x, Field field, double hx, double hy, double hz, int g0) throw()
{
  enzo_float * X = (enzo_float * ) field.values(id_x);
  enzo_float * Y = (enzo_float * ) field.values(id_y);

  matvec_(Y,X,field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

void EnzoMatrixIdentity::matvec
(precision_type precision, void * y, void * x,
 Field field, double hx, double hy, double hz, int g0) throw()
{
  matvec_((enzo_float *)(y),(enzo_float *)(x),field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

void EnzoMatrixIdentity::diagonal
(int id_x, Field field, double hx, double hy, double hz, int g0) throw()
{
  enzo_float * X = (enzo_float * ) field.values(id_x);

  diagonal_ (X,field,hx,hy,hz,g0);
}

//----------------------------------------------------------------------

double EnzoMatrixIdentity::stencil_value
(int ix, int iy, int iz,
 double hx, double hy, double hz) const
{
  return (ix==0 && iy==0 && iz==0) ? 1.0 : 0.0;
}

//----------------------------------------------------------------------

void EnzoMatrixIdentity::matvec_
(enzo_float * Y, enzo_float * X,
 Field field, double hx, double hy, double hz,int g0) const throw()
{
  int mx,my,mz;
  field.dimensions(0,&mx,&my,&mz);
  const int ix0 = (mx > 1) ? g0 : 0;
  const int iy0 = (my > 1) ? g0 : 0;
  const int iz0 = (mz > 1) ? g0 : 0;

  for (int iz=iz0; iz<mz-iz0; iz++) {
    for (int iy=iy0; iy<my-iy0; iy++) {
      for (int ix=ix0; ix<mx-ix0; ix++) {
	int i = ix + mx*(iy + my*iz);
	Y[i] = X[i];
      }
    }
  }
}

//----------------------------------------------------------------------

void EnzoMatrixIdentity::diagonal_
(enzo_float * X,
 Field field, double hx, double hy, double hz,int g0) const throw()
{
  int mx,my,mz;
  field.dimensions(0,&mx,&my,&mz);

  const int ix0 = (mx > 1) ? g0 : 0;
  const int iy0 = (my > 1) ? g0 : 0;
  const int iz0 = (mz > 1) ? g0 : 0;

  for (int iz=iz0; iz<mz-iz0; iz++) {
    for (int iy=iy0; iy<my-iy0; iy++) {
      for (int ix=ix0; ix<mx-ix0; ix++) {
	int i = ix + mx*(iy + my*iz);
	X[i] = 1.0;
      }
    }
  }
}
