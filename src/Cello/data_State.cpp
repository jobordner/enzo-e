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

void State::advance()
{
  ++cycle_;
  time_ += dt_;

  if (state_type_ == Type::Level) {

    for (int level=level_lower_; level<level_upper_; level++) {
      cycle_level_[level]++;
      const double time_next = time_level_curr_[level] + dt_level_[level];
      time_level_prev_[level] = time_level_curr_[level];
      time_level_curr_[level] = (level > 0) ?
        std::min(time_level_curr_[level-1],time_next) : time_next;
    }

    if (state_next_ == Next::Sequential) {

      int level=time_level_curr_.size() - 1;
      while (level >= 0 && time_level_curr_[level] == time_level_curr_[level-1])
        level--;
      level_lower_ = level;
      level_upper_ = level + 1;

    } else if (state_next_ == Next::Concurrent) {

      int level=time_level_curr_.size() - 1;
      while (level >= 0 && (time_level_curr_[level] == time_level_curr_[level-1]))
        level--;
      level_lower_ = level;
      level_upper_ = time_level_curr_.size();

    }
  }
}

//----------------------------------------------------------------------

void State::update_dt (std::vector<double> & dt_level)
{
  INCOMPLETE("State::update_dt()");
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


