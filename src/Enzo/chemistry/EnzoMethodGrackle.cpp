// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMethodGrackle.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
///           Andrew Emerick (aemerick11@gmail.com)
/// @date     Tues May  7
/// @brief    Implements the EnzoMethodGrackle class

#include "cello.hpp"
#include "enzo.hpp"


//----------------------------------------------------------------------------

EnzoMethodGrackle::EnzoMethodGrackle
(
 GrackleChemistryData my_chemistry,
 bool use_cooling_timestep,
 const double radiation_redshift,
 const double physics_cosmology_initial_redshift,
 const double time
)
  : Method(),
    grackle_facade_(std::move(my_chemistry), radiation_redshift,
                    physics_cosmology_initial_redshift, time),
    use_cooling_timestep_(use_cooling_timestep)
{

  // Gather list of fields that MUST be defined for this
  // method and check that they are permanent. If not,
  // define them.

  define_required_grackle_fields();

  /// Initialize default Refresh
  cello::simulation()->refresh_set_name(ir_post_,name());
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_all_fields();

}

//----------------------------------------------------------------------
 
void EnzoMethodGrackle::define_required_grackle_fields
()
{
  // Gather list of fields that MUST be defined for this method and
  // check that they are permanent. If not, define them.

  // This has been split off from the constructor so that other methods that
  // are initialized first and need knowledge of these fields at initialization
  // (e.g. to set up a refresh object), can ensure that the fields are defined

  const GrackleChemistryData* my_chemistry = this->try_get_chemistry();

  if ((my_chemistry == nullptr) ||
      (my_chemistry->get<int>("use_grackle") == 0)){
    return;
  }

  // special container for ensuring color fields are properly grouped

  std::vector<std::string> fields_to_define;
  std::vector<std::string> color_fields;

  const int rank = cello::rank();

  cello::define_field ("density");
  cello::define_field ("internal_energy");
  cello::define_field ("total_energy");

  if (rank>=1) cello::define_field ("velocity_x");
  if (rank>=2) cello::define_field ("velocity_y");
  if (rank>=3) cello::define_field ("velocity_z");

  // Get Grackle parameters defining fields to define

  const int metal_cooling = my_chemistry->get<int>("metal_cooling");
  const int chemistry_level = my_chemistry->get<int>("primordial_chemistry");
  const int specific_heating
    = my_chemistry->get<int>("use_specific_heating_rate");
  const int volumetric_heating
    = my_chemistry->get<int>("use_volumetric_heating_rate");
  const int radiative_transfer
      = my_chemistry->get<int>("use_radiative_transfer");

  // Metal cooling fields

  if (metal_cooling > 0) {
    cello::define_field_in_group ("metal_density", "color");
  }

  // Primordial chemistry fields

  if (chemistry_level >= 1) {
    cello::define_field_in_group ("HI_density",    "color");
    cello::define_field_in_group ("HII_density",   "color");
    cello::define_field_in_group ("HeI_density",   "color");
    cello::define_field_in_group ("HeII_density",  "color");
    cello::define_field_in_group ("HeIII_density", "color");
    cello::define_field_in_group ("e_density",     "color");
  }

  if (chemistry_level >= 2) {
    cello::define_field_in_group ("HM_density",   "color");
    cello::define_field_in_group ("H2I_density",  "color");
    cello::define_field_in_group ("H2II_density", "color");
  }

  if (chemistry_level >= 3) {
    cello::define_field_in_group ("DI_density",  "color");
    cello::define_field_in_group ("DII_density", "color");
    cello::define_field_in_group ("HDI_density", "color");
  }

  if (radiative_transfer) {
    cello::define_field("RT_heating_rate");
    cello::define_field("RT_HI_ionization_rate");
    cello::define_field("RT_HeI_ionization_rate");
    cello::define_field("RT_HeII_ionization_rate");
    cello::define_field("RT_H2_dissociation_rate");
  }

  if (specific_heating) {
    cello::define_field("specific_heating_rate");
  }

  if (volumetric_heating) {
    cello::define_field("volumetric_heating_rate");
  }

}

