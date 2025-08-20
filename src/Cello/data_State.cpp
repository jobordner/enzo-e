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
    set_(time_level_prev_,level,time_curr);
    set_(time_level_curr_,level,time);
  }
}

void State::advance(int level_top)
{
  ++cycle_;
  time_ += dt_;

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

    if (state_next_ == Next::Sequential) {

      int level=level_top;
      while (level > 0 && time_level_curr_[level] == time_level_curr_[level-1])
        level--;
      level_lower_ = level;
      level_upper_ = level + 1;

    } else if (state_next_ == Next::Concurrent) {

      int level=time_level_curr_.size() - 1;
      while (level > 0 && (time_level_curr_[level] == time_level_curr_[level-1]))
        level--;
      level_lower_ = level;
      level_upper_ = time_level_curr_.size();

    }
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


