// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodDebug.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2020-03-19
/// @brief    Implementation of the debug method

#include "problem.hpp"
#include "charm_simulation.hpp"
#include "test.hpp"

// #define DEBUG_DEBUG
#define CHECK_FOR_NAN

//----------------------------------------------------------------------

MethodDebug::MethodDebug
(int num_fields,
 int num_particles,
 bool l_print,
 bool l_coarse,
 bool l_ghost
 ) throw()
  : Method (),
    num_fields_(num_fields),
    num_particles_(num_particles),
    field_sum_(),
    field_min_(),
    field_max_(),
    field_count_(),
    particle_sum_(),
    particle_min_(),
    particle_max_(),
    particle_count_(),
    l_print_(l_print),
    l_coarse_(l_coarse),
    l_ghost_(l_ghost)
{
  // Set up post-refresh to refresh all fields

  cello::simulation()->refresh_set_name(ir_post_,name());
  cello::refresh(ir_post_)->add_all_fields();

  field_sum_.resize(num_fields*2);
  field_min_.resize(num_fields*2);
  field_max_.resize(num_fields*2);
  field_count_.resize(num_fields*2);

  for (int i=0; i<3; i++) {
    particle_sum_[i].resize(num_particles);
    particle_min_[i].resize(num_particles);
    particle_max_[i].resize(num_particles);
    particle_count_[i].resize(num_particles);
  }
}

//----------------------------------------------------------------------

void MethodDebug::compute ( Block * block) throw()
{
  Field field = block->data()->field();
  int num_history = (field.num_history() + 1);
  const int num_reduce = 4*(num_fields_*num_history+3*num_particles_);
  cello_reduce_type * reduce = new cello_reduce_type [1+num_reduce];
  reduce[0] = num_reduce+1;
  const int kmin=0;
  const int kmax=1;
  const int ksum=2;
  const int knum=3;
  for (int k=1; k<num_reduce; k+=4) {
    reduce[k+kmin] = std::numeric_limits<cello_reduce_type>::max();
    reduce[k+kmax] = -std::numeric_limits<cello_reduce_type>::max();
    reduce[k+ksum] = 0;
    reduce[k+knum] = 0;
  }

  // accumulate local reductions for global

  int mx,my,mz;
  int gx,gy,gz;
  field.dimensions (0,&mx,&my,&mz);
  field.ghost_depth (0,&gx,&gy,&gz);

  const double rel_vol = cello::relative_cell_volume (block->level());
  int k=1;
  for (int ih = 0; ih < num_history; ih++) {
    for (int index_field=0; index_field<num_fields_; index_field++) {

      cello_float * values = (cello_float *) field.values(index_field,ih);

      int err_count = 0;
      for (int iz=gz; iz<mz-gz; iz++) {
        for (int iy=gy; iy<my-gy; iy++) {
          for (int ix=gx; ix<mx-gx; ix++) {
            int i=ix + mx*(iy + my*iz);

            cello_reduce_type value = values[i];
            if (values[i] != values[i] && err_count++ < 10) {
              CkPrintf ("DEBUG_NAN %s %s %d %d %d %p %d/10 is nan!\n",
                        block->name8().c_str(),
                        field.field_name(index_field).c_str(),
                        ix,iy,iz,&values[i],err_count);
            }
            if (block->is_leaf()) {
              reduce[k+kmin] = std::min(reduce[k+kmin],value);
              reduce[k+kmax] = std::max(reduce[k+kmax],value);
              reduce[k+ksum] += values[i];
              reduce[k+knum] += rel_vol;
            }
          }
        }
      }
      if (block->is_leaf()) {
        k += 4;
      }
    }
  }
  if (block->is_leaf()) {
    // particles
    Particle particle = block->data()->particle();
    const int mb = particle.batch_size();
    std::vector<double> position[3];
    position[0].resize(mb); position[0].clear();
    position[1].resize(mb); position[1].clear();
    position[2].resize(mb); position[2].clear();
    for (int it=0; it<num_particles_; it++) {
      const int nb = particle.num_batches(it);
      for (int ib=0; ib<nb; ib++) {
        particle.position(it,ib,
			  position[0].data(),
			  position[1].data(),
			  position[2].data());
        const int np = particle.num_particles(it,ib);
        for (int i=0; i<cello::rank(); i++) {
          for (int ip=0; ip<np; ip++) {
            cello_reduce_type value = position[i][ip];
            reduce[k+4*i+kmin] = std::min(reduce[k+4*i+kmin],value);
            reduce[k+4*i+kmax] = std::max(reduce[k+4*i+kmax],value);
            reduce[k+4*i+ksum] += value;
            reduce[k+4*i+knum] += 1;
          }
        }
      }
      k += 4*3;
    }

    ASSERT2("MethodDebug::compute()",
            "reduce array mismatch %d != %d",
            k,num_reduce+1,(k == num_reduce+1));
  }

#ifdef DEBUG_DEBUG  
  {
    int id = 0;
    Field field = block->data()->field();
    for (int ih = 0; ih < num_history; ih++) {
    for (int i_f=0; i_f<num_fields_; i_f++) {
      std::string name = field.field_name(i_f).c_str();
      cello::monitor()->print
        ("Method", "Field %s age %d %s min %Lg max %Lg sum %Lg cnt %Lg",
         name.c_str(),ih,block->name().c_str(),
         reduce[id],reduce[id+1],reduce[id+2],reduce[id+3]);
      id+=4;
    }
    }
    Particle particle = block->data()->particle();
    for (int it=0; it<num_particles_; it++) {
      const std::string name = particle.type_name(it).c_str();
      cello::monitor()->print
        ("Method", "Particle %s %s num_particles %d",
         name.c_str(),block->name().c_str(),particle.num_particles(it));
      for (int i=0; i<3; i++) {
        const char axis[3] = {'X','Y','Z'};
        cello::monitor()->print
          ("Method", "Particle %s %c %s min %Lg max %Lg sum %Lg cnt %Lg",
           name.c_str(),axis[i],block->name().c_str(),
           reduce[id],reduce[id+1],reduce[id+2],reduce[id+3]);
        id+=4;
      }
    }
  }
#endif
  CkCallback callback (CkIndex_Block::r_method_debug_sum_fields(NULL),
                       block->proxy_array());

  PERF_REDUCE_START(perf_rindex_reduce_method_debug);
  block->contribute
    ((1+num_reduce)*sizeof(cello_reduce_type), reduce,
     r_reduce_method_debug_type, callback);

  delete [] reduce;
}

