// See LICENSE_CELLO file for license and copyright information

/// @file     mesh_RefineDensity.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-07-31
/// @brief    Implementation of RefineDensity class

#include "mesh.hpp"

//----------------------------------------------------------------------

RefineDensity::RefineDensity
(
 double min_refine,
 double max_coarsen,
 int max_level,
 bool include_ghosts,
 std::string output) throw ()
  : Refine(min_refine,max_coarsen,max_level,include_ghosts,output)
{
  TRACE("RefineDensity::RefineDensity");
  WARNING ("RefineDensity::RefineDensity()",
	   "Assuming non-Cosmology problem for RefineDensity");

}

//----------------------------------------------------------------------

int RefineDensity::apply ( Block * block ) throw ()
{

  Field field = block->data()->field();

  int id = field.field_id ("density");

  int mx,my,mz;
  field.dimensions(id,&mx,&my,&mz);
  int gx,gy,gz;
  if (include_ghosts_) {
    gx = gy = gz = 0;
  } else {
    field.ghost_depth(id, &gx,&gy,&gz);
  }
  cello_float * array = field.values(id);

  bool any_refine  = false;
  bool all_coarsen = true;
  for (int iz=gz; iz<mz-gz; iz++) {
    for (int iy=gy; iy<my-gy; iy++) {
      for (int ix=gx; ix<mx-gx; ix++) {
	int i = ix + mx*(iy + my*iz);
	if (array[i] > min_refine_)  any_refine  = true;
	if (array[i] > max_coarsen_) all_coarsen = false;
      }
    }
  }
  int adapt_result = any_refine ?
    adapt_refine : (all_coarsen ? adapt_coarsen : adapt_same) ;

  // Don't refine if already at maximum level
  adjust_for_level_( &adapt_result, block->level() );
  return adapt_result;
}

//======================================================================

