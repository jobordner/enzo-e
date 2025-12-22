// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSolverRBGS.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-04-30 18:45:58
/// @brief    [\ref Enzo] Declaration of the EnzoSolverRBGS class

#ifndef ENZO_ENZO_SOLVER_RBGS_HPP
#define ENZO_ENZO_SOLVER_RBGS_HPP

class EnzoSolverRBGS : public Solver {

  /// @class    EnzoSolverRBGS
  /// @ingroup  Enzo
  /// @brief    [\ref Enzo]

public: // interface

  /// Constructor
  EnzoSolverRBGS(std::string name,
                   std::string field_x,
                   std::string field_b,
                   int monitor_iter,
                   int restart_cycle,
                   int solve_type,
                   int index_prolong,
                   int index_restrict,
                   double weight=1.0,
                   int iter_max = 1) throw();

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoSolverRBGS);

  /// Charm++ PUP::able migration constructor
  EnzoSolverRBGS (CkMigrateMessage *m)
    : Solver(m),
      A_(NULL),
      w_(0),
      i_iter_(-1),
      n_(0),
      ir_smooth_(-1),
      local_(false)
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {
    TRACEPUP;
    Solver::pup(p);

    //    p | A_;
    p | w_;
    p | i_iter_;
    p | n_;
    p | ir_smooth_;
    p | local_;
  }

public: // virtual methods

  /// Solve the linear system Ax = b
  virtual void apply ( std::shared_ptr<Matrix> A, Block * block) throw();

  /// Type of this solver
  virtual std::string type() const { return "RBGS"; }

  bool is_finest(Block * block) {return is_finest_(block); }

protected: // virtual methods

  /// Whether Block is active
  virtual bool is_active_(Block * block) const
  {
    if (solve_type_ == solve_level) {
      return true;
    } else {
      return Solver::is_active_(block);
    }
  }

  /// Whether solution is defined on this Block
  virtual bool is_finest_(Block * block) const
  {
    if (solve_type_ == solve_level) {
      return true;
    } else {
      return Solver::is_finest_(block);
    }
  }

public: // methods

  /// Continue after refresh to perform RBGS update
  void compute (Block * block);

protected: // methods

  /// Implementation of solver() for given precision
  void apply_(Block * block);

  /// Refresh after computing
  void do_refresh_(Block * block);

  /// Return a pointer to the iteration counter on the block
  int * piter_(Block * block) {
    ScalarData<int> * scalar_data  = block->data()->scalar_data_int();
    ScalarDescr *     scalar_descr = cello::scalar_descr_int();
    return scalar_data->value(scalar_descr,i_iter_);
  }

  /// Serial RBGS solver if local_ == true
  void local_solve_ (Block * block, int num_iter);

  /// Clean up after solve and call Solver::end_()
  void end_ (Block * block );

protected: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Matrix A for smoothing A*X = B
  std::shared_ptr<Matrix> A_;

  /// Weighting
  double w_;

  /// Scalar index for current iteration on a Block
  int i_iter_;

  /// Number of iterations
  int n_;

  // Refresh after each smoothing
  int ir_smooth_;

  /// Whether to solve on a standalone Block
  bool local_;
};

#endif /* ENZO_ENZO_SOLVER_RBGS_HPP */

