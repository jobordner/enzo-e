// See LICENSE_CELLO file for license and copyright information

/// @file     io_IoBlock.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2011-10-06
/// @brief    Implementation of the IoBlock class

#include "io.hpp"

//----------------------------------------------------------------------

IoBlock::IoBlock() throw ()
  : Io()
{
  meta_name_.push_back("num_field_data");
  meta_name_.push_back("index");
  meta_name_.push_back("lower");
  meta_name_.push_back("upper");
  meta_name_.push_back("cycle");
  meta_name_.push_back("time");
  meta_name_.push_back("dt");
  meta_name_.push_back("array");
  meta_name_.push_back("order_index");
  meta_name_.push_back("order_count");
  meta_name_.push_back("order_next");
}

//----------------------------------------------------------------------

void IoBlock::set_block (Block * block) throw()
{
  int i;
  Data * data = block->data();
  num_field_data_ = data->num_field_data_;

  for (i=0; i<3; i++) index_[i] = block->index_[i];
  for (i=0; i<3; i++) lower_[i] = data->lower_[i];
  for (i=0; i<3; i++) upper_[i] = data->upper_[i];
  cycle_ = block->cycle_;
  time_  = block->time_;
  dt_    = block->dt_;
  for (i=0; i<3; i++) array_[i] = block->array_[i];
  Index order_next;
  block->get_order(&order_index_, &order_count_, &order_next);
  order_next.values(order_next_);
}

//----------------------------------------------------------------------

void IoBlock::meta_value
(int index,
 void ** buffer, std::string * name, int * type,
 int * nxd, int * nyd, int * nzd) throw()
{
  Io::meta_value(index,buffer,name,type,nxd,nyd,nzd);

  int count = 0;

  if (index == count++) {
    *buffer = (void *) & num_field_data_;
    *type   = type_int;
  } else if (index == count++) {
    *buffer = (void *) & index_;
    *type   = type_int;
    *nxd     = 3;
  } else if (index == count++) {
    *buffer = (void *) lower_;
    *type   = type_double;
    *nxd     = 3;
  } else if (index == count++) {
    *buffer = (void *) upper_;
    *type   = type_double;
    *nxd     = 3;
  } else if (index == count++) {
    *buffer = (void *) & cycle_;
    *type   = type_int;
  } else if (index == count++) {
    *buffer = (void *) & time_;
    *type   = type_double;
  } else if (index == count++) {
    *buffer = (void *) & dt_;
    *type   = type_double;
  } else if (index == count++) {
    *buffer = (void *) & array_;
    *type   = type_int;
    *nxd    = 3;
  } else if (index == count++) {
    *buffer = (void *) & order_index_;
    *type   = type_long_long;
  } else if (index == count++) {
    *buffer = (void *) & order_count_;
    *type   = type_long_long;
  } else if (index == count++) {
    *buffer = (void *) & order_next_;
    *type   = type_int;
    *nxd    = 3;
  }
}
//======================================================================

int IoBlock::data_size () const
{
  int size = 0;

  size += Io::data_size();

  SIZE_SCALAR_TYPE(size,int,num_field_data_);
  SIZE_ARRAY_TYPE(size,int,index_,3);
  SIZE_ARRAY_TYPE(size,double, lower_,3);
  SIZE_ARRAY_TYPE(size,double, upper_,3);
  SIZE_SCALAR_TYPE(size,int, cycle_);
  SIZE_SCALAR_TYPE(size,double, time_);
  SIZE_SCALAR_TYPE(size,double, dt_);
  SIZE_ARRAY_TYPE(size,int,array_,3);
  SIZE_SCALAR_TYPE(size,long long, order_index_);
  SIZE_SCALAR_TYPE(size,long long, order_count_);
  SIZE_ARRAY_TYPE(size,int, order_next_,3);

  return size;
}

//----------------------------------------------------------------------

char * IoBlock::save_data (char * buffer) const
{
  char * pc = buffer;

  pc = Io::save_data(pc);
  
  SAVE_SCALAR_TYPE(pc,int,num_field_data_);
  SAVE_ARRAY_TYPE(pc,int,index_,3);
  SAVE_ARRAY_TYPE(pc,double, lower_,3);
  SAVE_ARRAY_TYPE(pc,double, upper_,3);
  SAVE_SCALAR_TYPE(pc,int, cycle_);
  SAVE_SCALAR_TYPE(pc,double, time_);
  SAVE_SCALAR_TYPE(pc,double, dt_);
  SAVE_ARRAY_TYPE(pc,int,array_,3);
  SAVE_SCALAR_TYPE(pc,long long, order_index_);
  SAVE_SCALAR_TYPE(pc,long long, order_count_);
  SAVE_ARRAY_TYPE(pc,int, order_next_,3);

  ASSERT2 ("IoBlock::save_data()",
  	   "Expecting buffer size %d actual size %d",
  	   IoBlock::data_size(),(pc-buffer),
  	   (IoBlock::data_size() == (pc-buffer)));
  
  // return first byte after filled buffer
  return pc;
}

//----------------------------------------------------------------------

char * IoBlock::load_data (char * buffer)
{
  char * pc = buffer;

  pc = Io::load_data(pc);
  
  LOAD_SCALAR_TYPE(pc,int,num_field_data_);
  LOAD_ARRAY_TYPE(pc,int,index_,3);
  LOAD_ARRAY_TYPE(pc,double, lower_,3);
  LOAD_ARRAY_TYPE(pc,double, upper_,3);
  LOAD_SCALAR_TYPE(pc,int, cycle_);
  LOAD_SCALAR_TYPE(pc,double, time_);
  LOAD_SCALAR_TYPE(pc,double, dt_);
  LOAD_ARRAY_TYPE(pc,int,array_,3);
  LOAD_SCALAR_TYPE(pc,long long, order_index_);
  LOAD_SCALAR_TYPE(pc,long long, order_count_);
  LOAD_ARRAY_TYPE(pc,int, order_next_,3);

  return pc;
}

//----------------------------------------------------------------------

void IoBlock::save_to (void * v)
{
  Block * b = static_cast<Block *>(v);

  for (int i=0; i<3; i++) {
    b->index_[i] = index_[i];
    // Block lower/upper computed from index
    //    b->lower_[i] = lower_[i];
    //    b->upper_[i] = upper_[i];
    b->array_[i] = array_[i];
  }
  b->cycle_ = cycle_;
  b->time_  = time_;
  b->dt_    = dt_;
  Index order_next;
  order_next.set_values(order_next_);
  b->set_order(order_index_, order_count_,order_next);
}

