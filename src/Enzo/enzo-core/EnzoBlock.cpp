// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoBlock.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     Thu Mar  3 23:02:02 PST 2011
/// @brief    Implementation of the EnzoBlock class

#include "cello.hpp"
#include "charm_simulation.hpp"

#include "enzo.hpp"

//----------------------------------------------------------------------

EnzoBlock::EnzoBlock( CkMigrateMessage *m)
  : CBase_EnzoBlock (m)
{
  // replace Block's State with EnzoState
  state_ = std::make_shared<EnzoState>(0, 0.0, 0.0, false);
}

//----------------------------------------------------------------------

EnzoBlock::EnzoBlock( MsgType msg_type)
  : CBase_EnzoBlock (msg_type)
{
  // replace Block's State with EnzoState
  state_ = std::make_shared<EnzoState>(0, 0.0, 0.0, false);

  if (msg_type == MsgType::msg_check) {
    auto * msg_check = enzo::simulation()->get_msg_check(thisIndex);
    initialize_from_checkpoint(msg_check);
  } else if (msg_type == MsgType::msg_refine) {
    auto * msg_refine = enzo::simulation()->get_msg_refine(thisIndex);
    initialize_after_refinement(msg_refine);
  }
}

//----------------------------------------------------------------------

void EnzoBlock::initialize_from_checkpoint(EnzoMsgCheck * msg)
{
  PERF_START(iperf_block);
  restart_set_data_(msg);
  initialize();
  PERF_STOP(iperf_block);
}

//----------------------------------------------------------------------

void EnzoBlock::initialize_after_refinement(MsgRefine * msg)
{
  int io_reader = msg->restart_io_reader_;

  Block::initialize_after_refinement(msg);

  initialize();

  // If refined block and restarting, notify file reader block is
  // created
  if (io_reader >= 0) {
    proxy_io_enzo_reader[io_reader].p_block_created();
  }
}

//======================================================================

EnzoBlock::~EnzoBlock()
{
}

//----------------------------------------------------------------------

void EnzoBlock::ckAboutToMigrate(void)
{
  Block::ckAboutToMigrate();
}

//----------------------------------------------------------------------

void EnzoBlock::ckJustMigrated(void)
{
  Block::ckJustMigrated();
  if (!coarsened_)
    proxy_enzo_simulation[0].p_method_balance_check();
}

//----------------------------------------------------------------------

void EnzoBlock::pup(PUP::er &p)
{

  TRACEPUP;

  CBase_EnzoBlock::pup(p);

  const int in = cello::index_static();

  static bool warn1[CONFIG_NODE_SIZE] = {true};
  if (warn1[in]) {
    warn1[in] = false;
    WARNING("EnzoBlock::pup()", "skipping SubgridFluxes (not used)");
  }

  // If tagged for deletion, then delete after unpacking
  if (coarsened_ && p.isUnpacking()) {
    ckDestroy();
  }
}

//----------------------------------------------------------------------

bool EnzoBlock::spawn_child_blocks() throw()
{
  int level = index_.level();
  if (level >= 0) {

    if (level + 1 <= enzo::config()->refined_regions_lower.size()) {

      std::vector<int> lower = enzo::config()->refined_regions_lower.at(level);
      std::vector<int> upper = enzo::config()->refined_regions_upper.at(level);

      int ix, iy, iz, nx, ny, nz;
      index_global(&ix, &iy, &iz, &nx, &ny, &nz);
      if (lower.at(0) <= ix && ix < upper.at(0)) {
        if (lower.at(1) <= iy && iy < upper.at(1)) {
          if (lower.at(2) <= iz && iz < upper.at(2)) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

//----------------------------------------------------------------------

void EnzoBlock::create_initial_child_blocks()
{
  bool spawn_children = spawn_child_blocks();
  if (spawn_children) {instantiate_children();}
}

//----------------------------------------------------------------------

void EnzoBlock::instantiate_children() throw()
{
  child_face_level_curr_.resize(cello::num_children()*27);
  int num_field_blocks = 1;

  int nx, ny, nz;
  data()->field().size(&nx, &ny, &nz);

  const int rank = cello::rank();
  ItChild it_child(rank);
  int ic3[3];
  // WARNING: code duplication in control_adapt.cpp adapt_refine_()
  while (it_child.next(ic3)) {
    Index index_child = index_.index_child(ic3);
    DataMsg * data_msg = NULL;

    std::vector<int> face_level;
    face_level.resize(27);
    const int o = 27*IC3(ic3);
    for (size_t i=0; i<face_level.size(); i++) {
      face_level[i] = child_face_level_curr_[o+i];
    }
    int face_type;
    MsgRefine * msg = new MsgRefine
      (index_child,
       nx,ny,nz,
       num_field_blocks,
       adapt_step_,
       face_type = +1,
       face_level,
       &adapt_,state_.get());

    msg->set_data_msg(data_msg);
    enzo::simulation()->refine_create_block (msg);

    children_.push_back(index_child);
  }

  adapt_.set_valid(false);
  is_leaf_ = false;
}
