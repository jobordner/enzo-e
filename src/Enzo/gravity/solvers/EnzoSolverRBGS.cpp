// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSolverRBGS.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2025-12-12
/// @brief    Implements the EnzoSolverRBGS class

#include "Cello/cello.hpp"
#include "Enzo/enzo.hpp"
#include "Enzo/gravity/gravity.hpp"

//----------------------------------------------------------------------

EnzoSolverRBGS::EnzoSolverRBGS
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
    w_(weight),
    n_(iter_max),
    ir_smooth_(-1),
    local_(solve_type==solve_block)
{
  // Reserve temporary fields

  if (! local_) {

    Refresh * refresh = cello::refresh(ir_post_);

    refresh->add_field (ix_);
    refresh->set_min_face_rank(cello::rank() - 1);

    ScalarDescr * scalar_descr_int = cello::scalar_descr_int();
    i_iter_ = scalar_descr_int->new_value(name_ + ":iter");

    ir_smooth_ = add_refresh_(":smooth");

    Refresh * refresh_smooth = cello::refresh(ir_smooth_);

    refresh_smooth->add_field (ix_);
    refresh_smooth->set_min_face_rank(cello::rank() - 1);
    refresh_smooth->set_callback(CkIndex_EnzoBlock::p_solver_rbgs_continue());
    refresh_smooth->set_final_sync(true);
  }

}

//----------------------------------------------------------------------

void EnzoSolverRBGS::apply
( std::shared_ptr<Matrix> A, Block * block) throw()
{
  begin_(block);

  if (solve_type_ == solve_level && ! is_finest_(block))
    end_(block);

  A_ = A;

  Field field = block->data()->field();

  if (local_) {

    local_solve_(block, n_);
    end_ (block);

  } else {

    (*piter_(block)) = 0.0;

    // Refresh X

    do_refresh_(block);
  }
}

//----------------------------------------------------------------------

void EnzoBlock::p_solver_rbgs_continue()
{
  EnzoSolverRBGS * solver = nullptr;  

  solver = static_cast<EnzoSolverRBGS *> (this->solver());

  solver->compute(this);
}

//----------------------------------------------------------------------

void EnzoSolverRBGS::compute(Block * block)
{
  if (*piter_(block) < n_) {

    apply_(block);

  } else {

    Field field = block->data()->field();

    end_(block);

  }
}

//----------------------------------------------------------------------

void EnzoSolverRBGS::apply_(Block * block)
{
  if (is_finest_(block)) {

    local_solve_(block, 1);

  }
  // Next iteration

  (*piter_(block))++;

  // Refresh X

  do_refresh_(block);

}

//----------------------------------------------------------------------

void EnzoSolverRBGS::do_refresh_(Block * block)
{
  Refresh * refresh = cello::refresh(ir_smooth_);

  refresh->set_active(is_finest_(block));
  refresh->add_field (ix_);
  refresh->set_min_face_rank(cello::rank() - 1);

  block->refresh_start
    (ir_smooth_, CkIndex_EnzoBlock::p_solver_rbgs_continue());
}

//----------------------------------------------------------------------

