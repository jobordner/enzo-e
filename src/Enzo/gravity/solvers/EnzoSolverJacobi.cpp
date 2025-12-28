// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSolverJacobi.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2014-10-27 22:37:41
/// @brief    Implements the EnzoSolverJacobi class

#include "Cello/cello.hpp"
#include "Enzo/enzo.hpp"
#include "Enzo/gravity/gravity.hpp"

//----------------------------------------------------------------------

EnzoSolverJacobi::EnzoSolverJacobi
( std::string name,
  std::string field_x,
  std::string field_b,
  int monitor_iter,
  int restart_cycle,
  int solve_type,
  int index_prolong,
  int index_restrict,
  double weight, int iter_max) throw()
  : Solver(name,
           field_x,
           field_b,
           monitor_iter,
           restart_cycle,
           solve_type,
           index_prolong,
           index_restrict),
    A_ (nullptr),
    ir_ (-1),
    id_ (-1),
    w_(weight),
    n_(iter_max),
    ir_smooth_(-1),
    local_(solve_type==solve_block)
{
  // Reserve temporary fields

  id_ = cello::field_descr()->insert_temporary();
  ir_ = cello::field_descr()->insert_temporary();

  if (! local_) {

    Refresh * refresh = cello::refresh(ir_post_);
    cello::simulation()->refresh_set_name(ir_post_,name);

    refresh->add_field (ix_);
    refresh->set_min_face_rank(cello::rank() - 1);

    ScalarDescr * scalar_descr_int = cello::scalar_descr_int();
    i_iter_ = scalar_descr_int->new_value(name_ + ":iter");

    ir_smooth_ = add_refresh_();

    Refresh * refresh_smooth = cello::refresh(ir_smooth_);
    cello::simulation()->refresh_set_name(ir_smooth_,name+":smooth");

    refresh_smooth->add_field (ix_);
    refresh_smooth->set_min_face_rank(cello::rank() - 1);
    refresh_smooth->set_callback(CkIndex_EnzoBlock::p_solver_jacobi_continue());
    refresh_smooth->set_final_sync(true);
  }

}

//----------------------------------------------------------------------

void EnzoSolverJacobi::apply
( std::shared_ptr<Matrix> A, Block * block) throw()
{
  begin_(block);

  if (solve_type_ == solve_level && ! is_finest_(block))
    end_(block);

  A_ = A;

  Field field = block->data()->field();

  allocate_temporary_(field,block);

  if (local_) {

    local_solve_(block);

  } else {

    (*piter_(block)) = 0.0;

    // Refresh X

    do_refresh_(block);
  }
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_jacobi_continue()
{
 
  performance_start_(perf_compute,__FILE__,__LINE__);

  EnzoSolverJacobi * solver = nullptr;  

  solver = static_cast<EnzoSolverJacobi *> (this->solver());

  solver->compute(this);

  performance_stop_(perf_compute,__FILE__,__LINE__);
}

//----------------------------------------------------------------------

void EnzoSolverJacobi::compute(Block * block)
{
  if (*piter_(block) < n_) {

    apply_(block);

  } else {

    Field field = block->data()->field();

    end_(block);

  }
}

//----------------------------------------------------------------------

void EnzoSolverJacobi::apply_(Block * block)
{
  Field field = block->data()->field();

  int mx,my,mz;
  field.dimensions(ix_,&mx,&my,&mz);

  int gx,gy,gz;
  field.ghost_depth(ix_,&gx,&gy,&gz);

  const int ng = A_->stencil_width();
  gx = (mx > 1) ? ng : 0;
  gy = (my > 1) ? ng : 0;
  gz = (mz > 1) ? ng : 0;

  if (is_finest_(block)) {

     double hx,hy,hz;
     block->cell_width(&hx,&hy,&hz);

     A_->diagonal (id_, field,hx,hy,hz,ng);
     A_->residual (ir_, ib_, ix_, field,hx,hy,hz,ng);

    enzo_float * X = (enzo_float*) field.values(ix_);
    enzo_float * R = (enzo_float*) field.values(ir_);
    enzo_float * D = (enzo_float*) field.values(id_);

    if (w_ == 1.0) {
      for (int iz=gz; iz<mz-gz; iz++) {
        for (int iy=gy; iy<my-gy; iy++) {
          for (int ix=gx; ix<mx-gx; ix++) {
            int i = ix + mx*(iy + my*iz);
            X[i] += R[i] / D[i];
          }
        }
      }
    } else {
      for (int iz=gz; iz<mz-gz; iz++) {
        for (int iy=gy; iy<my-gy; iy++) {
          for (int ix=gx; ix<mx-gx; ix++) {
            int i = ix + mx*(iy + my*iz);
            X[i] = w_*(R[i] / D[i]) + (1.0-w_)*X[i];
          }
        }
      }
    }
  }
  // Next iteration

  (*piter_(block))++;
  
  // Refresh X

  do_refresh_(block);

}

//----------------------------------------------------------------------

void EnzoSolverJacobi::do_refresh_(Block * block)
{
  Refresh * refresh = cello::refresh(ir_smooth_);

  refresh->set_active(is_finest_(block));
  refresh->add_field (ix_);
  refresh->set_min_face_rank(cello::rank() - 1);

  block->refresh_start
    (ir_smooth_, CkIndex_EnzoBlock::p_solver_jacobi_continue());
}

//----------------------------------------------------------------------

void EnzoSolverJacobi::local_solve_(Block * block)
{
  Field field = block->data()->field();

  enzo_float * D = (enzo_float*) field.values(id_);
  enzo_float * R = (enzo_float*) field.values(ir_);
  enzo_float * X = (enzo_float*) field.values(ix_);

  int gx,gy,gz;
  field.ghost_depth(ix_,&gx,&gy,&gz);

  gx = include_ghosts_ ? std::min(1,gx) : gx;
  gy = include_ghosts_ ? std::min(1,gy) : gy;
  gz = include_ghosts_ ? std::min(1,gz) : gz;

  double hx,hy,hz;
  block->cell_width(&hx,&hy,&hz);
  A_->diagonal (id_, field,hx,hy,hz,gx);
  A_->residual (ir_, ib_, ix_, field,hx,hy,hz,gx);

  int mx,my,mz;
  field.dimensions(ix_,&mx,&my,&mz);
  if (w_ == 1.0) {
    for (int k=0; k<n_; k++) {
      for (int iz=gz; iz<mz-gz; iz++) {
        for (int iy=gy; iy<my-gy; iy++) {
          for (int ix=gx; ix<mx-gx; ix++) {
            int i = ix + mx*(iy + my*iz);
            X[i] += R[i] / D[i];
          }
        }
      }
    }
  } else {
    for (int k=0; k<n_; k++) {
      for (int iz=gz; iz<mz-gz; iz++) {
        for (int iy=gy; iy<my-gy; iy++) {
          for (int ix=gx; ix<mx-gx; ix++) {
            int i = ix + mx*(iy + my*iz);
            X[i] = w_*(R[i] / D[i]) + (1.0-w_)*X[i];
          }
        }
      }
    }
  }
  end_ (block);
}

//----------------------------------------------------------------------

void EnzoSolverJacobi::end_(Block * block)
{
  Field field = block->data()->field();

  deallocate_temporary_ (field,block);
  Solver::end_(block);
}
