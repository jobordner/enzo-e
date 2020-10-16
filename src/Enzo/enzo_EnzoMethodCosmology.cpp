// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMethodCosmology.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2017-07-24
/// @brief    Implementation of the EnzoMethodCosmology class

#include "cello.hpp"
#include "charm_simulation.hpp"
#include "enzo.hpp"

// #define DEBUG_COSMO

//----------------------------------------------------------------------

EnzoMethodCosmology::EnzoMethodCosmology(int index) throw()
  : Method (index)
{
}

//----------------------------------------------------------------------

void EnzoMethodCosmology::compute(Block * block) throw()
{
  const int index_perf = perf_method + 2*index_;
  block->performance_start(index_perf);

  auto cosmology = enzo::cosmology();

#ifdef DEBUG_COSMO  
  cosmology->print();
#endif  

  // Monitor current redshift
  Monitor * monitor = cello::monitor();
  if (block->index().is_root()) {
    monitor->print("Method", "%s redshift %.8f",
		   this->name().c_str(),
		   cosmology->current_redshift());
  }

  block->performance_stop(index_perf);
  block->compute_done(); 
}
