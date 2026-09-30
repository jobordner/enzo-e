// See LICENSE_CELLO file for license and copyright information

/// @file     charm_MsgOrder.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2023-07-22
/// @brief    [\ref Charm] Declaration of the MsgOrder Charm++ ABC

#ifndef CHARM_MSG_ORDER_HPP
#define CHARM_MSG_ORDER_HPP

#include "cello.hpp"

class MsgOrder : public CMessage_MsgOrder {

public: // interface

  static int64_t counter[CONFIG_NODE_SIZE];

  MsgOrder();

  virtual ~MsgOrder();

  /// Copy constructor
  MsgOrder(const MsgOrder & msg_order) = delete;
  /// Assignment operator
  MsgOrder & operator= (const MsgOrder & data_msg) = delete;

  inline void get_index (long long & index, double & windex) const
  {
    index  = index_;
    windex = windex_;
  }
  inline void set_index (long long index, double windex)
  {
    index_ = index;
    windex_ = windex;
  }
  inline void get_count (long long & count, double & wcount) const
  {
    count  = count_;
    wcount = wcount_;
  }
  inline void set_count (long long count, double wcount)
  {
    count_ = count;
    wcount_ = wcount;
  }
  inline void get_child (int ic3[3]) const
  {
    ic3[0]=ic3_[0];
    ic3[1]=ic3_[1];
    ic3[2]=ic3_[2];
  }
  inline void set_child (int ic3[3])
  {
    ic3_[0]=ic3[0];
    ic3_[1]=ic3[1];
    ic3_[2]=ic3[2];
  }
  void print (std::string message) const
  {
    CkPrintf ("MsgOrder %d %p %s %lld %lld %g %g [%d %d %d]\n",
              CkMyPe(),this,message.c_str(),
              index_,count_,windex_,wcount_,ic3_[0],ic3_[1],ic3_[2]);
  }

public: // static methods

  /// Pack data to serialize
  static void * pack (MsgOrder*);

  /// Unpack data to de-serialize
  static MsgOrder * unpack(void *);

protected: // attributes

  long long index_;
  long long count_;
  double windex_;
  double wcount_;
  int ic3_[3];
};

#endif /* CHARM_MSG_HPP */

