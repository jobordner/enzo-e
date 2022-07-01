// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMethodFmm.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2022-06-27
/// @brief    Implements the EnzoMethodFmm class

#include <iostream>
#include "cello.hpp"
#include "enzo.hpp"

#define DEBUG_FMM
#define PRINT_APPROX
#define PRINT_DIRECT
//----------------------------------------------------------------------

EnzoMethodFmm::EnzoMethodFmm (double theta)
  : Method(),
    theta_(theta),
    min_level_(0) // actual initialization deferred to compute() to
                  // avoid dependency issues
{
  cello::simulation()->refresh_set_name(ir_post_,name());

  Refresh * refresh = cello::refresh(ir_post_);

  // add a field to avoid deadlock
  refresh->add_field("density");
}

//----------------------------------------------------------------------

void EnzoMethodFmm::pup (PUP::er &p)
{
  // NOTE: change this function whenever attributes change

  TRACEPUP;

  Method::pup(p);

  p | theta_;
  p | min_level_;
}

//----------------------------------------------------------------------

void EnzoMethodFmm::compute ( Block * block) throw()
{
  min_level_ = cello::hierarchy()->min_level();
  Index index = block->index();
  if (block->level() == min_level_) {
    int type = block->is_leaf() ? 1 : 0;
    enzo::block_array()[index].p_method_fmm_traverse(index,type);
  }
}

//----------------------------------------------------------------------

void EnzoBlock::p_method_fmm_traverse(Index index, int type)
{
  EnzoMethodFmm * method = static_cast<EnzoMethodFmm*> (this->method());
  method->traverse(this, index, type);
}

//----------------------------------------------------------------------

void EnzoMethodFmm::traverse
(EnzoBlock * enzo_block, Index index_b, int type_b)
{
  Index index_a = enzo_block->index();
  const bool known_leaf_b = (type_b != -1);
  const bool is_leaf_a = enzo_block->is_leaf();

  if (! known_leaf_b) {
    // If unknown wether B is leaf or not, flip arguments and send to B
    enzo::block_array()[index_b].p_method_fmm_traverse
      (index_a, is_leaf_a ? 1 : 0);
    return;
  }

  // Determine if blocks are "far", and save radii for later
  double ra,rb;
  bool mac = is_far_(enzo_block,index_b,&ra,&rb);

  // is_leaf values: 0 no, 1 yes -1 unknown
  const bool is_leaf_b = (type_b == 1);

  ItChild it_child_a(cello::rank());
  ItChild it_child_b(cello::rank());
  int ica3[3],icb3[3];
  
  if (mac) {

#ifdef PRINT_APPROX
    CkPrintf ("APPROX %s %s\n",
              enzo_block->name().c_str(),
              enzo_block->name(index_b).c_str());
#endif

  } else if (is_leaf_a && is_leaf_b) {

#ifdef PRINT_DIRECT
    CkPrintf ("DIRECT %s %s\n",
              enzo_block->name().c_str(),
              enzo_block->name(index_b).c_str());
#endif

  } else if (is_leaf_a) {

    while (it_child_b.next(icb3)) {

      Index index_b_child = index_b.index_child(icb3,min_level_);
      traverse(enzo_block,index_b_child,-1);
    }

  } else if (is_leaf_b) {

    while (it_child_a.next(ica3)) {

      Index index_a_child = index_a.index_child(ica3,min_level_);
      enzo::block_array()[index_a_child].p_method_fmm_traverse
        (index_b,type_b);
    }
  } else if (index_a == index_b) {

    // loop 1 through A children
    while (it_child_a.next(ica3)) {
      int ica = ica3[0] + 2*(ica3[1] + 2*ica3[2]);
      Index index_a_child = index_a.index_child(ica3,min_level_);
      // loop 2 through A children
      while (it_child_b.next(icb3)) {
        int icb = icb3[0] + 2*(icb3[1] + 2*icb3[2]);
        Index index_b_child = index_b.index_child(icb3,min_level_);
        // avoid calling both traverse (A,B) and traverse (B,A)
        if (ica <= icb) {
          // call traverse on child 1 and child 2
          enzo::block_array()[index_a_child].p_method_fmm_traverse
            (index_b_child,-1);
        }
      }
    }
  } else if (ra < rb) {
    while (it_child_b.next(icb3)) {

      Index index_b_child = index_b.index_child(icb3,min_level_);
      traverse(enzo_block,index_b_child,-1);
    }
  } else {
    while (it_child_b.next(icb3)) {

      Index index_b_child = index_b.index_child(icb3,min_level_);
      traverse(enzo_block,index_b_child,-1);
    }
  }
}

//======================================================================

bool EnzoMethodFmm::is_far_ (EnzoBlock * enzo_block,
                             Index index_b, double *ra, double *rb) const
{
  double iam3[3],iap3[3],ibm3[3],ibp3[3],na3[3],nb3[3];
  enzo_block->lower(iam3,iam3+1,iam3+2);
  enzo_block->upper(iap3,iap3+1,iap3+2);
  enzo_block->lower(ibm3,ibm3+1,ibm3+2,&index_b);
  enzo_block->upper(ibp3,ibp3+1,ibp3+2,&index_b);
  *ra=0;
  *rb=0;
  double ra3[3]={0,0,0},rb3[3]={0,0,0},d3[3]={0,0,0};
  double d = 0;
  for (int i=0; i<cello::rank(); i++) {
    ra3[i] = (iap3[i] - iam3[i]);
    rb3[i] = (ibp3[i] - ibm3[i]);
    *ra += ra3[i]*ra3[i];
    *rb += rb3[i]*rb3[i];
    double ca = (iam3[i]+0.5*ra3[i]);
    double cb = (ibm3[i]+0.5*rb3[i]);
    d3[i] = (ca - cb);
    d += d3[i]*d3[i];
  }

  d = sqrt(d);
  *ra = 0.5*sqrt(*ra);
  *rb = 0.5*sqrt(*rb);

  return  (d > theta_ * (*ra + *rb));
}
