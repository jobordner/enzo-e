// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodNull.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2019-06-13

#include "problem.hpp"

void MethodNull::compute( Block * block) throw()
{
  block->compute_done();
}

//======================================================================

void MethodNull::init_refresh_()
{
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_all_fields();
  refresh->add_all_particles();
  refresh->set_global();
  refresh->set_advanced_time (advanced_time_);
  refresh->set_min_face_rank(cello::rank()-1);
  //  refresh->set_ghost_depth(2);
  refresh->set_final_sync();
}
