// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodOrderRotate.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2026-08-17
/// @brief    [\ref Problem] Declaration of the MethodOrderRotate class for
///           debugging Cello's dynamic load balancing, analogous to
///           Charm++ "RotateLB"

#ifndef PROBLEM_METHOD_ORDER_ROTATE_HPP
#define PROBLEM_METHOD_ORDER_ROTATE_HPP

class MethodOrderRotate : public Method {

  /// @class    MethodOrderRotate
  /// @ingroup  Problem
  /// @brief    [\ref Problem] 

public: // interface

  /// Constructor
  MethodOrderRotate() throw();

  /// Charm++ PUP::able declarations
  PUPable_decl(MethodOrderRotate);
  
  /// Charm++ PUP::able migration constructor
  MethodOrderRotate (CkMigrateMessage *m)
    : Method (m)
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {
    Method::pup(p);
  }

public: // virtual methods

  /// Apply the method to determine the ordering of blocks
  virtual void compute( Block * block) throw();

  virtual bool is_active(std::shared_ptr<State> state, int level) override
  { return true; }

public: // methods

};

#endif /* PROBLEM_METHOD_ORDER_ROTATE_HPP */

