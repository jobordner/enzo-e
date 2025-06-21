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

  enum class Type { Global, Level };
  enum class Next { Sequential, Concurrent };

public: // interface

  /// Constructor
  State() throw()
    : PUP::able(),
      cycle_(0),
      time_(0.0),
      dt_(0.0),
      cycle_level_(),
      time_level_(),
      dt_level_(),
      stopping_(false),
      method_state_(),
      level_lower_(0),
      level_upper_(1)
  {
  }

  /// Constructor
  State(int cycle, double time, double dt, bool stopping) throw()
    : PUP::able(),
      cycle_(cycle),
      time_(time),
      dt_(dt),
      cycle_level_(),
      time_level_(),
      dt_level_(),
      stopping_(stopping),
      method_state_(),
      level_lower_(0),
      level_upper_(1),
      state_type_(Type::Global),
      state_next_(Next::Sequential)
  {
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
    p | cycle_level_;
    p | time_level_;
    p | dt_level_;
    p | stopping_;
    p | method_state_;
    p | level_lower_;
    p | level_upper_;
    p | state_type_;
    p | state_next_;
  };

  //----------------------------------------------------------------------
  /// Initializers
  //----------------------------------------------------------------------

  void set_cycle(int cycle)
  { cycle_ = cycle; }
  void set_cycle(int cycle, int level)
  { set_(cycle_level_,level,cycle); }

  virtual void set_time (double time)
  { time_ = time; }
  virtual void set_time (double time, int level)
  { set_(time_level_,level,time); }

  void set_dt (double dt)
  { dt_ = dt; }
  void set_dt (double dt, int level)
  { set_(dt_level_,level,dt); }
  void update_dt (std::vector<double> & dt_level);

  void set_stopping (bool stopping)
  { stopping_ = stopping; }

  void set_levels (int level_lower, int level_upper = 0)
  { level_lower_ = level_lower;
    level_upper_ = level_upper ? level_upper : level_lower_ + 1;
  }

  void init (int cycle, double time, double dt, bool stopping)
  {
    set_cycle (cycle);
    set_time  (time);
    set_dt    (dt);
    set_stopping (stopping);
  }

  void init (int cycle, double time, double dt, bool stopping, int level)
  {
    set_cycle (cycle,level);
    set_time  (time, level);
    set_dt    (dt,   level);
    set_stopping (stopping);
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

  void set_type (const std::string & type, int max_level)
  {
    if (type == "global") {

      state_type_ = Type::Global;

    } else if (type == "level") {

      state_type_ = Type::Level;

      // allocate level states and initialize from global 
      cycle_level_.resize(max_level+1);
      time_level_.resize(max_level+1);
      dt_level_.resize(max_level+1);
      for (int i=0; i<=max_level; i++) {
        cycle_level_[i]  = cycle_;
        dt_level_[i]  = dt_;
        time_level_[i]  = time_;
      }

    } else {

      ERROR1 ("State::set_type()",
              "Unknown State type '%s' (should be \"global\" or \"level\"",
              type.c_str());

    }
  }

  void set_level_type (const std::string & level_type, int max_level)
  {
    if (level_type == "sequential") {
      state_next_ = Next::Sequential;
    } else if (level_type == "concurrent") {
      state_next_ = Next::Concurrent;
      level_lower_ = 0;
      level_upper_ = max_level+1;
    } else {
      ERROR1 ("State::set_level_type()",
              "Unknown State level_type %s (should be \"sequential\" or \"concurrent\"",
              level_type.c_str());
    }
  }

  //----------------------------------------------------------------------
  /// Accessors
  //----------------------------------------------------------------------

  int cycle() const
  {
    int cycle = cycle_;
    if ( state_type_ == Type::Level ) {
      if ( state_next_ == Next::Sequential ) {
        cycle = std::accumulate(cycle_level_.begin(), cycle_level_.end(), 0);
      } else if ( state_next_ == Next::Concurrent ) {
        cycle = cycle_level_.back();
      }
    }
    return cycle;
  }
  int cycle(int level) const
  {
    alloc_(cycle_level_,level);
    return cycle_level_[level]; }

  double time() const
  {
    double time = time_;
    if ( state_type_ == Type::Level ) {
      time = *std::min_element(time_level_.begin(), time_level_.end());
    }
    return time;
  }
  double time(int level) const
  {
    if (state_type_ == Type::Global) return time_;
    alloc_(time_level_,level);
    return time_level_[level]; }

  double dt() const
  { return dt_; }

  double dt(int level) const
  {
    if (state_type_ == Type::Global) return dt_;
    alloc_(dt_level_,level);
    double dt = is_active(level) ? dt_level_[level] : 0.0;
    double t =  time_level_[level] ;
    double tc = std::numeric_limits<double>::max();
    if (level > 0) tc = time_level_[level-1];
    double dtc = 0.0;
    if (level > 0) dtc = dt_level_[level-1];
    if (is_active(level-1)) tc += dtc;
    return (level == 0) ? dt : std::min(dt, tc-t);
  }

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

  int level_lower() const { return level_lower_; }
  int level_upper() const { return level_upper_; }

  //----------------------------------------------------------------------
  // Modifiers
  //----------------------------------------------------------------------

  /// Update level range for next set of timesteps
  void advance();

  /// Return whether blocks in the given level can advance
  bool is_active ( int level ) const;

  /// Return whether blocks in the given level participate in barriers
  bool in_barrier ( int level ) const;

  /// Packing / unpacking
  //----------------------------------------------------------------------

  /// Return the number of bytes required to serialize the data object
  int data_size () const
  {
    int size = 0;
    SIZE_SCALAR_TYPE(size,int,cycle_);
    SIZE_SCALAR_TYPE(size,double,time_);
    SIZE_SCALAR_TYPE(size,double,dt_);
    SIZE_VECTOR_TYPE(size,int,cycle_level_);
    SIZE_VECTOR_TYPE(size,double,time_level_);
    SIZE_VECTOR_TYPE(size,double,dt_level_);
    SIZE_SCALAR_TYPE(size,bool,stopping_);
    SIZE_VECTOR_OBJECT_TYPE(size, MethodState, method_state_);
    SIZE_SCALAR_TYPE(size,int,level_lower_);
    SIZE_SCALAR_TYPE(size,int,level_upper_);
    SIZE_ENUM_CLASS_TYPE(size,Type,state_type_);
    SIZE_ENUM_CLASS_TYPE(size,Next,state_next_);
    return size;
  }

  /// Serialize the object into the provided empty memory buffer.
  char * save_data (char * buffer) const
  {
    char * pc = buffer;
    SAVE_SCALAR_TYPE(pc,int,cycle_);
    SAVE_SCALAR_TYPE(pc,double,time_);
    SAVE_SCALAR_TYPE(pc,double,dt_);
    SAVE_VECTOR_TYPE(pc,int,cycle_level_);
    SAVE_VECTOR_TYPE(pc,double,time_level_);
    SAVE_VECTOR_TYPE(pc,double,dt_level_);
    SAVE_SCALAR_TYPE(pc,bool,stopping_);
    SAVE_VECTOR_OBJECT_TYPE(pc, MethodState, method_state_);
    SAVE_SCALAR_TYPE(pc,int,level_lower_);
    SAVE_SCALAR_TYPE(pc,int,level_upper_);
    SAVE_ENUM_CLASS_TYPE(pc,Type,state_type_);
    SAVE_ENUM_CLASS_TYPE(pc,Next,state_next_);
    return pc;
  }

  /// Restore the object from the provided initialized memory buffer data.
  char * load_data (char * buffer)
  {
    char * pc = buffer;
    LOAD_SCALAR_TYPE(pc,int,cycle_);
    LOAD_SCALAR_TYPE(pc,double,time_);
    LOAD_SCALAR_TYPE(pc,double,dt_);
    LOAD_VECTOR_TYPE(pc,int,cycle_level_);
    LOAD_VECTOR_TYPE(pc,double,time_level_);
    LOAD_VECTOR_TYPE(pc,double,dt_level_);
    LOAD_SCALAR_TYPE(pc,bool,stopping_);
    LOAD_VECTOR_OBJECT_TYPE(pc, MethodState, method_state_);
    LOAD_SCALAR_TYPE(pc,int,level_lower_);
    LOAD_SCALAR_TYPE(pc,int,level_upper_);
    LOAD_ENUM_CLASS_TYPE(pc,Type,state_type_);
    LOAD_ENUM_CLASS_TYPE(pc,Next,state_next_);
    return pc;
  }

  //----------------------------------------------------------------------
  // Debugging
  //----------------------------------------------------------------------
  void print(std::string msg)
  {
    CkPrintf ("State %s\n",msg.c_str());

    if (state_type_ == Type::Global) {

      CkPrintf ("   cycle_ = %d",cycle_);
      CkPrintf ("   time_  = %g",time_);
      CkPrintf ("   dt_    = %g",dt_);

    } else if (state_type_ == Type::Level) {

      CkPrintf ("   cycle_level_[] = ");
      for (int level=0; level<cycle_level_.size(); level++) {
        CkPrintf (" %d",cycle_level_[level]);
      }
      CkPrintf ("\n");

      CkPrintf ("   time_level_[] = ");
      for (int level=0; level<time_level_.size(); level++) {
        CkPrintf (" %g",time_level_[level]);
      }
      CkPrintf ("\n");

      CkPrintf ("   dt_level_[] = ");
      for (int level=0; level<dt_level_.size(); level++) {
        CkPrintf (" %g",dt_level_[level]);
      }
      CkPrintf ("\n");

    }

    CkPrintf ("  stopping_ %d\n",stopping_?1:0);

    for (int i=0; i<method_state_.size(); i++) {
      CkPrintf ("       Method %d time      %g\n",i,method_state_[i].time());
      CkPrintf ("       Method %d dt        %g\n",i,method_state_[i].dt());
      CkPrintf ("       Method %d num_steps %d\n",i,method_state_[i].num_steps());
      CkPrintf ("       Method %d step      %d\n",i,method_state_[i].step());
    }

    CkPrintf ("   level_lower_ %d\n",level_lower_);
    CkPrintf ("   level_upper_ %d\n",level_upper_);

    CkPrintf ("    state_type_ %s\n",
              (state_type_ == Type::Global) ?
              "global" : "level" );
    CkPrintf ("    state_next_ %s\n",
              (state_next_ == Next::Sequential) ?
              "sequential" : "concurrent" );
  }

protected: // functions

  /// Ensure vector is long enough for the given index; resize if needed
  template <typename T>
  void set_ (std::vector<T> & vector, int index, T value)
  {
    alloc_<T>(vector,index);
    vector[index] = value;
  }

  template <typename T>
  void alloc_ (std::vector<T> & vector, int index)
  {
    if (index >= 0) {
      if ( ! (index < vector.size()) ) {
        vector.resize(index+1);
      }
    }
  }

  template <typename T>
  void alloc_ (std::vector<T> & vector, int index) const
  {
    if (index >= 0) {
      if ( ! (index < vector.size()) ) {
        vector.resize(index+1);
      }
    }
  }

protected: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Current global cycle number
  int cycle_;

  /// Current global time 
  double time_;

  /// Current global timestep
  double dt_;

  /// Current level cycles (mutable for resizing)
  mutable std::vector<int> cycle_level_;

  /// Current level time (mutable for resizing)
  mutable std::vector<double> time_level_;

  /// Current level timestep (mutable for resizing)
  mutable std::vector<double> dt_level_;

  /// Current stopping criteria
  bool stopping_;

  /// Method-specific state scalars
  std::vector<MethodState> method_state_;

  /// Range of levels active
  int level_lower_;
  int level_upper_;

  /// Type of timestep being used: global or level
  Type state_type_;

  /// With level timestepping, how to advance active levels:
  /// sequentially (one at a time), or concurrent (multiple
  /// levels at once)
  Next state_next_;
};

#endif /* DATA_STATE_HPP */

