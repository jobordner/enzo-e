// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMethodBalance.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     <2022-12-09 Fri>
/// @brief    Implements the EnzoMethodBalance class

#include "cello.hpp"
#include "enzo.hpp"

//----------------------------------------------------------------------

EnzoMethodBalance::EnzoMethodBalance(float limit_rel)
  : Method(),
    limit_rel_(limit_rel)
{
  cello::define_field("density");

  cello::simulation()->refresh_set_name(ir_post_,name());
  Refresh * refresh = cello::refresh(ir_post_);
  refresh->add_field("density");
}

//----------------------------------------------------------------------

void EnzoMethodBalance::pup (PUP::er &p)
{
  // NOTE: change this function whenever attributes change

  TRACEPUP;

  Method::pup(p);

  p | limit_rel_;
}

//----------------------------------------------------------------------

void EnzoMethodBalance::compute ( Block * block) throw()
{
  // Output that we're calling the load balancer
  if (block->index().is_root()) {
    cello::monitor()->print
      ("Method EnzoMethodBalance",
       "entering Cello load-balancer");
  }

  // Start timer
  if (! timer().is_running()) {
    timer().clear();
    timer().start();
  }

  long long index, count;
  block->get_order (&index,&count);

  // Use long long to prevent overflow on large runs
  int ip_next = (long long) CkNumPes()*index/count;

  block->set_ip_next(ip_next);

  int count_local = 0;
  if (ip_next != CkMyPe()) {
    count_local = 1;
  }

  CkCallback callback
    (CkIndex_EnzoSimulation::r_method_balance_count(nullptr), 0,
     proxy_enzo_simulation);

  block->contribute(sizeof(int), &count_local,
                    CkReduction::sum_int, callback);

}

void EnzoSimulation::r_method_balance_count(CkReductionMsg * msg)
{
  // Include self-call of balance check to prevent hanging when no
  // blocks migrate
  int * count_total = (int * )msg->getData();
  sync_method_balance_.set_stop(*count_total + 1);

  if (CkMyPe() == 0) {
    cello::monitor()->print
      ("Method EnzoMethodBalance",
       "migrating at most %d blocks",(*count_total));
  }

  // Initiate migration
  enzo::block_array().p_method_balance_migrate(*count_total);
  p_method_balance_check(0);
}

void EnzoBlock::p_method_balance_migrate(int num_migrate)
{
  static_cast<EnzoMethodBalance*> (method())->do_migrate(this, num_migrate);
}

void EnzoMethodBalance::do_migrate(EnzoBlock * enzo_block, int num_migrate)
{
  int ip_next = enzo_block->ip_next();
  enzo_block->set_ip_next(-1);
  if (ip_next != CkMyPe()) {
    long long index,count;
    enzo_block->get_order (&index,&count);
    // migrate at most (about) limit_rel_ out of total blocks
    double r = static_cast<double>(std::rand()) / RAND_MAX;
    if (r < limit_rel_*count/num_migrate) {
      enzo_block->migrateMe(ip_next);
    } else {
      // make sure non-migrated block gets counted
      proxy_enzo_simulation[0].p_method_balance_check(0);
    }
  }
}

void EnzoSimulation::p_method_balance_check(int migrated)
{
  // Write progress num_updates times
  int curr=sync_method_balance_.value();
  int stop=sync_method_balance_.stop();
  int num_updates = enzo::config()->method_balance_num_updates;
  if (stop!=0) {
    if ( (long long)(num_updates* curr   /stop) !=
        ((long long)(num_updates*(curr+1)/stop))) {
      CkPrintf ("    EnzoMethodBalance: %5.1f%% in %3.1f s\n",
                100.*(curr+1)/stop, method_balance_timer_.value());
      fflush(stdout);
    }
  }

  // count actual number of blocks migrated
  method_balance_count_ += migrated;

  if (sync_method_balance_.next()) {
    // done migrating
    sync_method_balance_.reset();
    enzo::block_array().doneInserting();
    enzo::block_array().p_method_balance_done();
    cello::monitor()->print
      ("Method EnzoMethodBalance",
       "done migrating %d blocks in %.2f s",
       method_balance_count_);
    method_balance_count_ = 0;
  }
}

void EnzoBlock::p_method_balance_done()
{
  static_cast<EnzoMethodBalance*> (method())->done(this);
}

void EnzoMethodBalance::done(EnzoBlock * enzo_block)
{
  // Stop timer
  if (timer().is_running())
    timer().stop();
  enzo_block->compute_done();
}

