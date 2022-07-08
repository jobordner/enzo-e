// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoSolverFmm.hpp
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     2022-06-27
/// @brief    [\ref Enzo] Implementation of Enzo FMM hydro solver

#ifndef ENZO_ENZO_SOLVER_FMM_HPP
#define ENZO_ENZO_SOLVER_FMM_HPP

class EnzoSolverFmm : public Solver {

  /// @class    EnzoSolverFmm
  /// @ingroup  Enzo
  /// @brief    [\ref Enzo] Encapsulate Enzo's FMM hydro solver

public: // interface

  /// Create a new EnzoSolverFmm object
  EnzoSolverFmm(double theta);

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoSolverFmm);
  
  /// Charm++ PUP::able migration constructor
  EnzoSolverFmm (CkMigrateMessage *m)
    : Solver (m),
      A_(nullptr),
      theta_(0),
      min_level_(0),
      max_level_(0),
      is_volume_(-1),
      block_volume_(),
      max_volume_(0)
  {}

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  void traverse (EnzoBlock * enzo_block, Index index_b, int type_b);
  void traverse_done (EnzoBlock * enzo_block);
  /// Identified pair of blocks considered to be "distant"
  void traverse_approx_pair (EnzoBlock * enzo_block,
                             Index index_a, int volume_a,
                             Index index_b, int volume_b);
  /// Identified pair of blocks considered to be "close". (Block
  /// A may be the same as block B but won't be called twice.)
  void traverse_direct_pair (EnzoBlock * enzo_block,
                             Index index_a, int volume_a,
                             Index index_b, int volume_b);

  void update_volume (EnzoBlock * enzo_block, Index index, int volume);

public: // virtual solvers

  /// Apply the solver to advance a block one timestep 
  virtual void apply( std::shared_ptr<Matrix> A, Block * block) throw();

  /// Type of this solver
  virtual std::string type() const { return "fmm"; }

  
protected: // interface

  bool is_far_ (EnzoBlock * enzo_block, Index index_b,
                double * ra, double * rb) const;

  long long * volume_(Block * block) {
    return block->data()->scalar_long_long().value(is_volume_);
  }

protected: // attributes

  /// Matrix
  std::shared_ptr<Matrix> A_;

  /// Parameter controlling the multipole acceptance criterion, defined
  /// as distance(A,B) > theta * (radius(A) + radius(B))
  double theta_;

  /// Minimum/maximum mesh refinement level (saved for efficiency)
  int min_level_;
  int max_level_;

  /// ScalarData<long long> index for summing block volumes to
  /// test for termination
  int is_volume_;
  /// volume of block in each level, with finest level normalized to 1. Used
  /// to test for termination
  std::vector<int> block_volume_;
  /// Volume of domain in terms of finest blocks
  long long max_volume_;
};

#endif /* ENZO_ENZO_SOLVER_FMM_HPP */