//----------------------------------------------------------------------

void EnzoMethodGrackle::compute ( Block * block) throw()
{
  if (block->is_leaf()){

#ifndef CONFIG_USE_GRACKLE

    ERROR("EnzoMethodGrackle::compute()",
          "Can't use method 'grackle' when Enzo-E isn't linked to Grackle");

#else /* CONFIG_USE_GRACKLE */

    EnzoBlock * enzo_block = enzo::block(block);

    this->compute_(enzo_block);
#endif
  }

  block->compute_done();

  return;

}

//----------------------------------------------------------------------------

void EnzoMethodGrackle::update_grackle_density_fields(
                               Block * block,
                               grackle_field_data * grackle_fields
                               ) const throw() {
#ifndef CONFIG_USE_GRACKLE
  ERROR("EnzoMethodGrackle::update_grackle_density_fields",
        "Enzo-E isn't linked to grackle");
#else

  // Intended for use at problem initialization. Scale species
  // density fields to be sensible mass fractions of the initial
  // density field. Problem types that require finer-tuned control
  // over individual species fields should adapt this function
  // in their initialization routines.

  const GrackleChemistryData* my_chemistry = this->try_get_chemistry();
  ASSERT("update_grackle_density_fields", "not configured to use grackle",
         my_chemistry != nullptr);

  grackle_field_data tmp_grackle_fields;
  bool cleanup_grackle_fields = false;
  if (grackle_fields == nullptr){
    grackle_fields = &tmp_grackle_fields;
    setup_grackle_fields(block, grackle_fields);
    cleanup_grackle_fields = true;
  }

  Field field = block->data()->field();

  int gx,gy,gz;
  field.ghost_depth (0,&gx,&gy,&gz);

  int nx,ny,nz;
  field.size (&nx,&ny,&nz);

  int ngx = nx + 2*gx;
  int ngy = ny + 2*gy;
  int ngz = nz + 2*gz;

  const double tiny_number = 1.0E-10;

  const EnzoFluidFloorConfig& fluid_floors
    = enzo::fluid_props()->fluid_floor_config();
  const enzo_float metal_factor = fluid_floors.has_metal_mass_frac_floor()
    ? fluid_floors.metal_mass_frac() : (enzo_float)tiny_number;

  const int primordial_chemistry
    = my_chemistry->get<int>("primordial_chemistry");
  const int metal_cooling  = my_chemistry->get<int>("metal_cooling");
  const double HydrogenFractionByMass
    = my_chemistry->get<double>("HydrogenFractionByMass");
  const double DeuteriumToHydrogenRatio
    = my_chemistry->get<double>("DeuteriumToHydrogenRatio");

  for (int iz = 0; iz<ngz; iz++){
    for (int iy=0; iy<ngy; iy++){
      for (int ix=0; ix<ngx; ix++){
        int i = INDEX(ix,iy,iz,ngx,ngy);

        if(primordial_chemistry > 0){
          grackle_fields->HI_density[i]   = grackle_fields->density[i] * HydrogenFractionByMass;
          grackle_fields->HII_density[i]   = grackle_fields->density[i] * tiny_number;
          grackle_fields->HeI_density[i]   = grackle_fields->density[i] * (1.0 - HydrogenFractionByMass);
          grackle_fields->HeII_density[i]  = grackle_fields->density[i] * tiny_number;
          grackle_fields->HeIII_density[i] = grackle_fields->density[i] * tiny_number;
          grackle_fields->e_density[i]     = grackle_fields->density[i] * tiny_number;
        }

        if (primordial_chemistry > 1){
          grackle_fields->HM_density[i]    = grackle_fields->density[i] * tiny_number;
          grackle_fields->H2I_density[i]   = grackle_fields->density[i] * tiny_number;
          grackle_fields->H2II_density[i]  = grackle_fields->density[i] * tiny_number;
        }

        if (primordial_chemistry > 2){
          grackle_fields->DI_density[i]    = grackle_fields->density[i] * DeuteriumToHydrogenRatio;
          grackle_fields->DII_density[i]   = grackle_fields->density[i] * tiny_number;
          grackle_fields->HDI_density[i]   = grackle_fields->density[i] * tiny_number;
        }

       if (metal_cooling == 1){
          grackle_fields->metal_density[i] = grackle_fields->density[i] * metal_factor;
       }

      }
    }
  }

  if (cleanup_grackle_fields){
    EnzoMethodGrackle::delete_grackle_fields(grackle_fields);
  }

  return;

#endif // CONFIG_USE_GRACKLE
}

