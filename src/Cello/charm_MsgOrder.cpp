// See LICENSE_CELLO file for license and copyright information

/// @file     charm_MsgOrder.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2023-07-22
/// @brief    [\ref Charm] Declaration of the MsgOrder Charm++ message

#include "charm.hpp"

//----------------------------------------------------------------------

int64_t MsgOrder::counter[CONFIG_NODE_SIZE] = {0};

//----------------------------------------------------------------------

MsgOrder::MsgOrder()
  : CMessage_MsgOrder(),
    index_(0),
    count_(0),
    windex_(0.0),
    wcount_(0.0)
{
  ++counter[cello::index_static()];
  ic3_[0] = ic3_[1] = ic3_[2] = 0;
}

//----------------------------------------------------------------------

MsgOrder::~MsgOrder()
{
  --counter[cello::index_static()];
}

//----------------------------------------------------------------------

void * MsgOrder::pack (MsgOrder * msg)
{
  int size = 0;

  SIZE_SCALAR_TYPE (size, long long, msg->index_);
  SIZE_SCALAR_TYPE (size, long long, msg->count_);
  SIZE_SCALAR_TYPE (size, double,    msg->windex_);
  SIZE_SCALAR_TYPE (size, double,    msg->wcount_);
  SIZE_ARRAY_TYPE  (size, int,       msg->ic3_,3);

  char * buffer = (char *) CkAllocBuffer (msg,size);
  char * pc = buffer;

  SAVE_SCALAR_TYPE (pc, long long, msg->index_);
  SAVE_SCALAR_TYPE (pc, long long, msg->count_);
  SAVE_SCALAR_TYPE (pc, double,    msg->windex_);
  SAVE_SCALAR_TYPE (pc, double,    msg->wcount_);
  SAVE_ARRAY_TYPE  (pc, int,       msg->ic3_,3);

  delete msg;

  ASSERT2("MsgOrder::pack()",
	  "buffer size mismatch %ld allocated %d packed",
	  (pc - (char*)buffer),size,
	  (pc - (char*)buffer) == size);

  return (void *) buffer;
}

//----------------------------------------------------------------------

MsgOrder * MsgOrder::unpack(void * buffer)
{

  // 1. Allocate message using CkAllocBuffer.  NOTE do not use new.

  MsgOrder * msg = 
    (MsgOrder *) CkAllocBuffer (buffer,sizeof(MsgOrder));

  msg = new ((void*)msg) MsgOrder;

  char * pc = (char *) buffer;

  LOAD_SCALAR_TYPE (pc, long long, msg->index_);
  LOAD_SCALAR_TYPE (pc, long long, msg->count_);
  LOAD_SCALAR_TYPE (pc, double,    msg->windex_);
  LOAD_SCALAR_TYPE (pc, double,    msg->wcount_);
  LOAD_ARRAY_TYPE  (pc, int,       msg->ic3_,3);

  return msg;
}

