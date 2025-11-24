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
   int max_level) ;

  EnzoSolverEnzo() {};

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoSolverEnzo);
  
  /// Charm++ PUP::able migration constructor
  EnzoSolverEnzo (CkMigrateMessage *m)
    :  Solver(m),
       A_()
  {  }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {

    // NOTE: change this function whenever attributes change

    TRACEPUP;

    Solver::pup(p);
  }

public:  // virtual methods

  /// Solve the linear system
  virtual void apply ( std::shared_ptr<Matrix> A, Block * block) throw();

  /// Type of this solver
  virtual std::string type() const { return "enzo"; }

public: // methods

  /// End of solver
  void end(Block* block) throw();

protected: // methods

protected: // attributes

  /// Matrix
  std::shared_ptr<Matrix> A_;

};

#endif /* ENZO_ENZO_SOLVER_GRAVITY_ENZO_HPP */
