// See LICENSE_ENZO file for license and copyright information

/***********************************************************************
/
/  GRID CLASS (SOLVE THE MHD EQUATIONS (with PPML), SAVING SUBGRID FLUXES)
/
/  written by: Greg Bryan
/  date:       November, 1994
/  modified1:  Alexei Kritsuk, April 2009 (PPML)
/
/  PURPOSE:
/
/  RETURNS:
/    ENZO_SUCCESS or ENZO_FAIL
/
************************************************************************/

// Solve the MHD equations with the solver, saving the subgrid fluxes

/// When this function was first ported to Enzo-E, it was an instance method of
/// the EnzoBlock class. However as the codebase has matured, it has become
/// customary for integrators to be encapsulated by Method Objects. As such
/// this function has been converted to a static method of EnzoMethodPpml.
///
/// In the future, it probably makes sense to consolidate this file with
/// EnzoMethodPpml.cpp (this consolidation has been left to a separate PR from
/// the one where this function was removed from the EnzoBlock class to make
/// merge conflicts easier to address)

#include "Cello/cello.hpp"
#include "Enzo/enzo.hpp"
#include "Enzo/hydro-mhd/hydro-mhd.hpp"

#include "Enzo/hydro-mhd/ppml_fortran/ppml_fortran.hpp" // FORTRAN_NAME(ppml)

