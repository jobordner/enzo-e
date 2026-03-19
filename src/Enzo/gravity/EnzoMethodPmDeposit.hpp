// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMethodPmDeposit.hpp
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     2016-04-25
/// @brief    [\ref Enzo] Implementation of mass-deposition for PM particle-mesh 

#ifndef ENZO_ENZO_METHOD_PM_DEPOSIT_HPP
#define ENZO_ENZO_METHOD_PM_DEPOSIT_HPP

class EnzoMethodPmDeposit : public Method {

  /// @class    EnzoMethodPmDeposit
  /// @ingroup  Enzo
  /// @brief    [\ref Enzo] Declare Enzo's Particle-mesh method class

public: // interface

  /// Create a new EnzoMethodPmDeposit object
  EnzoMethodPmDeposit(ParameterGroup p);

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoMethodPmDeposit);
  
  /// Charm++ PUP::able migration constructor
  EnzoMethodPmDeposit (CkMigrateMessage *m)
    : Method (m),
      alpha_(0.0)
  { }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);
  
  /// Apply the method to advance a block one timestep 
  virtual void compute( Block * block) throw();

  virtual std::string name () throw () 
  { return "pm_deposit"; }

  void restrict_send(EnzoBlock * enzo_block);
  void restrict_recv(EnzoBlock * enzo_block, FieldMsg * field_message);

protected: // methods

  /// Access the Field message for buffering restriction data
  FieldMsg ** pmsg_restrict_(Block * block, int ic)
  {
    ScalarData<void *> * scalar_data = block->data()->scalar_data_void();
    ScalarDescr *        scalar_descr = cello::scalar_descr_void();
    return (FieldMsg **)scalar_data->value(scalar_descr,i_msg_restrict_[ic]);
  }

  /// Access the restrict Sync Scalar value for the Block
  Sync * psync_restrict_(Block * block)
  {
    ScalarData<Sync> * scalar_data = block->data()->scalar_data_sync();
    ScalarDescr *      scalar_descr = cello::scalar_descr_sync();
    return scalar_data->value(scalar_descr,i_sync_restrict_);
  }

  void continue_after_restrict_(Block * block);

  FieldMsg * pack_field_
  (EnzoBlock *, int index_field, int refresh_type, int ic3[3]);

  void unpack_field_
  (EnzoBlock *, FieldMsg *, int index_field, int refresh_type);


protected: // attributes

  /// Deposit at time + alpha*dt
  double alpha_;

  /// Total density field id
  int idt_;

  /// Restrict messages
  int i_msg_restrict_[8];
  int i_sync_restrict_;
};

#endif /* ENZO_ENZO_METHOD_PM_DEPOSIT_HPP */