//----------------------------------------------------------------------

void EnzoMethodGrackle::compute_ ( Block * block) throw()
{
#ifndef CONFIG_USE_GRACKLE
  ERROR("EnzoMethodGrackle::compute_", "Enzo-E isn't linked to grackle");
#else
  const EnzoConfig * enzo_config = enzo::config();
  if (block->cycle() == enzo_config->initial_cycle) {
    bool nohydro = ( (enzo::problem()->method("ppm") == nullptr) |
                     (enzo::problem()->method("mhd_vlct") == nullptr) |
                     (enzo::problem()->method("ppml") == nullptr) );

    ASSERT("EnzoMethodGrackle::compute_",
           "The current implementation requires the dual-energy formalism to "
           "be in use, when EnzoMethodGrackle is used with a (M)HD-solver",
           nohydro | !enzo::fluid_props()->dual_energy_config().is_disabled());
  }

  // Solve chemistry
  // NOTE: should we set compute_time to `block->time() + 0.5*block->dt()`?
  //       I think that's what enzo-classic does...
  double compute_time = block->time(); // only matters in cosmological sims
  grackle_facade_.solve_chemistry(block, compute_time, block->dt());

  // now we have to do some extra-work after the fact (such as adjusting total
  // energy density and applying floors...)

  // todo: avoid constructing this instance of grackle_fields
  grackle_field_data grackle_fields;
  setup_grackle_fields(EnzoFieldAdaptor(block,0), &grackle_fields);

  Field field = block->data()->field();

  int gx,gy,gz;
  field.ghost_depth (0,&gx,&gy,&gz);

  int nx,ny,nz;
  field.size (&nx,&ny,&nz);

  int ngx = nx + 2*gx;
  int ngy = ny + 2*gy;
  int ngz = nz + 2*gz;

  const int rank = cello::rank();

  // enforce metallicity floor (if one was provided)
  enforce_metallicity_floor(block);

  /* Correct total energy for changes in internal energy */
  gr_float * v3[3];
  v3[0] = grackle_fields.x_velocity;
  v3[1] = grackle_fields.y_velocity;
  v3[2] = grackle_fields.z_velocity;

  const bool mhd = field.is_field("bfield_x");
  enzo_float * b3[3] = {NULL, NULL, NULL};
  if (mhd) {
    b3[0]                = (enzo_float*) field.values("bfield_x");
    if (rank >= 2) b3[1] = (enzo_float*) field.values("bfield_y");
    if (rank >= 3) b3[2] = (enzo_float*) field.values("bfield_z");
  }

  enzo_float * total_energy    = (enzo_float *) field.values("total_energy");
  for (int i = 0; i < ngx*ngy*ngz; i++){
    total_energy[i] = grackle_fields.internal_energy[i];

    enzo_float inv_density;
    if (mhd) inv_density = 1.0 / grackle_fields.density[i];
    for (int dim = 0; dim < rank; dim++){
      total_energy[i] += 0.5 * v3[dim][i] * v3[dim][i];
      if (mhd) total_energy[i] += 0.5 * b3[dim][i] * b3[dim][i] * inv_density;
    }
  }

  // For testing purposes - reset internal energies with changes in mu
  if (enzo_config->initial_grackle_test_reset_energies){
    this->ResetEnergies(block);
  }

  delete_grackle_fields(&grackle_fields);

  return;
#endif // CONFIG_USE_GRACKLE
}

