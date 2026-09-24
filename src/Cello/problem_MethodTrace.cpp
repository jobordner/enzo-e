// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodTrace.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-11-06
/// @brief    Implementation of the Tracer Particle method

#include "problem.hpp"
#include "charm_simulation.hpp"
  
//----------------------------------------------------------------------

MethodTrace::MethodTrace(ParameterGroup p) noexcept
  : Method ( "trace", p.value<double>("courant",1.0) ),
    timestep_( p.value<double>("timestep", std::numeric_limits<double>::max()) ),
    type_( p.value<std::string>("name", "trace") )
{
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_all_particles();
}

//----------------------------------------------------------------------

void MethodTrace::compute ( Block * block) throw()
{
  if (block->is_leaf()) {

    Particle particle (block->data()->particle());
    Field    field    (block->data()->field());

    // initialize trace particle type and position attributes

    const int it = particle.type_index(type_);

    const int ia_x = particle.attribute_index(it,"x");
    const int ia_y = particle.attribute_index(it,"y");
    const int ia_z = particle.attribute_index(it,"z");

    const int dp =  particle.stride(it,ia_x);

    const int rank = cello::rank();

    // NOTE: union so v?a4 also initialized

    cello_float * vxa_array = field.values("velocity_x");
    cello_float * vya_array = field.values("velocity_y");
    cello_float * vza_array = field.values("velocity_z");

    int mx,my,mz;
    int nx,ny,nz;
    int gx,gy,gz;
    field.dimensions(0,&mx,&my,&mz);
    field.size(&nx,&ny,&nz);
    field.ghost_depth(0,&gx,&gy,&gz);

    double xm,ym,zm;
    double xp,yp,zp;
    block->lower(&xm,&ym,&zm);
    block->upper(&xp,&yp,&zp);

    const double hx = (xp-xm)/nx;
    const double hy = (yp-ym)/ny;
    const double hz = (zp-zm)/nz;

    double dt = block->dt();

    // declare particle position arrays
    float * xa = 0;
    float * ya = 0;
    float * za = 0;

    for (int ib=0; ib<particle.num_batches(it); ib++) {

      xa = (float *) particle.attribute_array (it,ia_x,ib);
      ya = (float *) particle.attribute_array (it,ia_y,ib);
      za = (float *) particle.attribute_array (it,ia_z,ib);

      const int np = particle.num_particles(it,ib);

      if (rank == 1) {

        for (int ip=0; ip<np; ip++) {

          double x = xa[ip*dp];

          int ix0 = gx + floor((nx-1)*(x - xm) / (xp - xm));
          int ix1 = ix0 + 1;
          double x0 = xm + (ix0-gx+0.5)*hx;
          double x1 = 1 - x0;
          double v0 = vxa_array[ix0];
          double v1 = vxa_array[ix1];
          double vx = v0*x1 + v1*x0;

          xa[ip*dp] += vx*dt;

        }

      } else if (rank == 2) {

        for (int ip=0; ip<np; ip++) {

          double x = xa[ip*dp];
          double y = ya[ip*dp];

          int ix0 = gx + floor((nx-1)*(x - xm) / (xp - xm));
          int iy0 = gy + floor((ny-1)*(y - ym) / (yp - ym));

          int ix1 = ix0 + 1;
          int iy1 = iy0 + 1;

          double x0 = xm + (ix0-gx+0.5)*hx;
          double y0 = ym + (iy0-gy+0.5)*hy;

          double x1 = 1.0 - x0;
          double y1 = 1.0 - y0;

          const int i00 = ix0+mx*iy0;
          const int i10 = ix1+mx*iy0;
          const int i01 = ix0+mx*iy1;
          const int i11 = ix1+mx*iy1;

          double vx00 = vxa_array[i00];
          double vx10 = vxa_array[i10];
          double vx01 = vxa_array[i01];
          double vx11 = vxa_array[i11];

          double vx = vx00*x1*y1 
            +         vx10*x0*y1
            +         vx01*x1*y0
            +         vx11*x0*y0;

          double vy00 = vya_array[i00];
          double vy10 = vya_array[i10];
          double vy01 = vya_array[i01];
          double vy11 = vya_array[i11];

          double vy = vy00*x1*y1 
            +         vy10*x0*y1
            +         vy01*x1*y0
            +         vy11*x0*y0;

          xa[ip*dp] += vx*dt;
          ya[ip*dp] += vy*dt;

        }
      } else if (rank == 3) {
        for (int ip=0; ip<np; ip++) {

          double x = xa[ip*dp];
          double y = ya[ip*dp];
          double z = za[ip*dp];

          int ix0 = gx + floor((nx-1)*(x - xm) / (xp - xm));
          int iy0 = gy + floor((ny-1)*(y - ym) / (yp - ym));
          int iz0 = gz + floor((nz-1)*(z - zm) / (zp - zm));

          int ix1 = ix0 + 1;
          int iy1 = iy0 + 1;
          int iz1 = iz0 + 1;

          double x0 = xm + (ix0-gx+0.5)*hx;
          double y0 = ym + (iy0-gy+0.5)*hy;
          double z0 = zm + (iz0-gz+0.5)*hz;

          double x1 = 1.0 - x0;
          double y1 = 1.0 - y0;
          double z1 = 1.0 - z0;

          const int i000 = ix0+mx*(iy0 + my*iz0);
          const int i001 = ix0+mx*(iy0 + my*iz1);
          const int i010 = ix0+mx*(iy1 + my*iz0);
          const int i011 = ix0+mx*(iy1 + my*iz1);
          const int i100 = ix1+mx*(iy0 + my*iz0);
          const int i101 = ix1+mx*(iy0 + my*iz1);
          const int i110 = ix1+mx*(iy1 + my*iz0);
          const int i111 = ix1+mx*(iy1 + my*iz1);

          double vx000 = vxa_array[i000];
          double vx010 = vxa_array[i010];
          double vx001 = vxa_array[i001];
          double vx011 = vxa_array[i011];
          double vx100 = vxa_array[i100];
          double vx110 = vxa_array[i110];
          double vx101 = vxa_array[i101];
          double vx111 = vxa_array[i111];

          double vx = vx000*x1*y1*z1
            +         vx100*x0*y1*z1
            +         vx010*x1*y0*z1
            +         vx110*x0*y0*z1
            +         vx001*x1*y1*z0
            +         vx101*x0*y1*z0
            +         vx011*x1*y0*z0
            +         vx111*x0*y0*z0;

          double vy000 = vya_array[i000];
          double vy010 = vya_array[i010];
          double vy001 = vya_array[i001];
          double vy011 = vya_array[i011];
          double vy100 = vya_array[i100];
          double vy110 = vya_array[i110];
          double vy101 = vya_array[i101];
          double vy111 = vya_array[i111];

          double vy = vy000*x1*y1*z1
            +         vy100*x0*y1*z1
            +         vy010*x1*y0*z1
            +         vy110*x0*y0*z1
            +         vy001*x1*y1*z0
            +         vy101*x0*y1*z0
            +         vy011*x1*y0*z0
            +         vy111*x0*y0*z0;

          double vz000 = vza_array[i000];
          double vz010 = vza_array[i010];
          double vz001 = vza_array[i001];
          double vz011 = vza_array[i011];
          double vz100 = vza_array[i100];
          double vz110 = vza_array[i110];
          double vz101 = vza_array[i101];
          double vz111 = vza_array[i111];

          double vz = vz000*x1*y1*z1
            +         vz100*x0*y1*z1
            +         vz010*x1*y0*z1
            +         vz110*x0*y0*z1
            +         vz001*x1*y1*z0
            +         vz101*x0*y1*z0
            +         vz011*x1*y0*z0
            +         vz111*x0*y0*z0;


          xa[ip*dp] += vx*dt;
          ya[ip*dp] += vy*dt;
          za[ip*dp] += vz*dt;
        }
      }
    }
  }
  
  block->compute_done();
  
}

