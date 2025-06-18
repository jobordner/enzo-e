// See LICENSE_CELLO file for license and copyright information

/// @file     data_State.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     Mon Jun 16 03:42:03 PM PDT 2025
/// @brief    [\ref Data] Declaration of the State class


#include "cello.hpp"
#include "data.hpp"

//----------------------------------------------------------------------

void State::advance()
{
  if (state_type_ == Type::Global) {
    cycle_++;
    time_+= dt_;
  } else if (state_type_ == Type::Level) {
    for (int level=level_lower_; level<level_upper_; level++) {
      cycle_level_[level]++;
      time_level_[level] += dt_level_[level];
    }
  }
}

//----------------------------------------------------------------------

bool State::is_active ( int level )
{
  bool retval = true;
  if (state_type_ == Type::Level) {
    retval = (level_lower_ <= level) && (level < level_upper_);
  }
  return retval;
}

//----------------------------------------------------------------------

bool State::in_barrier ( int level )
{
  bool retval = true;
  if (state_type_ == Type::Level) {
    retval = (level_lower_ <= level) && (level < level_upper_);
  }
  return retval;
}


