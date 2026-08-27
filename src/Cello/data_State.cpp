// See LICENSE_CELLO file for license and copyright information

/// @file     data_State.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     Mon Jun 16 03:42:03 PM PDT 2025
/// @brief    [\ref Data] Declaration of the State class


#include "cello.hpp"
#include "data.hpp"

//----------------------------------------------------------------------

void State::set_time (double time, int level)
{
  double time_curr = time_level_curr_[level];
  if (time > time_curr) {
    set_(time_level_curr_,level,time);
  }
}

//----------------------------------------------------------------------

void State::init_time (double time, int level)
{
  set_(time_level_prev_,level,time);
  set_(time_level_curr_,level,time);
}

//----------------------------------------------------------------------

void State::set_cycle(int cycle)
{
  cycle_ = cycle;
  if (state_type_ == Type::Level) {
    for (size_t level = 0; level<cycle_level_.size(); level++) {
      set_cycle(cycle,level);
    }
  }
}

//----------------------------------------------------------------------

double State::time(int level) const
{
  return (state_type_ == Type::Global) ?
    time_ : time_level_curr_[level];
}

//----------------------------------------------------------------------

void State::set_time (double time)
{
  time_ = time;
  if (state_type_ == Type::Level) {
    for (size_t level = 0; level<time_level_curr_.size(); level++) {
      set_time (time,level);
    }
  }
}

//----------------------------------------------------------------------

void State::init_time (double time)
{
  time_ = time;
  if (state_type_ == Type::Level) {
    for (size_t level = 0; level<time_level_curr_.size(); level++) {
      init_time (time,level);
    }
  }
}

//----------------------------------------------------------------------

void State::set_dt (double dt)
{
  dt_ = dt;
  if (state_type_ == Type::Level) {
    for (size_t level = 0; level<dt_level_.size(); level++) {
      set_dt (dt,level);
    }
  }
}

//----------------------------------------------------------------------

void State::advance()
{
  ++cycle_;
  time_ += dt_;

  const int level_top = cello::level_top();

  if (state_type_ == Type::Level) {

    // update level cycles and level times for active levels
    for (int level=level_lower_; level<level_upper_; level++) {
      cycle_level_[level]++;
      const double time_next = time_level_curr_[level] + dt_level_[level];
      time_level_prev_[level] = time_level_curr_[level];
      time_level_curr_[level] = time_next;
    }

    // update level cycles and level times for empty levels
    const int level_max = time_level_curr_.size();

    for (int level=level_top+1; level<level_max; level++) {
      cycle_level_[level] = cycle_level_[level_top];
      time_level_prev_[level] = time_level_prev_[level_top];
      time_level_curr_[level] = time_level_curr_[level_top];
    }

    // Find finest level whose current time != next-coarser level time

    const int level_root = cello::level_root();

    int level = (state_next_ == Next::Sequential) ?
      level_top : level_max - 1;
    while ((level > level_root) &&
           (time_level_curr_[level] == time_level_curr_[level-1]))
      level--;

    level_lower_ = level;

    level_upper_ = (state_next_ == Next::Sequential) ?
      level + 1 : time_level_curr_.size();

  }
}

//----------------------------------------------------------------------

bool State::is_active ( int level ) const
{
  bool retval = true;
  
  if (state_type_ == Type::Level) {
    retval = (level_lower_ <= level) && (level < level_upper_);
  }
  return retval;
}

//----------------------------------------------------------------------

bool State::in_barrier ( int level ) const
{
  bool retval = true;
  if (state_type_ == Type::Level) {
    retval = (level_lower_ <= level) && (level < level_upper_);
  }
  return retval;
}


