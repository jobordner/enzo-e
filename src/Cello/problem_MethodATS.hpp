// See LICENSE_CELLO file for license and copyright information

/// @file     problem_MethodATS.hpp
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     Wed Jun 18 2025
/// @brief    [\ref Problem] Declaration of MethodATS Method

#ifndef PROBLEM_METHOD_ATS_HPP
#define PROBLEM_METHOD_ATS_HPP

class MethodATS : public Method {

  /// @class    MethodATS
  /// @ingroup  Problem
  ///
  /// @brief [\ref Problem] ATS method, used for testing adaptive time-stepping
  /// by setting different timesteps per level

public: // interface

  /// Create a new MethodATS object
  MethodATS ( ParameterGroup p )
    : Method("ats"), dt_level_(), field_name_()
  {
    field_name_ = p.value<std::string>("field_name","ats_field");
    int n=p.list_length("dt_level");
    if (n>0) {
      dt_level_.resize(n);
      int k=0;
      for (auto & dt: dt_level_) {
        dt = p.value(k,"dt_level",pow(0.5,k));
        k++;
      }
    }
    init_refresh_();
  }

  MethodATS()
    : Method("ats"), dt_level_(), field_name_()
  {
    init_refresh_();
  }

  /// Charm++ PUP::able declarations
  PUPable_decl(MethodATS);

  /// Charm++ PUP::able migration constructor
  MethodATS (CkMigrateMessage *m)
    : Method (m), dt_level_(), field_name_()
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p) 
  {
    TRACEPUP;
    Method::pup(p);
    p | dt_level_;
    p | field_name_;
  }

public: // virtual methods

  /// Apply the method to advance a block one timestep 
  virtual void compute( Block * block) throw() override;

  /// Compute maximum timestep for this method
  virtual double timestep ( Block * block) throw() override;

protected: // methods

  /// Initialize refresh
  void init_refresh_();

  /// Advance the field value by dt
  void advance_field_(cello_float * array,
                      int mx, int my, int mz,
                      int gx, int gy, int gz,
                      double dt);

protected: // attributes

  /// Level time steps
  std::vector<double> dt_level_;

  /// Field used for storing current time for testing
  std::string field_name_;
};

#endif /* PROBLEM_METHOD_ATS_HPP */
