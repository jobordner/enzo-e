// See LICENSE_CELLO file for license and copyright information

/// @file     data_NewFace.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2019-10-22
/// @brief    Implementation of the NewFace class

#include "data.hpp"

// #define DEBUG_FACE

// #define DEBUG_REFRESH

//======================================================================

bool NewFace::operator == (const NewFace & f)
{
  return (index_this_ == f.index_this_ &&
          index_that_ == f.index_that_);
}

int NewFace::data_size () const
{
  int size = 0;

  SIZE_SCALAR_TYPE(size,Index,index_this_);
  SIZE_SCALAR_TYPE(size,Index,index_that_);

  return size;
}

//----------------------------------------------------------------------

char * NewFace::save_data (char * buffer) const
{

  char * pc =  buffer;

  SAVE_SCALAR_TYPE(pc,Index,index_this_);
  SAVE_SCALAR_TYPE(pc,Index,index_that_);

  return pc;
}

//----------------------------------------------------------------------

char * NewFace::load_data (char * buffer)
{
  char * pc = buffer;

  LOAD_SCALAR_TYPE(pc,Index,index_this_);
  LOAD_SCALAR_TYPE(pc,Index,index_that_);

  return pc;
}

