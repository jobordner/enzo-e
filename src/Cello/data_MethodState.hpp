// See LICENSE_CELLO file for license and copyright information

/// @file     data_MethodState.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2025-06-11
/// @brief    [\ref Data] Declaration of the MethodState class

#ifndef DATA_METHOD_STATE_HPP
#define DATA_METHOD_STATE_HPP

class MethodState {
  /// @class    MethodState
  /// @ingroup  Data
  /// @brief    [\ref Data] States for individual methods

  friend State;

public:

  MethodState(int max_level = 0)
  {
    init(max_level);
  }

  void init(int max_level = 0) {
    dt_        = 0.0;
    time_      = 0.0;
    num_steps_ = 0;
    step_      = 0;

    dt_level_.       resize (max_level+1, 0.0);
    time_level_.     resize (max_level+1, 0.0);
    num_steps_level_.resize (max_level+1, 0);
    step_level_.     resize (max_level+1, 0);
  }

  void pup (PUP::er &p) {
    p | dt_;
    p | time_;
    p | num_steps_;
    p | step_;
    p | dt_level_;
    p | time_level_;
    p | num_steps_level_;
    p | step_level_;
  }

  //----------------------------------------------------------------------
  /// Accessor methods
  //----------------------------------------------------------------------

  double dt() const
  { return dt_; }
  double time() const
  { return time_; }
  int num_steps() const
  { return num_steps_; }
  int step() const
  { return step_; }


  void set_dt(double dt)
  { dt_ = dt; }
  void set_time(double time)
  { time_ = time; }
  void set_num_steps(int num_steps)
  { num_steps_ = num_steps; }
  void set_step(int step)
  { step_ = step; }

  double dt  (int level ) const 
  { return dt_level_[level]; }
  double time (int level ) const
  { return time_level_[level]; }
  int num_steps (int level ) const
  { return num_steps_level_[level]; }
  int step (int level ) const
  { return step_level_[level]; }

  void set_dt (double dt, int level )
  { dt_level_[level] = dt; }
  void set_time (double time, int level)
  { time_level_[level] = time; }
  void set_num_steps (int num_steps, int level )
  { num_steps_level_[level] = num_steps; }
  void set_step (int step, int level )
  { step_level_[level] = step; }


  //----------------------------------------------------------------------
  /// Packing / unpacking
  //----------------------------------------------------------------------

  int data_size () const
  {
    int size = 0;
    SIZE_SCALAR_TYPE(size,double,dt_);
    SIZE_SCALAR_TYPE(size,double,time_);
    SIZE_SCALAR_TYPE(size,int,num_steps_);
    SIZE_SCALAR_TYPE(size,int,step_);
    SIZE_VECTOR_TYPE(size,double,dt_level_);
    SIZE_VECTOR_TYPE(size,double,time_level_);
    SIZE_VECTOR_TYPE(size,int,num_steps_level_);
    SIZE_VECTOR_TYPE(size,int,step_level_);
    return size;
  }

  /// Serialize the object into the provided empty memory buffer.
  char * save_data (char * buffer) const
  {
    char * pc = buffer;
    SAVE_SCALAR_TYPE(pc,double,dt_);
    SAVE_SCALAR_TYPE(pc,double,time_);
    SAVE_SCALAR_TYPE(pc,int,num_steps_);
    SAVE_SCALAR_TYPE(pc,int,step_);
    SAVE_VECTOR_TYPE(pc,double,dt_level_);
    SAVE_VECTOR_TYPE(pc,double,time_level_);
    SAVE_VECTOR_TYPE(pc,int,num_steps_level_);
    SAVE_VECTOR_TYPE(pc,int,step_level_);
    return pc;
  }

  /// Restore the object from the provided initialized memory buffer data.
  char * load_data (char * buffer)
  {
    char * pc = buffer;
    LOAD_SCALAR_TYPE(pc,double,dt_);
    LOAD_SCALAR_TYPE(pc,double,time_);
    LOAD_SCALAR_TYPE(pc,int,num_steps_);
    LOAD_SCALAR_TYPE(pc,int,step_);
    LOAD_VECTOR_TYPE(pc,double,dt_level_);
    LOAD_VECTOR_TYPE(pc,double,time_level_);
    LOAD_VECTOR_TYPE(pc,int,num_steps_level_);
    LOAD_VECTOR_TYPE(pc,int,step_level_);
    return pc;
  }

protected:

  /// Method's timestep
  double dt_;
  /// Method's current time_    
  double time_;
  /// Number of steps expected for method ( > 1 for supercycling)
  int num_steps_;
  /// Number of steps remaining for method
  int step_;

  /// Method's current time_
  std::vector<double> time_level_;
  /// Method's timestep
  std::vector<double> dt_level_;
  /// Number of steps expected for method ( > 1 for supercycling)
  std::vector<int> num_steps_level_;
  /// Current supercycling step
  std::vector<int> step_level_;

};

#endif /* DATA_STATE_HPP */