int EnzoMethodPpml::SolveMHDEquations( EnzoBlock& block, enzo_float dt )
{

  /* exit if not 3D */

  const int GridRank = cello::rank();
  ASSERT("EnzoBlock::SolveMHDEquations",
         "This function requires the domain's rank to be 3",
         GridRank == 3);

  Field field = block.data()->field();
  if (field.num_permanent() > 0) { // TODO: revisit if-clause. This could be
                                   // improved. (plus we probably want to
                                   // report an error when false)

    // load the position of the lower left edge of the grid.
    //
    // TODO: make DomainLeftEdge hold values in double precision (we don't
    // currently do that to avoid breaking tests)
    std::array<double,3> lo_arr_;
    cello::hierarchy()->lower(&lo_arr_[0], &lo_arr_[1], &lo_arr_[2]);

    const std::array<enzo_float, 3> DomainLeftEdge = {enzo_float(lo_arr_[0]),
                                                      enzo_float(lo_arr_[1]),
                                                      enzo_float(lo_arr_[2])};


    /* initialize */

    int dim, i,  size;

    size = field.dimensions(field.field_id("density"));

    /* Get easy to handle pointers for each variable. */

    enzo_float *density    = (enzo_float *) field.values ("density");
    enzo_float *velox      = (enzo_float *) field.values ("velox");
    enzo_float *veloy      = (enzo_float *) field.values ("veloy");
    enzo_float *veloz      = (enzo_float *) field.values ("veloz");
    enzo_float *bfieldx    = (enzo_float *) field.values ("bfieldx");
    enzo_float *bfieldy    = (enzo_float *) field.values ("bfieldy");
    enzo_float *bfieldz    = (enzo_float *) field.values ("bfieldz");
    enzo_float *dens_rx    = (enzo_float *) field.values ("dens_rx");
    enzo_float *velox_rx   = (enzo_float *) field.values ("velox_rx");
    enzo_float *veloy_rx   = (enzo_float *) field.values ("veloy_rx");
    enzo_float *veloz_rx   = (enzo_float *) field.values ("veloz_rx");
    enzo_float *bfieldx_rx = (enzo_float *) field.values ("bfieldx_rx");
    enzo_float *bfieldy_rx = (enzo_float *) field.values ("bfieldy_rx");
    enzo_float *bfieldz_rx = (enzo_float *) field.values ("bfieldz_rx");

    enzo_float *dens_ry    = (enzo_float *) field.values ("dens_ry");
    enzo_float *velox_ry   = (enzo_float *) field.values ("velox_ry");
    enzo_float *veloy_ry   = (enzo_float *) field.values ("veloy_ry");
    enzo_float *veloz_ry   = (enzo_float *) field.values ("veloz_ry");
    enzo_float *bfieldx_ry = (enzo_float *) field.values ("bfieldx_ry");
    enzo_float *bfieldy_ry = (enzo_float *) field.values ("bfieldy_ry");
    enzo_float *bfieldz_ry = (enzo_float *) field.values ("bfieldz_ry");

    enzo_float *dens_rz    = (enzo_float *) field.values ("dens_rz");
    enzo_float *velox_rz   = (enzo_float *) field.values ("velox_rz");
    enzo_float *veloy_rz   = (enzo_float *) field.values ("veloy_rz");
    enzo_float *veloz_rz   = (enzo_float *) field.values ("veloz_rz");
    enzo_float *bfieldx_rz = (enzo_float *) field.values ("bfieldx_rz");
    enzo_float *bfieldy_rz = (enzo_float *) field.values ("bfieldy_rz");
    enzo_float *bfieldz_rz = (enzo_float *) field.values ("bfieldz_rz");

    /* compute global start index for left edge of entire grid
       (including boundary zones) */

    enzo_float lower[3];
    block.lower(lower,lower+1,lower+2);
    int im3[3],ip3[3];

    {
      int g3[3],m3[3];
      enzo_float h3[3];
      const int id = field.field_id("density");
      field.ghost_depth(id,g3,g3+1,g3+2);
      field.dimensions (id,m3,m3+1,m3+2);
      block.cell_width(h3,h3+1,h3+2);
      for (dim = 0; dim < GridRank; dim++) {
        im3[dim]=g3[dim];
        ip3[dim]=m3[dim]-g3[dim]-1;
      }
    }

    /* allocate temporary space for solver */
    int k = 0;
    enzo_float *temp = new enzo_float[size*(31)];
    enzo_float *f1 = &temp[k*size];  k++;
    enzo_float *f2 = &temp[k*size];  k++;
    enzo_float *f3 = &temp[k*size];  k++;
    enzo_float *f4 = &temp[k*size];  k++;
    enzo_float *f5 = &temp[k*size];  k++;
    enzo_float *f6 = &temp[k*size];  k++;
    enzo_float *f7 = &temp[k*size];  k++;
    enzo_float *g1 = &temp[k*size];  k++;
    enzo_float *g2 = &temp[k*size];  k++;
    enzo_float *g3 = &temp[k*size];  k++;
    enzo_float *g4 = &temp[k*size];  k++;
    enzo_float *g5 = &temp[k*size];  k++;
    enzo_float *g6 = &temp[k*size];  k++;
    enzo_float *g7 = &temp[k*size];  k++;
    enzo_float *h1 = &temp[k*size];  k++;
    enzo_float *h2 = &temp[k*size];  k++;
    enzo_float *h3 = &temp[k*size];  k++;
    enzo_float *h4 = &temp[k*size];  k++;
    enzo_float *h5 = &temp[k*size];  k++;
    enzo_float *h6 = &temp[k*size];  k++;
    enzo_float *h7 = &temp[k*size];  k++;
    enzo_float *ex = &temp[k*size];  k++;
    enzo_float *ey = &temp[k*size];  k++;
    enzo_float *ez = &temp[k*size];  k++;
    enzo_float *qu1 = &temp[k*size];  k++;
    enzo_float *qu2 = &temp[k*size];  k++;
    enzo_float *qu3 = &temp[k*size];  k++;
    enzo_float *qu4 = &temp[k*size];  k++;
    enzo_float *qu5 = &temp[k*size];  k++;
    enzo_float *qu6 = &temp[k*size];  k++;
    enzo_float *qu7 = &temp[k*size];  k++;

    ASSERT ("EnzoBlock::SolveMHDEquations",
	    "Insufficient temporary storage",
	    k <= 31);
    /* create and fill in arrays which are easiler for the solver to
       understand. */

    int NumberOfSubgrids = 0; // JB

    enzo_float *standard = NULL;

    int *leftface  = NULL;
    int *rightface =  NULL;
    int *istart    =  NULL;
    int *jstart    =  NULL;
    int *iend      =  NULL;
    int *jend      =  NULL;
    int *dnindex   =  NULL;
    int *vxindex   =  NULL;
    int *vyindex   =  NULL;
    int *vzindex   =  NULL;
    int *bxindex   =  NULL;
    int *byindex   =  NULL;
    int *bzindex   =  NULL;

    /* current PPML implementation only supports 3D and does not
       support color fields */

    int mx,my,mz;
    field.dimensions(field.field_id("density"),&mx,&my,&mz);
    enzo_float hx,hy,hz;
    block.cell_width(&hx,&hy,&hz);
      FORTRAN_NAME(ppml)
	(density,velox,   veloy,   veloz,   bfieldx,   bfieldy,   bfieldz,
	 dens_rx,velox_rx,veloy_rx,veloz_rx,bfieldx_rx,bfieldy_rx,bfieldz_rx,
	 dens_ry,velox_ry,veloy_ry,veloz_ry,bfieldx_ry,bfieldy_ry,bfieldz_ry,
	 dens_rz,velox_rz,veloy_rz,veloz_rz,bfieldx_rz,bfieldy_rz,bfieldz_rz,
	 &dt,
         &hx,&hy,&hz,
         &mx,&my,&mz,
         im3,ip3,
	 &NumberOfSubgrids, leftface, rightface,
	 istart, iend, jstart, jend,
	 standard, dnindex,
	 vxindex, vyindex, vzindex,
	 bxindex, byindex, bzindex,
	 f1,f2,f3,f4,f5,f6,f7,
	 g1,g2,g3,g4,g5,g6,g7,
	 h1,h2,h3,h4,h5,h6,h7,
	 ex,ey,ez,
	 qu1,qu2,qu3,qu4,qu5,qu6,qu7);
    /* deallocate temporary space for solver */

    delete [] temp;

    delete [] leftface;

  }  // end: if (field.num_permanent() > 0)

  return ENZO_SUCCESS;
 
}
