// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMethodFmm.hpp
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     2022-06-27
/// @brief    [\ref Enzo] Implementation of Enzo FMM hydro method

#ifndef ENZO_ENZO_METHOD_FMM_HPP
#define ENZO_ENZO_METHOD_FMM_HPP

class EnzoMethodFmm : public Method {

  /// @class    EnzoMethodFmm
  /// @ingroup  Enzo
  /// @brief    [\ref Enzo] Encapsulate Enzo's FMM hydro method

public: // interface

  /// Create a new EnzoMethodFmm object
  EnzoMethodFmm(double theta);

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoMethodFmm);
  
  /// Charm++ PUP::able migration constructor
  EnzoMethodFmm (CkMigrateMessage *m)
    : Method (m),
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

public: // virtual methods

  /// Apply the method to advance a block one timestep 
  virtual void compute( Block * block) throw();

  virtual std::string name () throw () 
  { return "fmm"; }

  
protected: // interface

  bool is_far_ (EnzoBlock * enzo_block, Index index_b,
                double * ra, double * rb) const;

  long long * volume_(Block * block) {
    return block->data()->scalar_long_long().value(is_volume_);
  }

protected: // attributes

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

#endif /* ENZO_ENZO_METHOD_FMM_HPP */
