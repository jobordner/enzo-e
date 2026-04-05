// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoState
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2024-06-14
/// @brief    State object including Enzo state, e.g. redshift

#include "Enzo/cosmology/cosmology.hpp"
#include "Enzo/enzo.hpp"

//----------------------------------------------------------------------

void EnzoState::advance (int level_top)
{
  State::advance(level_top);
  auto * cosmology = enzo::cosmology();
  if (cosmology) {
    // update block redshift
    redshift_ = cosmology->redshift_from_time(time_);
    // update level redshifts
    if (state_type_ == Type::Level) {
      for (int level=level_lower_; level<level_upper_; level++) {
        set_redshift
          (level,cosmology->redshift_from_time(time_level_curr_[level]));
      }
      const int level_max = time_level_curr_.size();
      for (int level=level_top+1; level<level_max; level++) {
        set_redshift(level,cosmology->redshift_from_time(time_level_curr_[level_top]));
      }
    }
  }
}

//----------------------------------------------------------------------

void EnzoState::set_time (double time)
{
  State::set_time(time);
  auto * cosmology = enzo::cosmology();
  if (cosmology) {
    redshift_ = cosmology->redshift_from_time(time);
  }
}

//----------------------------------------------------------------------

void EnzoState::init_time (double time)
{
  State::init_time(time);
  auto * cosmology = enzo::cosmology();
  if (cosmology) {
    redshift_ = cosmology->redshift_from_time(time);
  }
}

//----------------------------------------------------------------------

void EnzoState::set_time (double time, int level)
{
  State::set_time(time, level);
  auto * cosmology = enzo::cosmology();
  if (cosmology) {
    set_redshift(level,cosmology->redshift_from_time(time));
  }
}

//----------------------------------------------------------------------

void EnzoState::init_time (double time, int level)
{
  State::init_time(time, level);
  auto * cosmology = enzo::cosmology();
  if (cosmology) {
    set_redshift(level,cosmology->redshift_from_time(time));
  }
}

