// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoState.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2024-01-23
/// @brief    [\ref Enzo] Declaration of the EnzoState class

#ifndef ENZO_STATE_HPP
#define ENZO_STATE_HPP

class EnzoState : public State {

  /// @class    EnzoState
  /// @ingroup  Enzo
  /// @brief    [\ref Enzo]

public: // interface

  /// Constructor
  EnzoState() throw()
    : State(),
      redshift_level_()
  {
  }

  EnzoState(int cycle, double time, double dt, bool stopping) throw() :
    State(cycle,time,dt,stopping),
    redshift_(0.0)
  {
  }

  /// CHARM++ PUP::able declaration
  PUPable_decl(EnzoState);

  /// CHARM++ migration constructor
  EnzoState(CkMigrateMessage *m)
    : State (m)
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {
    TRACEPUP;

    State::pup(p);

    p | redshift_;
    p | redshift_level_;
  };


  /// Update the current time including redshift
  virtual void set_time (double time);
  virtual void set_time (double time, int level);

  void set_redshift (enzo_float redshift)
  { redshift_ = redshift; }

  void set_redshift (enzo_float redshift, int level)
  { redshift_level_[level] = redshift; }

  enzo_float redshift () const
  { return redshift_; }

  enzo_float redshift (int level) const
  { return redshift_level_[level]; }

  int data_size () const
  {
    int size = 0;
    size += ((State*)this)->data_size();
    SIZE_SCALAR_TYPE(size,enzo_float,redshift_);
    SIZE_VECTOR_TYPE(size,enzo_float,redshift_level_);
    return size;
  }

  char * save_data (char * buffer) const
  {
    char * pc = buffer;
    pc = ((State *)this) -> save_data(pc);
    SAVE_SCALAR_TYPE(pc,enzo_float,redshift_);
    SAVE_VECTOR_TYPE(pc,enzo_float,redshift_level_);
    return pc;
  }

  char * load_data (char * buffer)
  {
    char * pc = buffer;
    pc = ((State *)this) -> load_data(pc);
    LOAD_SCALAR_TYPE(pc,enzo_float,redshift_);
    LOAD_VECTOR_TYPE(pc,enzo_float,redshift_level_);
    return pc;
  }

protected: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Current redshift
  enzo_float redshift_;
  std::vector<enzo_float> redshift_level_;
};

#endif /* ENZO_STATE_HPP */
