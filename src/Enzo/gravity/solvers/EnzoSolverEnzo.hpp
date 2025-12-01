// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSolverEnzo.hpp
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     2018-10-01
/// @brief    [\ref Enzo] Declaration of EnzoSolverEnzo
///
/// Domain decomposition solver

#ifndef ENZO_ENZO_SOLVER_ENZO_HPP
#define ENZO_ENZO_SOLVER_ENZO_HPP

class EnzoSolverEnzo : public Solver {

  /// @class    EnzoSolverEnzo
  /// @ingroup  Enzo
  ///
  /// @brief [\ref Enzo] Multigrid on the root-level grid using Mg0, then
  /// BiCgStab in overlapping subdomains defined by root-level Blocks.
  /// An optional final Jacobi step can be applied to smooth the solution
  /// along subdomain boundaries.

public: // interface

  /// Create a new EnzoSolverEnzo object
  EnzoSolverEnzo
  (std::string name,
   std::string field_x,
   std::string field_b,
   int monitor_iter,
   int restart_cycle,
   int solve_type,
   int index_prolong,
   int index_restrict,
   int min_level,
   int max_level,
   int index_solve_root,
   int index_solve_block) ;

  EnzoSolverEnzo() {};

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoSolverEnzo);
  
  /// Charm++ PUP::able migration constructor
  EnzoSolverEnzo (CkMigrateMessage *m)
    :  Solver(m),
       A_(),
       index_solve_root_(-1),
       index_solve_block_(-1)
  {  }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {

    // NOTE: change this function whenever attributes change

    TRACEPUP;

    Solver::pup(p);
    p | index_solve_root_;
    p | index_solve_block_;
  }

public:  // virtual methods

  /// Solve the linear system
  virtual void apply ( std::shared_ptr<Matrix> A, Block * block) throw();

  /// Type of this solver
  virtual std::string type() const { return "enzo"; }

public: // methods

  /// synchronize entering solver for each level
  void begin_solve(EnzoBlock * enzo_block) throw();

  /// Restrict b to coarser Block
  void do_restrict(EnzoBlock * enzo_block) throw();
  void restrict_send(EnzoBlock * enzo_block) throw();
  void restrict_recv(EnzoBlock * enzo_block,
		     FieldMsg * field_message) throw();

  /// Call coarse solver--must be called by all blocks
  void call_coarse_solver(EnzoBlock * enzo_block) throw();

  //----------------------------------------------------------------------
  
  /// Root-level solver
  void call_root_solver(EnzoBlock * enzo_block) throw();

  /// End of solver
  void end(Block* block) throw();

protected: // methods

  /// Access the Field message for buffering prolongation data
  FieldMsg ** pmsg_prolong_(Block * block)
  {
    return pmsg_(block,i_msg_prolong_);
  }

  /// Access the Field message for buffering restriction data
  FieldMsg ** pmsg_restrict_(Block * block, int ic)
  {
    return pmsg_(block,i_msg_restrict_[ic]);
  }

  /// Access the prolong Sync Scalar value for the Block
  Sync * psync_prolong_(Block * block)
  {
    return psync_(block,i_sync_prolong_);
  }

  /// Access the restrict Sync Scalar value for the Block
  Sync * psync_restrict_(Block * block)
  {
    return psync_(block,i_sync_restrict_);
  }

  Sync * psync_(Block * block, int i_sync)
  {
    ScalarData<Sync> * scalar_data = block->data()->scalar_data_sync();
    ScalarDescr *      scalar_descr = cello::scalar_descr_sync();
    return scalar_data->value(scalar_descr,i_sync);
  }

  FieldMsg ** pmsg_(Block * block, int i_msg)
  {
    ScalarData<void *> * scalar_data = block->data()->scalar_data_void();
    ScalarDescr *        scalar_descr = cello::scalar_descr_void();
    return (FieldMsg **)scalar_data->value(scalar_descr,i_msg);
  }

  FieldMsg * pack_field_
  (EnzoBlock *, int index_field, int refresh_type, int ic3[3]);

  void unpack_field_
  (EnzoBlock *, FieldMsg *, int index_field, int refresh_type);

protected: // attributes

  /// Matrix
  std::shared_ptr<Matrix> A_;

  /// Indices for root solver and (extended) block solver
  int index_solve_root_;
  int index_solve_block_;

  /// Sync ids
  int i_sync_restrict_;
  int i_sync_prolong_;
  int i_msg_restrict_[8];
  int i_msg_prolong_;

};

#endif /* ENZO_ENZO_SOLVER_GRAVITY_ENZO_HPP */
