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

  static long counter[CONFIG_NODE_SIZE];

  MsgRefresh() ;

  virtual ~MsgRefresh();

  /// Copy constructor
  MsgRefresh(const MsgRefresh & data_msg) throw()
  {
    ++counter[cello::index_static()]; 
  };

  /// Assignment operator
  MsgRefresh & operator= (const MsgRefresh & data_msg) throw()
  { return *this; }

  // Set the DataMsg object
  void set_data_msg (DataMsg * data_msg);

  /// Set the refresh index
  void set_index_refresh (int index_refresh)
  {  index_refresh_ = index_refresh;  }

  /// Return the refresh index
  int index_refresh () const
  { return index_refresh_; }

  /// Set the type of refresh index
  void set_type_refresh (int type_refresh)
  {  type_refresh_ = type_refresh;  }

  /// Return the refresh type
  int type_refresh () const
  { return type_refresh_; }
  
  /// Update the Data with data stored in this message
  void update (Data * data);

public: // static methods

  /// Pack data to serialize
  static void * pack (MsgRefresh*);

  /// Unpack data to de-serialize
  static MsgRefresh * unpack(void *);
  
protected: // attributes

  /// Whether destination is local or remote
  bool is_local_;

  /// Index of associated refresh object
  int index_refresh_;

  /// Type of refresh
  int type_refresh_;

  DataMsg * data_msg_;

  /// Saved Charm++ buffer for deleting after unpack()
  void * buffer_;

};

#endif /* CHARM_MSG_HPP */

