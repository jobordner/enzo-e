// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodOrderUnigrid.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2022-03-01
/// @brief    [\ref Problem] Declaration of the MethodOrderUnigrid class for
///           generating the Unigrid (Z-)ordering of blocks in the hierarchy

#ifndef PROBLEM_METHOD_ORDER_UNIGRID_HPP
#define PROBLEM_METHOD_ORDER_UNIGRID_HPP

class MethodOrderUnigrid : public Method {

  /// @class    MethodOrderUnigrid
  /// @ingroup  Problem
  /// @brief    [\ref Problem] 

public: // interface

  /// Constructor
  MethodOrderUnigrid() throw();

  /// Charm++ PUP::able declarations
  PUPable_decl(MethodOrderUnigrid);
  
  /// Charm++ PUP::able migration constructor
  MethodOrderUnigrid (CkMigrateMessage *m)
    : Method (m)
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {
    Method::pup(p);
    p | is_index_;
    p | is_count_;
    p | nx_;
    p | ny_;
    p | nz_;
  }

public: // virtual methods
  
  /// Apply the method to determine the Unigrid ordering of blocks
  virtual void compute( Block * block) throw();

  virtual std::string name () throw () 
  { return "order_unigrid"; }

private: // methods

  /// Return the pointer to the Block's Unigrid ordering index 
  long long * pindex_(Block * block);

  /// Return the pointer to the number of Block indices
  long long * pcount_(Block * block);

private: // functions


private: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Block Scalar<int> index
  int is_index_;
  /// Block Scalar<int> count
  int is_count_;

  /// Mesh blocking
  int nx_,ny_,nz_;
};

#endif /* PROBLEM_METHOD_ORDER_UNIGRID_HPP */

