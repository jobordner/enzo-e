// See LICENSE_CELLO file for license and copyright information

/// @file     charm_MsgAdapt.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-12-22
/// @brief    [\ref Charm] Declaration of the MsgAdapt Charm++ message

#include "data.hpp"
#include "charm.hpp"
#include "charm_simulation.hpp"

//----------------------------------------------------------------------

int64_t MsgAdapt::counter[CONFIG_NODE_SIZE] = {0};

//----------------------------------------------------------------------

MsgAdapt::~MsgAdapt()
{
  --counter[cello::index_static()];

  CkFreeMsg (buffer_);
  buffer_=nullptr;
}

//----------------------------------------------------------------------

void * MsgAdapt::pack (MsgAdapt * msg)
{
  if (msg->buffer_ != nullptr) {
    // delete msg but leave buffer to return
    void * buffer = nullptr;
    std::swap (buffer,msg->buffer_);
    delete msg;
    return buffer;
  }

  int size = 0;

  SIZE_SCALAR_TYPE(size,int,   msg->adapt_step_);
  SIZE_SCALAR_TYPE(size,Index, msg->index_);
  SIZE_ARRAY_TYPE (size,int,   msg->ic3_,3);
  SIZE_VECTOR_TYPE(size,int,   msg->ofv_[0]);
  SIZE_VECTOR_TYPE(size,int,   msg->ofv_[1]);
  SIZE_VECTOR_TYPE(size,int,   msg->ofv_[2]);
  SIZE_SCALAR_TYPE(size,int,   msg->level_now_);
  SIZE_SCALAR_TYPE(size,int,   msg->level_min_);
  SIZE_SCALAR_TYPE(size,int,   msg->level_max_);
  SIZE_SCALAR_TYPE(size,bool,  msg->can_coarsen_);
  SIZE_SCALAR_TYPE(size,int,   msg->count_);

  //--------------------------------------------------
  //  2. allocate buffer using CkAllocBuffer()
  //--------------------------------------------------

  char * buffer = (char *) CkAllocBuffer (msg,size);

  //--------------------------------------------------
  //  3. serialize message data into buffer 
  //--------------------------------------------------

  char * pc = buffer;

  SAVE_SCALAR_TYPE(pc,int,   msg->adapt_step_);
  SAVE_SCALAR_TYPE(pc,Index, msg->index_);
  SAVE_ARRAY_TYPE (pc,int,   msg->ic3_,3);
  SAVE_VECTOR_TYPE(pc,int,   msg->ofv_[0]);
  SAVE_VECTOR_TYPE(pc,int,   msg->ofv_[1]);
  SAVE_VECTOR_TYPE(pc,int,   msg->ofv_[2]);
  SAVE_SCALAR_TYPE(pc,int,   msg->level_now_);
  SAVE_SCALAR_TYPE(pc,int,   msg->level_min_);
  SAVE_SCALAR_TYPE(pc,int,   msg->level_max_);
  SAVE_SCALAR_TYPE(pc,bool,  msg->can_coarsen_);
  SAVE_SCALAR_TYPE(pc,int,   msg->count_);

  ASSERT2("MsgAdapt::pack()",
	  "buffer size mismatch %ld allocated %d packed",
	  (pc - (char*)buffer),size,
	  (pc - (char*)buffer) == size);

  delete msg;

  // Return the buffer

  return (void *) buffer;
}

//----------------------------------------------------------------------

MsgAdapt * MsgAdapt::unpack(void * buffer)
{

  // 1. Allocate message using CkAllocBuffer.  NOTE do not use new.
 
  MsgAdapt * msg = 
    (MsgAdapt *) CkAllocBuffer (buffer,sizeof(MsgAdapt));

  msg = new ((void*)msg) MsgAdapt;
  
  // 2. De-serialize message data from input buffer into the allocated
  // message (must be consistent with pack())

  char * pc = (char *) buffer;

  LOAD_SCALAR_TYPE(pc,int,   msg->adapt_step_);
  LOAD_SCALAR_TYPE(pc,Index, msg->index_);
  LOAD_ARRAY_TYPE (pc,int,   msg->ic3_,3);
  LOAD_VECTOR_TYPE(pc,int,   msg->ofv_[0]);
  LOAD_VECTOR_TYPE(pc,int,   msg->ofv_[1]);
  LOAD_VECTOR_TYPE(pc,int,   msg->ofv_[2]);
  LOAD_SCALAR_TYPE(pc,int,   msg->level_now_);
  LOAD_SCALAR_TYPE(pc,int,   msg->level_min_);
  LOAD_SCALAR_TYPE(pc,int,   msg->level_max_);
  LOAD_SCALAR_TYPE(pc,bool,  msg->can_coarsen_);
  LOAD_SCALAR_TYPE(pc,int,   msg->count_);

  // 3. Save the input buffer for freeing later

  msg->buffer_ = buffer;

  return msg;
}
