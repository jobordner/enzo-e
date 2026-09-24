// See LICENSE_CELLO file for license and copyright information

/// @file     mesh_Refine.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2014-08-18
/// @brief Implementation of the Refine base class for mesh refinement
///        criteria

#include "mesh.hpp"
#include "charm_simulation.hpp"


void Refine::pup (PUP::er &p)
{
  TRACEPUP;
  PUP::able::pup(p);
  // NOTE: change this function whenever attributes change
  p | min_refine_;
  p | max_coarsen_;
  p | max_level_;
  p | include_ghosts_;
  p | schedule_;
  p | output_;
}

//----------------------------------------------------------------------

void Refine::set_schedule (Schedule * schedule) throw()
{ 
  if (schedule_) delete schedule_;
  schedule_ = schedule;
}

//----------------------------------------------------------------------

cello_float * Refine::initialize_output_(FieldData * field_data)
{
  cello_float * output = nullptr;
  const bool do_output = output_ != "";

  if (do_output) {

    Field field (cello::field_descr(),field_data);

    const int id_output = field.field_id(output_);
    output = field.values(id_output);
    const int m = field.dimensions(id_output);
    std::fill_n(output,m,-1.0);
  }
  return output;
}