//----------------------------------------------------------------------

double EnzoMethodGrackle::timestep ( Block * block ) throw()
{
  double dt = std::numeric_limits<double>::max();;

  if (use_cooling_timestep_){
    Field field = block->data()->field();

    enzo_float * cooling_time = field.is_field("cooling_time") ?
                        (enzo_float *) field.values("cooling_time") : NULL;

    // make it if it doesn't exist
    bool delete_cooling_time = false;
    int gx,gy,gz;
    field.ghost_depth (0,&gx,&gy,&gz);

    int nx,ny,nz;
    field.size (&nx,&ny,&nz);

    int ngx = nx + 2*gx;
    int ngy = ny + 2*gy;
    int ngz = nz + 2*gz;

    int size = ngx*ngy*ngz;

    if (!(cooling_time)){
      cooling_time = new enzo_float [size];
      delete_cooling_time = true;
    }

    this->calculate_cooling_time(EnzoFieldAdaptor(block,0), cooling_time, 0);

    // make sure to exclude the ghost zone. Because there is no refresh before
    // this method is called (at least during the very first cycle) - this can
    // including ghost zones can lead to timesteps of 0
    for (int iz = gz; iz < ngz - gz; iz++) {   // if rank < 3: gz = 0, ngz = 1
      for (int iy = gy; iy < ngy - gy; iy++) { // if rank < 2: gy = 0, ngy = 1
        for (int ix = gx; ix < ngx - gx; ix++) {
	  int i = INDEX(ix, iy, iz, ngx, ngy);
          dt = std::min(enzo_float(dt), std::abs(cooling_time[i]));
        }
      }
    }

    if (delete_cooling_time){
      delete [] cooling_time;
    }
  }

  return dt * courant_;
}

//----------------------------------------------------------------------

void EnzoMethodGrackle::enforce_metallicity_floor(Block * block) throw()
{
  const EnzoFluidFloorConfig& fluid_floors
    = enzo::fluid_props()->fluid_floor_config();

  if (!fluid_floors.has_metal_mass_frac_floor()){
    return; // return early if the floor has not been defined
  }

  const enzo_float metal_mass_frac_floor = fluid_floors.metal_mass_frac();

  // MUST have metal_density field tracked
  Field field = block->data()->field();
  enzo_float * density = (enzo_float*) field.values("density");
  enzo_float * metal_density  = (enzo_float*) field.values("metal_density");
  ASSERT("EnzoMethodGrackle::enforce_metallicity_floor",
         ("Can't enforce metallicity floor when the \"metal_density\" field "
          "doesn't exist"),
         metal_density != nullptr);

  int gx,gy,gz;
  field.ghost_depth (0,&gx,&gy,&gz);

  int nx,ny,nz;
  field.size (&nx,&ny,&nz);

  int ngx = nx + 2*gx;
  int ngy = ny + 2*gy;
  int ngz = nz + 2*gz;

  for (int iz=0; iz<ngz; iz++){
    for (int iy=0; iy<ngy; iy++){
      for (int ix=0; ix<ngx; ix++){
        int i = INDEX(ix,iy,iz,ngx,ngy);
        metal_density[i] = std::max(metal_density[i],
                                    metal_mass_frac_floor * density[i]);
      }
    }
  }
  return;
}

//----------------------------------------------------------------------

