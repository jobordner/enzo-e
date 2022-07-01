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
    : Method (m), theta_(0), min_level_(0)
  {}

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  void traverse (EnzoBlock * enzo_block, Index index_b, int type_b);

public: // virtual methods

  /// Apply the method to advance a block one timestep 
  virtual void compute( Block * block) throw();

  virtual std::string name () throw () 
  { return "fmm"; }

  
protected: // interface

  bool is_far_ (EnzoBlock * enzo_block, Index index_b,
                double * ra, double * rb) const;
  
protected: // attributes

  /// Parameter controlling the multipole acceptance criterion, defined
  /// as distance(A,B) > theta * (radius(A) + radius(B))
  double theta_;

  /// Minimum mesh refinement level (saved for efficiency)
  int min_level_;
};

#endif /* ENZO_ENZO_METHOD_FMM_HPP */
