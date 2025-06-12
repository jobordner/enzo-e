// See LICENSE_CELLO file for license and copyright information

/// @file     data_State.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2023-12-29
/// @brief    [\ref Data] Declaration of the State class

#ifndef DATA_STATE_HPP
#define DATA_STATE_HPP

class State : public PUP::able {

  /// @class    State
  /// @ingroup  Data
  /// @brief    [\ref Data] 

public: // component classes

  #include "data_MethodState.hpp"

public: // interface

  /// Constructor
  State(int max_level = 0) throw()
    : PUP::able(),
      cycle_(),
      time_(),
      dt_(),
      stopping_(false),
      method_state_(),
      level_lower_(0),
      level_upper_(1)
  {
  }

  /// Constructor
  State(int cycle, double time, double dt, bool stopping) throw()
    : PUP::able(),
      stopping_(stopping),
      method_state_(),
      level_lower_(0),
      level_upper_(1)
  {
    set_cycle(cycle);
    set_time(time);
    set_dt(dt);
  }

  /// CHARM++ PUP::able declaration
  PUPable_decl(State);

  State (CkMigrateMessage *m)
    : PUP::able(m)
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {
    TRACEPUP;
    PUP::able::pup(p);
    p | cycle_;
    p | time_;
    p | dt_;
    p | stopping_;
    p | method_state_;
    p | level_lower_;
    p | level_upper_;
  };

  //----------------------------------------------------------------------
  /// Initializers
  //----------------------------------------------------------------------

  virtual void set_cycle(int cycle, int level = 0)
  {
    set_(cycle_,level,cycle);
  }

  virtual void set_time (double time, int level = 0)
  {
    set_(time_,level,time);
  }

  virtual void set_dt (double dt, int level = 0)
  {
    set_(dt_,level,dt);
  }

  virtual void set_stopping (bool stopping)
  {
    stopping_ = stopping;
  }

  virtual void set_levels (int level_lower, int level_upper = 0)
  {
    level_lower_ = level_lower;
    level_upper_ = level_upper ? level_upper : level_lower_ + 1;
  }

  void init (int cycle, double time, double dt, bool stopping, int max_level = 0)
  {
    set_cycle (cycle,max_level);
    set_time  (time, max_level);
    set_dt    (dt,   max_level);
    set_stopping (stopping);
    set_levels (0);
  }

  void init_method(int num_methods = 0)
  {
    if (num_methods == 0) {
      method_state_.clear();
    } else {
      method_state_.resize(num_methods);
      for (auto & m : method_state_)
        m.init();
    }
  }

  //----------------------------------------------------------------------
  /// Accessors
  //----------------------------------------------------------------------

  int cycle(int level = 0) const
  { return cycle_[level]; }

  double time(int level = 0) const
  { return time_[level]; }

  double dt(int level = 0) const
  { return dt_[level]; }

  bool stopping () const { return stopping_; }

  /// Get ith MethodState
  MethodState & method(int index_method) {
    ASSERT2("State::method()",
            "array length %d is too small for index %d",
            method_state_.size(),index_method,
            ((0 <= index_method) && (index_method < method_state_.size())));
    return method_state_[index_method];
  }

  int num_methods() const {
    return method_state_.size();
  }

  //----------------------------------------------------------------------
  /// Packing / unpacking
  //----------------------------------------------------------------------

  /// Return the number of bytes required to serialize the data object
  int data_size () const
  {
    int size = 0;
    SIZE_VECTOR_TYPE(size,int,cycle_);
    SIZE_VECTOR_TYPE(size,double,time_);
    SIZE_VECTOR_TYPE(size,double,dt_);
    SIZE_SCALAR_TYPE(size,bool,stopping_);
    SIZE_VECTOR_OBJECT_TYPE(size, MethodState, method_state_);
    SIZE_SCALAR_TYPE(size,int,level_lower_);
    SIZE_SCALAR_TYPE(size,int,level_upper_);
    return size;
  }

  /// Serialize the object into the provided empty memory buffer.
  char * save_data (char * buffer) const
  {
    char * pc = buffer;
    SAVE_VECTOR_TYPE(pc,int,cycle_);
    SAVE_VECTOR_TYPE(pc,double,time_);
    SAVE_VECTOR_TYPE(pc,double,dt_);
    SAVE_SCALAR_TYPE(pc,bool,stopping_);
    SAVE_VECTOR_OBJECT_TYPE(pc, MethodState, method_state_);
    SAVE_SCALAR_TYPE(pc,int,level_lower_);
    SAVE_SCALAR_TYPE(pc,int,level_upper_);
    return pc;
  }

  /// Restore the object from the provided initialized memory buffer data.
  char * load_data (char * buffer)
  {
    char * pc = buffer;
    LOAD_VECTOR_TYPE(pc,int,cycle_);
    LOAD_VECTOR_TYPE(pc,double,time_);
    LOAD_VECTOR_TYPE(pc,double,dt_);
    LOAD_SCALAR_TYPE(pc,bool,stopping_);
    LOAD_VECTOR_OBJECT_TYPE(pc, MethodState, method_state_);
    LOAD_SCALAR_TYPE(pc,int,level_lower_);
    LOAD_SCALAR_TYPE(pc,int,level_upper_);
    return pc;
  }

  //----------------------------------------------------------------------
  // Debugging
  //----------------------------------------------------------------------
  void print(std::string msg)
  {
    CkPrintf ("State %s\n",msg.c_str());
    for (int level=0; level<time_.size(); level++) {
      CkPrintf ("   cycle_[%d] %d  time_[%d] %g  dt_[%d] %g\n",
                level,cycle_[level],
                level,time_[level],
                level,dt_[level]);
    }
    CkPrintf ("  stopping_ %d\n",stopping_?1:0);
    for (int i=0; i<method_state_.size(); i++) {
      for (int level=0; level<method_state_[i].time_.size(); level++) {
        CkPrintf ("       Method %d level %d time %g  dt %g  num_steps %d  step %d\n",
                  i,level,
                  method_state_[i].time_[level],
                  method_state_[i].dt_[level],
                  method_state_[i].num_steps_[level],
                  method_state_[i].step_[level]);
      }
    }
    CkPrintf ("   level_lower_ %d\n",level_lower_);
    CkPrintf ("   level_upper_ %d\n",level_upper_);
  }

private: // functions

  /// Ensure vector is long enough for the given index; resize if needed
  template <typename T>
  void set_ (std::vector<T> & vector, int index, T value)
  {
    if ( ! (index < vector.size()) )
      vector.resize(index+1);
    vector[index] = value;
  }


protected: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Current cycle number
  std::vector<int> cycle_;

  /// Current time
  std::vector<double> time_;

  /// Current global timestep
  std::vector<double> dt_;

  /// Current stopping criteria
  bool stopping_;

  /// Method-specific state scalars
  std::vector<MethodState> method_state_;

  /// Range of levels active
  int level_lower_;
  int level_upper_;
};

#endif /* DATA_STATE_HPP */

