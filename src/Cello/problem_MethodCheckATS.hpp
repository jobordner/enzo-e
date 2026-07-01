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
    : Method("check_ats"), field_name_(), error_curr_(), error_prev_()
  {
    field_name_ = p.value<std::string>("field_name","ats_field");
    error_curr_ = p.value<std::string>("error_curr","ats_error_curr");
    error_prev_ = p.value<std::string>("error_prev","ats_error_prev");
    init_refresh_();
  }

  MethodCheckATS()
    : Method("check_ats"), field_name_(), error_curr_(), error_prev_()
  {
    init_refresh_();
  }

  /// Charm++ PUP::able declarations
  PUPable_decl(MethodCheckATS);

  /// Charm++ PUP::able migration constructor
  MethodCheckATS (CkMigrateMessage *m)
    : Method (m), field_name_(), error_curr_(), error_prev_()
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p) 
  {
    TRACEPUP;
    Method::pup(p);
    p | field_name_;
    p | error_curr_;
    p | error_prev_;
  }

public: // virtual methods

  /// Apply the method to advance a block one timestep 
  virtual void compute( Block * block) throw() override;

  /// Compute maximum timestep for this method
  virtual double timestep ( Block * block) throw() override;

protected: // methods

  /// Initialize refresh
  void init_refresh_();

  /// Test that ghost values are expected
  void test_field_(Block * block,
                   cello_float * array,
                   cello_float * error,
                   cello_float time,
                   int mx, int my, int mz,
                   int gx, int gy, int gz);

  bool compare_ (const cello_float & a, const cello_float & b) const;

protected: // attributes

  /// Field name for testing
  std::string field_name_;
  /// Field names to record errors
  std::string error_curr_;
  std::string error_prev_;
};

#endif /* PROBLEM_METHOD_CHECK_ATS_HPP */
