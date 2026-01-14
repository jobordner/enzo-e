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
  EnzoState(int max_level = 1) throw()
    : State(max_level)
  {
    redshift_.resize(max_level,0.0);
  }

  EnzoState(int cycle, double time, double dt, bool stopping) throw() :
  State(cycle,time,dt,stopping)
  {
    redshift_.resize(1,0.0);
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
  };


  /// Update the current time including redshift
  virtual void set_time (double time, int level = 0);

  void set_redshift (double redshift, int level = 0)
  { redshift_[level] = redshift; }

  double redshift (int level = 0) const
  { return redshift_[level]; }

  int data_size () const
  {
    int size = 0;
    size += ((State*)this)->data_size();
    SIZE_VECTOR_TYPE(size,double,redshift_);
    return size;
  }

  char * save_data (char * buffer) const
  {
    char * pc = buffer;
    pc = ((State *)this) -> save_data(pc);
    SAVE_VECTOR_TYPE(pc,double,redshift_);
    return pc;
  }

  char * load_data (char * buffer)
  {
    char * pc = buffer;
    pc = ((State *)this) -> load_data(pc);
    LOAD_VECTOR_TYPE(pc,double,redshift_);
    return pc;
  }

protected: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Current redshift
  std::vector<double> redshift_;
};

#endif /* ENZO_STATE_HPP */