//----------------------------------------------------------------------

void Block::r_method_debug_sum_fields(CkReductionMsg * msg)
{
  PERF_REDUCE_STOP(perf_rindex_reduce_method_debug);
  static_cast<MethodDebug*>
    (this->method())->compute_continue(this,msg);
}

//----------------------------------------------------------------------

void MethodDebug::compute_continue
( Block * block, CkReductionMsg * msg) throw()
{

  cello_reduce_type * data = (cello_reduce_type *) msg->getData();
  int id = 1;
  Field field = block->data()->field();
  int num_history = (field.num_history() + 1);
  for (int ih = 0; ih < num_history; ih++) {
    for (int index_field=0; index_field<num_fields_; index_field++) {
      field_min_[index_field*2+ih] = data[id];
      field_max_[index_field*2+ih] = data[id+1];
      field_sum_[index_field*2+ih] = data[id+2];
      field_count_[index_field*2+ih] = data[id+3];
      id+=4;
    }
  }
  for (int it=0; it<num_particles_; it++) {
    for (int i=0; i<3; i++) {
      particle_min_[i][it] = data[id];
      particle_max_[i][it] = data[id+1];
      particle_sum_[i][it] = data[id+2];
      particle_count_[i][it] = data[id+3];
      id+=4;
    }
  }
  const int num_reduce = 4*(num_fields_*num_history+3*num_particles_);
  ASSERT2("MethodDebug::compute_continue()",
          "reduce array mismatch %d != %d",
          id,num_reduce+1,(id == num_reduce+1));

  delete msg;

  Particle particle = block->data()->particle();
  if (block->index().is_root()) {
    int nx,ny,nz;
    cello::hierarchy()->root_size(&nx,&ny,&nz);
    //    long int root_cells = nx*ny*nz;
    for (int i_f=0; i_f<num_fields_; i_f++) {
      for (int ih = 0; ih < num_history; ih++) {
        std::string name = field.field_name(i_f).c_str();
        cello::monitor()->print
          ("Method", "Field %s-%d min %30.24Lg avg %30.24Lg max %30.24Lg",
           name.c_str(),ih, 
           field_min_[i_f*2+ih],
           field_sum_[i_f*2+ih]/field_count_[i_f*2+ih],
           field_max_[i_f*2+ih]);
      }
    }
    for (int it=0; it<num_particles_; it++) {
      for (int i=0; i<3; i++) {
        const char axis[3] = {'X','Y','Z'};
        const std::string name = particle.type_name(it).c_str();
        cello::monitor()->print
          ("Method", "Particle %s %c min avg max %30.24Lg %30.24Lg %30.24Lg",
           name.c_str(),axis[i],
           particle_min_[i][it],
           particle_sum_[i][it]/particle_count_[i][it],
           particle_max_[i][it]);
      }
    }
  }

  if (block->is_leaf()) {

    for (int ih = 0; ih < num_history; ih++) {
      for (int i_f=0; i_f<num_fields_; i_f++) {

        int mx,my,mz;
        field.dimensions (i_f,&mx,&my,&mz);
        cello_float * values = (cello_float*)field.values(i_f,ih);

        int gx=0,gy=0,gz=0;
        if (!l_ghost_) field.ghost_depth (i_f,&gx,&gy,&gz);

        if (l_print_) {
          // Write field sums to output
          char buffer[256];
          snprintf(buffer,255,"field-%s-%s-%03d.data",
                   field.field_name(i_f).c_str(),
                   block->name().c_str(),block->state()->cycle());
          FILE * fp = fopen (buffer,"a");
          for (int iz=gz; iz<mz-gz; iz++) {
            for (int iy=gy; iy<my-gy; iy++) {
              for (int ix=gx; ix<mx-gx; ix++) {
                int i=ix + mx*(iy + my*iz);
                fprintf(fp,"%d %d %d %20.18f\n",ix,iy,iz,values[i]);
              }
            }
          }
          fclose(fp);
        }
        // write coarse field if needed
        if (l_coarse_) {

          if (l_print_) {
            // write coarse field
            char buffer[256];
            snprintf(buffer,255,"FIELD-%s-%s-%03d.data",
                     field.field_name(i_f).c_str(),
                     block->name().c_str(),block->state()->cycle());
            FILE * fp = fopen (buffer,"a");

            int mx,my,mz;
            field.coarse_dimensions (i_f,&mx,&my,&mz);
            cello_float * values = (cello_float*)field.coarse_values(i_f);
            for (int iz=gz; iz<mz-gz; iz++) {
              for (int iy=gy; iy<my-gy; iy++) {
                for (int ix=gx; ix<mx-gx; ix++) {
                  int i=ix + mx*(iy + my*iz);
                  fprintf(fp,"%d %d %d %20.18f\n",ix,iy,iz,values[i]);
                }
              }
            }
            fclose(fp);
          }
        }
      }
    }
  }

  block->compute_done();
}

