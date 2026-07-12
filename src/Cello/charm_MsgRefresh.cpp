// See LICENSE_CELLO file for license and copyright information

/// @file     charm_MsgRefresh.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-12-22
/// @brief    [\ref Charm] Declaration of the MsgRefresh Charm++ message

#include "data.hpp"
#include "charm.hpp"
#include "charm_simulation.hpp"

//----------------------------------------------------------------------

long MsgRefresh::counter[CONFIG_NODE_SIZE] = { };

//----------------------------------------------------------------------

MsgRefresh::MsgRefresh()
    : CMessage_MsgRefresh(),
      tag_(),
      is_local_(true),
      id_refresh_(-1),
      data_msg_(nullptr),
      buffer_(nullptr)
{
  cello::hex_string(tag_,TAG_LEN);
  ++counter[cello::index_static()];
}

//----------------------------------------------------------------------

MsgRefresh::MsgRefresh(int id_refresh, DataMsg * data_msg)
    : CMessage_MsgRefresh(),
      tag_(),
      is_local_(true),
      id_refresh_(id_refresh),
      data_msg_(data_msg),
      buffer_(nullptr)
{
  cello::hex_string(tag_,TAG_LEN);
  ++counter[cello::index_static()];
}

//----------------------------------------------------------------------

MsgRefresh::~MsgRefresh()
{
  --counter[cello::index_static()];
  delete data_msg_;
  data_msg_ = nullptr;
  CkFreeMsg (buffer_);
  buffer_=nullptr;
}

//----------------------------------------------------------------------

void * MsgRefresh::pack (MsgRefresh * msg)
{
  if (msg->buffer_ != nullptr) return msg->buffer_;

  int size = 0;

  SIZE_SCALAR_TYPE(size,int,msg->id_refresh_);
  SIZE_OBJECT_PTR_TYPE(size,DataMsg,msg->data_msg_);
  SIZE_ARRAY_TYPE (size,char,msg->tag_,TAG_LEN+1);

  //--------------------------------------------------
  //  2. allocate buffer using CkAllocBuffer()
  //--------------------------------------------------

  cello::simulation()->refresh_perf_update (msg->id_refresh_, size);

  char * buffer = (char *) CkAllocBuffer (msg,size);

  //--------------------------------------------------
  //  3. serialize message data into buffer 
  //--------------------------------------------------

  char * pc = buffer;

  SAVE_SCALAR_TYPE(pc,int,msg->id_refresh_);
  SAVE_OBJECT_PTR_TYPE(pc,DataMsg,msg->data_msg_);
  SAVE_ARRAY_TYPE (pc,char,msg->tag_,TAG_LEN+1);

  delete msg;

  // Return the buffer

  ASSERT2("MsgRefresh::pack()",
	  "buffer size mismatch %ld allocated %d packed",
	  (pc - (char*)buffer),size,
	  (pc - (char*)buffer) == size);

  return (void *) buffer;
}

//----------------------------------------------------------------------

MsgRefresh * MsgRefresh::unpack(void * buffer)
{

  // 1. Allocate message using CkAllocBuffer.  NOTE do not use new.

  MsgRefresh * msg = 
    (MsgRefresh *) CkAllocBuffer (buffer,sizeof(MsgRefresh));

  msg = new ((void*)msg) MsgRefresh;

  msg->is_local_ = false;

  // 2. De-serialize message data from input buffer into the allocated
  // message (must be consistent with pack())

  char * pc = (char *) buffer;

  LOAD_SCALAR_TYPE(pc,int,msg->id_refresh_);
  LOAD_OBJECT_PTR_TYPE(pc,DataMsg,msg->data_msg_);
  LOAD_ARRAY_TYPE (pc,char,msg->tag_,TAG_LEN+1);

  // 3. Save the input buffer for freeing later

  msg->buffer_ = buffer;

  return msg;
}

//----------------------------------------------------------------------

void MsgRefresh::update (Data * data)
{
  if (data_msg_ == nullptr) return;

  data_msg_->update(data,is_local_);

  if (!is_local_) {
      CkFreeMsg (buffer_);
      buffer_ = nullptr;
  }
}

//----------------------------------------------------------------------

void MsgRefresh::print (const char * message)
{
  CkPrintf ("%s MSG_REFRESH %p %s\n",message,(void*)this,tag_);
  if (data_msg_) {
    data_msg_->print(message);
  } else {
    CkPrintf ("%s MSG_REFRESH data_msg_ = nil\n",message);
  }
  CkPrintf ("%s MSG_REFRESH is_local_ %d\n",message,is_local_?1:0);
  CkPrintf ("%s MSG_REFRESH id_refresh_ %d\n",message,id_refresh_);
  CkPrintf ("%s MSG_REFRESH buffer_ %p\n",message,buffer_);
}

void MsgRefresh::summary() const
{
  CkPrintf ("%d MR %s %p %p %d %d\n",CkMyPe(),tag_,this,data_msg_,is_local_,id_refresh_);
}
