// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMethodBalance.hpp
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     <2022-12-12 Mon>
/// @brief    [\ref Enzo] Declaration for the EnzoMethodBalance class

#ifndef ENZO_ENZO_METHOD_BALANCE_HPP
#define ENZO_ENZO_METHOD_BALANCE_HPP

class EnzoMethodBalance : public Method {

  /// @class    EnzoMethodBalance
  /// @ingroup  Enzo
  /// @brief    [\ref Enzo] Encapsulate Enzo's PPM hydro method

public: // interface

  /// Create a new EnzoMethodBalance object
  EnzoMethodBalance(float limit_rel);

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoMethodBalance);

  /// Charm++ PUP::able migration constructor
  EnzoMethodBalance (CkMigrateMessage *m)
    : Method (m),
      limit_rel_(1.0),
      timer_()
  {}

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  Timer & timer ()
  { return enzo::simulation()->method_balance_timer(); }
  void do_migrate(EnzoBlock * enzo_block, int migrated);
  void done(EnzoBlock * enzo_block);

public: // virtual methods

  /// Apply the method to advance a block one timestep 
  virtual void compute( Block * block) throw();

  virtual std::string name () throw () 
  { return "balance"; }

protected: // attributes

  /// Limit p on relative number of blocks to migrate; if M blocks
  /// want to migrate out of a total of N, then migrate if r < p*N/M,
  /// where r random between 0.0 and 1.0
  float limit_rel_;

  /// Timer used for monitoring
  Timer timer_;
};

#endif /* ENZO_ENZO_METHOD_BALANCE_HPP */
