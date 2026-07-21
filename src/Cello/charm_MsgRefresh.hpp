// See LICENSE_CELLO file for license and copyright information

/// @file     charm_MsgRefresh.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-07-06
/// @brief    [\ref Charm] Declaration of the MsgRefresh Charm++ Message

#ifndef CHARM_MSG_REFRESH_HPP
#define CHARM_MSG_REFRESH_HPP

#include "cello.hpp"

class Data;
class DataMsg;

class MsgRefresh : public CMessage_MsgRefresh {

public: // interface

  static int64_t counter[CONFIG_NODE_SIZE];

  MsgRefresh();

  MsgRefresh (DataMsg * data_msg, int id_refresh);

  virtual ~MsgRefresh();

  /// No copy constructors
  MsgRefresh(const MsgRefresh & msg_refresh) throw() = delete;
  MsgRefresh(const MsgRefresh && msg_refresh) throw() = delete;

  /// No assignment operators
  MsgRefresh & operator= (const MsgRefresh & data_msg) throw() = delete;
  MsgRefresh & operator= (const MsgRefresh && data_msg) throw() = delete;

  DataMsg * data_msg ()
  { return data_msg_; }

  int id_refresh() const
  { return id_refresh_; }

  /// Update the Data with data stored in this message
  void update (Data * data);

  void print(const char * message);
  void summary() const;

  const char * tag() { return tag_;}

public: // static methods

  /// Pack data to serialize
  static void * pack (MsgRefresh*);

  /// Unpack data to de-serialize
  static MsgRefresh * unpack(void *);

protected: // attributes

  /// Whether destination is local or remote
  bool is_local_;

  /// New Refresh object id associated with the message
  int id_refresh_;

  DataMsg * data_msg_;

  /// Saved Charm++ buffer for deleting after unpack()
  void * buffer_;

  /// Random hex tag for tracking messages for debugging
  char tag_[TAG_LEN+1];
};

#endif /* CHARM_MSG_HPP */