//----------------------------------------------------------------------

double MethodTrace::timestep (Block * block) throw()
{
  const int rank = cello::rank();

  double dt = std::numeric_limits<double>::max();

  if (block->is_leaf()) {

    Field    field    = block->data()->field();

    // NOTE: union so v?a4 also initialized

    cello_float * vxa_array = field.values("velocity_x");
    cello_float * vya_array = (rank >= 2) ? field.values("velocity_y") : nullptr;
    cello_float * vza_array = (rank >= 3) ? field.values("velocity_z") : nullptr;

    double xm,ym,zm;
    double xp,yp,zp;
    block->lower(&xm,&ym,&zm);
    block->upper(&xp,&yp,&zp);

    int nx,ny,nz;
    field.size(&nx,&ny,&nz);
    int gx,gy,gz;
    field.ghost_depth(0,&gx,&gy,&gz);
    int mx,my,mz;
    mx = nx + 2*gx;
    my = (rank >= 2) ? ny + 2*gy : 1;
    mz = (rank >= 3) ? nz + 2*gz : 1;

    const double hx = (xp-xm)/nx;
    const double hy = (yp-ym)/ny;
    const double hz = (zp-zm)/nz;

    if (rank == 1) {
      for (int ix=0; ix<mx; ix++) {
        int i = ix;
        double vx = vxa_array[i];
        double dt_vx = hx / MAX(fabs(vx),1e-6);
        dt = MIN(dt,dt_vx);
      }
    } else if (rank == 2) {
      for (int iy=0; iy<my; iy++) {
        for (int ix=0; ix<mx; ix++) {
          int i = ix + mx*iy;
          double vx = vxa_array[i];
          double vy = vya_array[i];
          double dt_vx = hx / MAX(fabs(vx),1e-6);
          double dt_vy = hy / MAX(fabs(vy),1e-6);
          dt = MIN(dt,dt_vx);
          dt = MIN(dt,dt_vy);
        }
      }
    } else if (rank == 3) {
      for (int iz=0; iz<mz; iz++) {
        for (int iy=0; iy<my; iy++) {
          for (int ix=0; ix<mx; ix++) {
            int i = ix + mx*(iy + my*iz);
            double vx = fabs(vxa_array[i]);
            double vy = fabs(vya_array[i]);
            double vz = fabs(vza_array[i]);
            double dt_vx = hx / MAX(fabs(vx),1e-6);
            double dt_vy = hy / MAX(fabs(vy),1e-6);
            double dt_vz = hz / MAX(fabs(vz),1e-6);
            dt = MIN(dt,dt_vx);
            dt = MIN(dt,dt_vy);
            dt = MIN(dt,dt_vz);
          }
        }
      }
    }
  }
  return MIN(timestep_,dt);
}
