// // See LICENSE_CELLO file for license and copyright information

// /// @file     enzo_EnzoRefineMass.cpp
// /// @author   James Bordner (jobordner@ucsd.edu)
// /// @date     2013-04-23
// /// @brief    Implementation of Enzo RefineMass class

#include "Enzo/mesh/mesh.hpp"
#include "Enzo/enzo.hpp"
#include "Cello/charm_simulation.hpp"
#include "enzo.decl.h"

//----------------------------------------------------------------------

EnzoRefineMass::EnzoRefineMass
(
 double min_refine,
 double max_coarsen,
 int    max_level,
 bool include_ghosts,
 std::string output,
 std::string name,
 std::string mass_type,
 double level_exponent) throw ()
  : Refine(min_refine,max_coarsen,max_level,include_ghosts,output),
    name_(name),
    mass_ratio_(0.0),
    level_exponent_(level_exponent)

{
  EnzoPhysicsCosmology * cosmology = enzo::cosmology();

  if (cosmology) {
    if (mass_type == "dark") {
      mass_ratio_ = cosmology->omega_cdm_now()
        /           cosmology->omega_matter_now();
    } else if (mass_type == "baryon") {
      mass_ratio_ = cosmology->omega_baryon_now()
        /           cosmology->omega_matter_now();
    } else {
      ERROR1 ("EnzoRefineMass::EnzoRefineMass()",
              "Unknown mass_type %s",  mass_type.c_str());
    }
  } else {
    mass_ratio_ = 0.0;
  }
}

//----------------------------------------------------------------------

int EnzoRefineMass::apply ( Block * block ) throw ()
{
  Field field = block->data()->field();
  int level = block->level();

  double hx,hy,hz;
  block->cell_width(&hx,&hy,&hz);

  double hx0 = hx*pow(2.0,level);
  double hy0 = hy*pow(2.0,level);
  double hz0 = hz*pow(2.0,level);

  double scale = (mass_ratio_ == 0.0) ? 1.0 :
    mass_ratio_*pow(2.0,level*level_exponent_)*hx0*hy0*hz0;
  double mass_min_refine  = scale*min_refine_;
  double mass_max_coarsen = scale*max_coarsen_;

  const int id_field = field.field_id(name_);
  ASSERT1 ("EnzoRefineMass::apply()",
           "Undefined field name %s",
           name_.c_str(), id_field >= 0);

  int mx,my,mz;
  int gx,gy,gz;
  field.dimensions (id_field, &mx,&my,&mz);
  field.ghost_depth(id_field, &gx,&gy,&gz);

  //  int num_fields = field_descr->field_count();

  bool all_coarsen = true;
  bool any_refine = false;

  cello_float * rho = field.values(id_field);
  cello_float * out = initialize_output_(field.field_data());

  double vol = hx*hy*hz;

  if (out) {
    for (int iz=gz; iz<mz-gz; iz++) {
      for (int iy=gy; iy<my-gy; iy++) {
        for (int ix=gx; ix<mx-gx; ix++) {
          int i = ix + mx*(iy + my*iz);
          double mass = vol*rho[i];
          if      (mass < mass_max_coarsen) out[i] = -1;
          else if (mass < mass_min_refine)  out[i] =  0;
          else                              out[i] = +1;
        }
      }
    }
  }
  for (int iz=gz; iz<mz-gz; iz++) {
    for (int iy=gy; iy<my-gy; iy++) {
      for (int ix=gx; ix<mx-gx; ix++) {
        int i = ix + mx*(iy + my*iz);
        double mass = vol*rho[i];
        if (mass > mass_min_refine)  any_refine  = true;
        if (mass > mass_max_coarsen) all_coarsen = false;
      }
    }
  }

  int adapt_result =
    any_refine ?  adapt_refine : (all_coarsen ? adapt_coarsen : adapt_same) ;

  // Don't refine if already at maximum level
  adjust_for_level_( &adapt_result, block->level() );

  return adapt_result;

}

//======================================================================