void EnzoMethodGrackle::ResetEnergies ( Block * block) throw()
{
   const EnzoConfig * enzo_config = enzo::config();
   EnzoUnits * enzo_units = enzo::units();

   /* Only need to do this if tracking chemistry */
   const GrackleChemistryData* my_chemistry = this->try_get_chemistry();
   ASSERT("update_grackle_density_fields", "not configured to use grackle",
          my_chemistry != nullptr);
   if (my_chemistry->get<int>("primordial_chemistry") < 1)
     return;

   Field field = block->data()->field();

   enzo_float * density     = (enzo_float*) field.values("density");
   enzo_float * internal_energy = (enzo_float*) field.values("internal_energy");
   enzo_float * total_energy    = (enzo_float*) field.values("total_energy");

   enzo_float * HI_density    = (enzo_float*) field.values("HI_density");
   enzo_float * HII_density   = (enzo_float*) field.values("HII_density");
   enzo_float * HeI_density   = (enzo_float*) field.values("HeI_density");
   enzo_float * HeII_density  = (enzo_float*) field.values("HeII_density");
   enzo_float * HeIII_density = (enzo_float*) field.values("HeIII_density");
   enzo_float * e_density     = (enzo_float*) field.values("e_density");

   enzo_float * H2I_density   = (enzo_float*) field.values("H2I_density");
   enzo_float * H2II_density  = (enzo_float*) field.values("H2II_density");
   enzo_float * HM_density    = (enzo_float*) field.values("HM_density");

   enzo_float * DI_density    = (enzo_float *) field.values("DI_density");
   enzo_float * DII_density   = (enzo_float *) field.values("DII_density");
   enzo_float * HDI_density   = (enzo_float *) field.values("HDI_density");


   enzo_float * metal_density = (enzo_float*) field.values("metal_density");

   // Field size
   int nx,ny,nz;
   field.size(&nx,&ny,&nz);

   // Cell widths
   double xm,ym,zm;
   block->data()->lower(&xm,&ym,&zm);
   double xp,yp,zp;
   block->data()->upper(&xp,&yp,&zp);

   // Ghost depths
   int gx,gy,gz;
   field.ghost_depth(0,&gx,&gy,&gz);

   int mx,my,mz;
   field.dimensions(0,&mx,&my,&mz);

   double temperature_slope = log10
     (enzo_config->initial_grackle_test_maximum_temperature/
      enzo_config->initial_grackle_test_minimum_temperature) / double(ny);

   const enzo_float nominal_gamma = enzo::fluid_props()->gamma();
   const int primordial_chemistry
     = my_chemistry->get<int>("primordial_chemistry");

   if (primordial_chemistry == 0){
     ERROR("EnzoMethodGrackle::ResetEnergies",
           "This function is not currently equipped to handle the case with "
           "primordial_chemistry == 0");
   }

   for (int iz=gz; iz<nz+gz; iz++){ // Metallicity
     for (int iy=gy; iy<ny+gy; iy++) { // Temperature
       for (int ix=gx; ix<nx+gx; ix++) { // H Number Density
         int i = INDEX(ix,iy,iz,mx,my);

         enzo_float mu = e_density[i] + HI_density[i] + HII_density[i] +
            (HeI_density[i] + HeII_density[i] + HeIII_density[i])*0.25;

         if (primordial_chemistry > 1){
           mu += HM_density[i] + 0.5 * (H2I_density[i] + H2II_density[i]);
         }

         if (primordial_chemistry > 2){
           mu += (DI_density[i] + DII_density[i])*0.5 + HDI_density[i]/3.0;
         }

         if (metal_density != nullptr){
           mu += metal_density[i] / 16.0;
         }

         mu = density[i] / mu;

         internal_energy[i] =
           (pow(10.0, ((temperature_slope * (iy-gy)) +
                       log10(enzo_config->initial_grackle_test_minimum_temperature)))/
            mu / enzo_units->kelvin_per_energy_units() /
            (nominal_gamma - 1.0));
         total_energy[i] = internal_energy[i];

       }
     }
   }

  return;
}

//======================================================================
