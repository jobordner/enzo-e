// See LICENSE_CELLO file for license and copyright information

/// @file     problem_Method.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-09-04
/// @brief    

#include "problem.hpp"

double Method::courant_global = 1.0;

//----------------------------------------------------------------------

Method::Method (int index, double courant) throw()
  : schedule_(NULL),
    courant_(courant),
    neighbor_type_(neighbor_leaf),
    index_(index)
{
  Performance * performance = cello::simulation()->performance();
  const std::string name = std::string("perf_method_") + cello::config()->method_list[index];
  ASSERT2 ("Method::Method",
           "Method index %d greater than PERFORMANCE_MAX_METHOD_COUNT %d: increase limit in performance_Performance.hpp",
           index,PERFORMANCE_MAX_METHOD_COUNT,
           (index < PERFORMANCE_MAX_METHOD_COUNT));
  const int index_perf       = perf_method+2*index;
  const int index_perf_charm = perf_method+2*index + 1;
  performance->set_region_name(index_perf,name);
  performance->set_region_name(index_perf_charm,name+"_charm");

  ir_post_ = add_new_refresh_();
  cello::refresh(ir_post_)->set_callback(CkIndex_Block::p_compute_continue());
}

//----------------------------------------------------------------------

Method::~Method() throw()
{
  delete schedule_;
}

//----------------------------------------------------------------------

void Method::pup (PUP::er &p)
{ TRACEPUP;
  PUP::able::pup(p);

  p | schedule_; // pupable
  p | courant_;
  p | ir_post_;
  p | neighbor_type_;
  p | index_;

}

//----------------------------------------------------------------------

int Method::add_new_refresh_ (int neighbor_type)
{
  // set Method::ir_post_

  const int ghost_depth = std::max
    (std::max(cello::config()->field_ghost_depth[0],
              cello::config()->field_ghost_depth[0]),
     cello::config()->field_ghost_depth[0]);
  const int min_face_rank = cello::config()->adapt_min_face_rank;

  // Set default refresh object
  Refresh refresh_default
    (ghost_depth,min_face_rank, neighbor_type, sync_neighbor, 0);

  return cello::simulation()->new_register_refresh(refresh_default);
}

//----------------------------------------------------------------------

int Method::refresh_id_post() const
{
  return ir_post_;
}

//----------------------------------------------------------------------

void Method::set_schedule (Schedule * schedule) throw()
{ 
  if (schedule_) delete schedule_;
  schedule_ = schedule;
}

//======================================================================

