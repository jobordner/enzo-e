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
    time_.     resize (max_level+1, 0.0);
    dt_.       resize (max_level+1, 0.0);
    num_steps_.resize (max_level+1, 0);
    step_.     resize (max_level+1, 0);
  }

  void pup (PUP::er &p) {
    p | time_;
    p | dt_;
    p | num_steps_;
    p | step_;
  }

  //----------------------------------------------------------------------
  /// Accessor methods
  //----------------------------------------------------------------------

  void set_time (double time, int level = 0)
  { time_[level] = time; }

  double time (int level = 0) const
  { return time_[level]; }

  void set_dt (double dt, int level = 0)
  { dt_[level] = dt; }

  double dt  (int level = 0) const
  { return dt_[level]; }

  void set_num_steps (int num_steps, int level = 0)
  { num_steps_[level] = num_steps; }

  int num_steps (int level = 0) const
  { return num_steps_[level]; }

  void set_step (int step, int level = 0)
  { step_[level] = step; }

  int step (int level = 0) const
  { return step_[level]; }

  //----------------------------------------------------------------------
  /// Packing / unpacking
  //----------------------------------------------------------------------

  int data_size () const
  {
    int size = 0;
    SIZE_VECTOR_TYPE(size,double,time_);
    SIZE_VECTOR_TYPE(size,double,dt_);
    SIZE_VECTOR_TYPE(size,int,num_steps_);
    SIZE_VECTOR_TYPE(size,int,step_);
    return size;
  }

  /// Serialize the object into the provided empty memory buffer.
  char * save_data (char * buffer) const
  {
    char * pc = buffer;
    SAVE_VECTOR_TYPE(pc,double,time_);
    SAVE_VECTOR_TYPE(pc,double,dt_);
    SAVE_VECTOR_TYPE(pc,int,num_steps_);
    SAVE_VECTOR_TYPE(pc,int,step_);
    return pc;
  }

  /// Restore the object from the provided initialized memory buffer data.
  char * load_data (char * buffer)
  {
    char * pc = buffer;
    LOAD_VECTOR_TYPE(pc,double,time_);
    LOAD_VECTOR_TYPE(pc,double,dt_);
    LOAD_VECTOR_TYPE(pc,int,num_steps_);
    LOAD_VECTOR_TYPE(pc,int,step_);
    return pc;
  }

protected:

  /// Method's current time_
  std::vector<double> time_;

  /// Method's timestep
  std::vector<double> dt_;

  /// Number of steps expected for method ( > 1 for supercycling)
  std::vector<int> num_steps_;

  /// Current supercycling step
  std::vector<int> step_;

};

#endif /* DATA_STATE_HPP */
