// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodCheckATS.hpp
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     Thu Aug 21, 2025
/// @brief    [\ref Problem] Declaration of MethodCheckATS Method

#ifndef PROBLEM_METHOD_CHECK_ATS_HPP
#define PROBLEM_METHOD_CHECK_ATS_HPP

class MethodCheckATS : public Method {

  /// @class    MethodCheckATS
  /// @ingroup  Problem
  ///
  /// @brief [\ref Problem] Check ATS method, used for checking
  /// MethodATS correctness

public: // interface

  /// Create a new MethodCheckATS object
  MethodCheckATS ( ParameterGroup p )
    : Method()
  {
    init_refresh_();
  }

  MethodCheckATS()
    : Method()
  {
    init_refresh_();
  }

  /// Charm++ PUP::able declarations
  PUPable_decl(MethodCheckATS);

  /// Charm++ PUP::able migration constructor
  MethodCheckATS (CkMigrateMessage *m)
    : Method (m)
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p) 
  {
    TRACEPUP;
    Method::pup(p);
  }

public: // virtual methods

  /// Apply the method to advance a block one timestep 
  virtual void compute( Block * block) throw();

  virtual std::string name () throw () 
  { return "check_ats"; }

  /// Compute maximum timestep for this method
  virtual double timestep ( Block * block) throw();

protected: // methods

  /// Initialize refresh
  void init_refresh_();

  /// Test that ghost values are expected
  void test_ghosts_(Block * block,
                    cello_float * array_curr,
                    cello_float * error_curr,
                    int mx, int my, int mz,
                    int gx, int gy, int gz);

  /// Test that field is constant including all ghosts
  void test_values_(Block * block,
                    cello_float * array_curr,
                    cello_float * error_curr,
                    int mx, int my, int mz,
                    int gx, int gy, int gz,
                    double time);

  /// Test that previous saved value is expected

  void test_history_(Block * block,
                     cello_float * array_curr,
                     cello_float * array_prev,
                     cello_float * error_curr,
                     int mx, int my, int mz,
                     int gx, int gy, int gz,
                     int level, double dt);

protected: // attributes

};

#endif /* PROBLEM_METHOD_CHECK_ATS_HPP */