void EnzoSolverRBGS::local_solve_(Block * block, int n)
{
  Field field = block->data()->field();

  enzo_float * X = (enzo_float*) field.values(ix_);
  enzo_float * B = (enzo_float*) field.values(ib_);

  int gx,gy,gz;
  field.ghost_depth(ix_,&gx,&gy,&gz);

  gx = include_ghosts_ ? std::min(1,gx) : gx;
  gy = include_ghosts_ ? std::min(1,gy) : gy;
  gz = include_ghosts_ ? std::min(1,gz) : gz;

  const int nd = A_->stencil_width();
  std::vector<double> ax(nd+1);
  std::vector<double> ay(nd+1);
  std::vector<double> az(nd+1);

  double hx,hy,hz;
  block->cell_width(&hx,&hy,&hz);

  const double ad = A_->stencil_value(0,0,0,hx,hy,hz);

  const int rank = cello::rank();
  for (int d=1; d<=nd; d++) {
    ax[d] = (rank >= 1) ? A_->stencil_value(d,0,0,hx,hy,hz) : 0.0;
    ay[d] = (rank >= 2) ? A_->stencil_value(0,d,0,hx,hy,hz) : 0.0;
    az[d] = (rank >= 3) ? A_->stencil_value(0,0,d,hx,hy,hz) : 0.0;
  }

  int mx,my,mz;
  field.dimensions(ix_,&mx,&my,&mz);
  const int ixp = 1;
  const int iyp = mx;
  const int izp = mx*my;
  if (w_ == 1.0) {
    // red (even)
    for (int k=0; k<n; k++) {
      for (int iz=gz; iz<mz-gz; iz++) {
        const int kz = iz-gz;
        for (int iy=gy; iy<my-gy; iy++) {
          const int ky = iy-gy;
          const int e = (ky + kz)%2;
          for (int ix=gx+e; ix<mx-gx; ix+=2) {
            int i = ix + mx*(iy + my*iz);
            X[i] = B[i];
            for (int d=1; d<=nd; d++) {
              X[i] -= ax[d]*(X[i+d*ixp] + X[i-d*ixp]);
              X[i] -= ay[d]*(X[i+d*iyp] + X[i-d*iyp]);
              X[i] -= az[d]*(X[i+d*izp] + X[i-d*izp]);
            }
            X[i] /= ad;
          }
        }
      }
      // black (odd)
      for (int iz=gz; iz<mz-gz; iz++) {
        const int kz = iz-gz;
        for (int iy=gy; iy<my-gy; iy++) {
          const int ky = iy-gy;
          const int o = 1 - (ky + kz)%2;
          for (int ix=gx+o; ix<mx-gx; ix+=2) {
            int i = ix + mx*(iy + my*iz);
            X[i] = B[i];
            for (int d=1; d<=nd; d++) {
              X[i] -= ax[d]*(X[i+d*ixp] + X[i-d*ixp]);
              X[i] -= ay[d]*(X[i+d*iyp] + X[i-d*iyp]);
              X[i] -= az[d]*(X[i+d*izp] + X[i-d*izp]);
            }
            X[i] /= ad;
          }
        }
      }
    }
  } else {
    // red (even)
    for (int k=0; k<n; k++) {
      for (int iz=gz; iz<mz-gz; iz++) {
        const int kz = iz-gz;
        for (int iy=gy; iy<my-gy; iy++) {
          const int ky = iy-gy;
          const int e = (ky + kz)%2;
          for (int ix=gx+e; ix<mx-gx; ix+=2) {
            int i = ix + mx*(iy + my*iz);
            double update = B[i];
            for (int d=1; d<=nd; d++) {
              update -= ax[d]*(X[i+d*ixp] + X[i-d*ixp]);
              update -= ay[d]*(X[i+d*iyp] + X[i-d*iyp]);
              update -= az[d]*(X[i+d*izp] + X[i-d*izp]);
            }
            update /= ad;
            X[i] = (1.0-w_)*X[i] + w_*update;
          }
        }
      }
      // black (odd)
      for (int iz=gz; iz<mz-gz; iz++) {
        const int kz = iz-gz;
        for (int iy=gy; iy<my-gy; iy++) {
          const int ky = iy-gy;
          const int o = 1 - (ky + kz)%2;
          for (int ix=gx+o; ix<mx-gx; ix+=2) {
            int i = ix + mx*(iy + my*iz);
            double update = B[i];
            for (int d=1; d<=nd; d++) {
              update -= ax[d]*(X[i+d*ixp] + X[i-d*ixp]);
              update -= ay[d]*(X[i+d*iyp] + X[i-d*iyp]);
              update -= az[d]*(X[i+d*izp] + X[i-d*izp]);
            }
            update /= ad;
            X[i] = (1.0-w_)*X[i] + w_*update;
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------

void EnzoSolverRBGS::end_(Block * block)
{
 Field field = block->data()->field();

  Solver::end_(block);
}
