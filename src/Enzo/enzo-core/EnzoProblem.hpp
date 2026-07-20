// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoProblem.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2012-03-03 14:43:11
/// @brief    [\ref Enzo] Declaration of the EnzoProblem class

#ifndef ENZO_ENZO_PROBLEM_HPP
#define ENZO_ENZO_PROBLEM_HPP

class EnzoProblem : public Problem {

  /// @class    EnzoProblem
  /// @ingroup  Enzo
  /// @brief    [\ref Enzo] 

public: // interface

  /// Constructor
  EnzoProblem() throw();

  /// Destructor
  virtual ~EnzoProblem() throw();

  /// CHARM++ PUP::able declaration
  PUPable_decl(EnzoProblem);

  /// CHARM++ migration constructor
  EnzoProblem(CkMigrateMessage *m)
    : Problem (m)
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  virtual Compute * create_compute
  ( std::string_view name,
    Config * config ) throw() override ;

private: // functions

  /// Create named boundary conditions object
  virtual Boundary * create_boundary_ 
  (std::string_view type,
   int index,
   Config * config,
   Parameters * parameters
   ) throw() override ;

  /// Create named initialization object
  virtual Initial *  create_initial_ 
  (std::string_view type, 
   int index,
   Config * config,
   Parameters * parameters) throw() override;

  /// Create named physics object
  virtual Physics *  create_physics_ 
  (std::string_view type, 
   int index,
   Config * config,
   Parameters * parameters) throw() override;

  /// Create stopping criteria
  virtual Stopping * create_stopping_ 
  (std::string_view type, Config * config) throw() override;

  /// Create named refine object
  virtual Refine * create_refine_ 
  (std::string_view type,
   int         index,
   Config * config, 
   Parameters * parameters) throw() override;

  /// Create named solver object
  virtual Solver * create_solver_ 
  (std::string_view type,
   int index_solver, 
   Config * config) throw() override;

  /// Create named method object
  virtual Method * create_method_ 
  (std::string_view type,
   int index_method, 
   Config * config,
   const Factory * factory) throw() override;

  /// Create named interpolation object
  virtual Prolong * create_prolong_ (std::string_view type) throw() override;

  /// Create named restriction object
  virtual Restrict * create_restrict_ (std::string_view type) throw() override;

  /// Create a Units object
  virtual Units *  create_units_ 
  (Config * config) throw() override;

  /// Method that gets called at the end of initialize_physics
  virtual void initialize_physics_coda_(Config * config,
                                        Parameters * parameters) throw() override;

protected: // attributes

};

#endif /* ENZO_ENZO_PROBLEM_HPP */

