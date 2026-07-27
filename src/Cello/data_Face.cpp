// See LICENSE_CELLO file for license and copyright information

/// @file     data_Face.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2019-10-22
/// @brief    Implementation of the Face class

#include "data.hpp"

//======================================================================

int Face::data_size () const
{
  int size = 0;

  SIZE_SCALAR_TYPE(size,int,ix_);
  SIZE_SCALAR_TYPE(size,int,iy_);
  SIZE_SCALAR_TYPE(size,int,iz_);

  SIZE_SCALAR_TYPE(size,int,axis_);
  SIZE_SCALAR_TYPE(size,int,face_);

  return size;
}

//----------------------------------------------------------------------

char * Face::save_data (char * buffer) const
{
  char * pc = buffer;

  SAVE_SCALAR_TYPE(pc,int,ix_);
  SAVE_SCALAR_TYPE(pc,int,iy_);
  SAVE_SCALAR_TYPE(pc,int,iz_);

  SAVE_SCALAR_TYPE(pc,int,axis_);
  SAVE_SCALAR_TYPE(pc,int,face_);

  return pc;
}

//----------------------------------------------------------------------

char * Face::load_data (char * buffer)
{
  char * pc = buffer;

  LOAD_SCALAR_TYPE(pc,int,ix_);
  LOAD_SCALAR_TYPE(pc,int,iy_);
  LOAD_SCALAR_TYPE(pc,int,iz_);

  LOAD_SCALAR_TYPE(pc,int,axis_);
  LOAD_SCALAR_TYPE(pc,int,face_);

  return pc;
}

