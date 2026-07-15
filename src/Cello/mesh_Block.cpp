// See LICENSE_CELLO file for license and copyright information

/// @file     mesh_Block.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     Mon Feb 28 13:22:26 PST 2011
/// @brief    Implementation of the Block object

#include "cello.hpp"
#include "mesh.hpp"
#include "main.hpp"
#include "charm_simulation.hpp"

// KEEP CONSISTENT WITH _comm.hpp: phase_type
const char * phase_name[] = {
  "unknown",
  "initial_enter",
  "initial_exit",
  "adapt_enter",
  "adapt_called",
  "adapt_next",
  "adapt_end",
  "adapt_exit",
  "compute_enter",
  "compute_continue",
  "compute_exit",
  "refresh_enter",
  "refresh_exit",
  "stopping_enter",
  "stopping_exit",
  "output_enter",
  "output_exit",
  "restart",
  "balance",
  "exit"
};

// #define TRACE_BLOCK

Block::Block ()
  : CBase_Block(),
    index_(thisIndex),
    data_(NULL),
    child_data_(NULL),
    level_next_(0),
    state_(new State (0, 0.0, 0.0, false)),
    index_initial_(0),
    children_(),
    sync_coarsen_(),
    sync_count_(),
    sync_max_(),
    adapt_(),
    child_face_level_curr_count_(),
    child_face_level_next_count_(),
    count_coarsen_(0),
    adapt_step_(0),
    adapt_ready_(false),
    adapt_balanced_(false),
    adapt_changed_(0),
    coarsened_(false),
    is_leaf_((thisIndex.level() >= 0)),
    age_(0),
    ip_next_(-1),
    name_(""),
    index_method_(0),
    index_solver_(),
    refresh_(),
    level_lower_(-1),
    level_upper_(-1),
    order_index_(0),
    order_count_(1),
    order_next_()
{
#ifdef TRACE_BLOCK
  CkPrintf ("%d TRACE_BLOCK %s Block::Block()\n",
            CkMyPe(),name(thisIndex).c_str());
#endif
  PERF_START(iperf_block);
  init_refresh_();
  init_adapt_(nullptr);
}


//----------------------------------------------------------------------

Block::Block (CkMigrateMessage *m)
  : CBase_Block(m)
{
#ifdef TRACE_BLOCK
  CkPrintf ("%d TRACE_BLOCK %s Block::Block(CkMigrateMessage)\n",
            CkMyPe(),name(thisIndex).c_str());
#endif
}

//----------------------------------------------------------------------

Block::Block ( MsgType msg_type )
  : CBase_Block(),
    index_(thisIndex),
    data_(NULL),
    child_data_(NULL),
    level_next_(0),
    state_(new State (0, 0.0, 0.0, false)),
    index_initial_(0),
    children_(),
    sync_coarsen_(),
    sync_count_(),
    sync_max_(),
    adapt_(),
    child_face_level_curr_(),
    child_face_level_next_(),
    count_coarsen_(0),
    adapt_step_(0),
    adapt_ready_(false),
    adapt_balanced_(false),
    adapt_changed_(0),
    coarsened_(false),
    is_leaf_((thisIndex.level() >= 0)),
    age_(0),
    ip_next_(-1),
    name_(""),
    name8_(""),
    index_method_(0),
    index_solver_(),
    refresh_(),
    level_lower_(-1),
    level_upper_(-1),
    order_index_(0),
    order_count_(1),
    order_next_()
{
#ifdef TRACE_BLOCK
  CkPrintf ("%d TRACE_BLOCK %s Block::Block(MsgType)\n",  CkMyPe(),name(thisIndex).c_str());
#endif

  PERF_START(iperf_block);

  init_refresh_();
  usesAtSync = true;

  thisIndex.array(array_,array_+1,array_+2);

  PERF_STOP(iperf_block);
}

//----------------------------------------------------------------------

void Block::set_msg_refine(MsgRefine * msg)
{
  PERF_START(iperf_block);

  std::vector<int> face_level;
  face_level.clear();
  init_refine_
    (msg->index_,
     msg->nx_, msg->ny_, msg->nz_,
     msg->num_field_blocks_,
     msg->num_adapt_steps_,
     0, nullptr,
     msg->face_type_,
     msg->face_level_,
     msg->adapt_parent_,
     msg->state_);

  init_adapt_(msg->adapt_parent_);

  apply_initial_(msg);

#ifdef TRACE_BLOCK
  {
  CkPrintf ("%d %s index TRACE_BLOCK set_msg_refine(MsgRefine) done\n",
            CkMyPe(),name(msg->index_).c_str());
  }
#endif

#ifdef TRACE_REFINE
  CkPrintf ("TRACE_REFINE %s\n",name().c_str());
  fflush(stdout);
#endif

  delete msg;
  PERF_STOP(iperf_block);
}

//----------------------------------------------------------------------

void Block::init_refine_
(
 Index index,
 int nx, int ny, int nz,
 int num_field_blocks,
 int num_adapt_steps,
 int narray, char * array, int face_type,
 const std::vector<int> & face_level,
 Adapt * adapt,
 State * state)
{
  index_ = index;
  *state_ = *state;

  // Initialize method state
  state_->alloc_method (cello::problem()->num_methods());

  adapt_step_ = num_adapt_steps;
  adapt_ready_ = false;
  adapt_balanced_ = false;
  adapt_changed_ = 0;

  // Enable Charm++ AtSync() dynamic load balancing

  Simulation * simulation = cello::simulation();

  Monitor * monitor = (simulation != NULL) ? simulation->monitor() : NULL;

  int ibx,iby,ibz;
  index.array(&ibx,&iby,&ibz);

  double xm,ym,zm;
  lower(&xm,&ym,&zm);
  double xp,yp,zp;
  upper(&xp,&yp,&zp);

  // Allocate block data

  data_ = new Data  (nx, ny, nz,
		     num_field_blocks,
		     xm,xp, ym,yp, zm,zp);

  data_->allocate(index.level());

  child_data_ = NULL;

  sync_coarsen_.reset();
  sync_coarsen_.set_stop(cello::num_children());

  // Initialize neighbor face levels

  const int nc = cello::num_children();
  if (face_level.size() == 0) {

    child_face_level_curr_.resize(nc*27);

    adapt_.reset_face_level_curr();
   // Compute and set the face levels of
    int if3[3], na3[3], face_levels[27] = {};
    size_array(na3,na3+1,na3+2);
    ItFace it_face = this->it_face(cello::config()->adapt_min_face_rank, index_);
    while (it_face.next(if3)) {
      Index neighbor_index = index_.index_neighbor(if3, na3);
      bool refine = refine_during_initialization(neighbor_index);
      face_levels[IF3(if3)] = refine ? 1 : 0;
    }
    adapt_.copy_face_level_curr(face_levels);
  } else {

    child_face_level_curr_.resize(nc*face_level.size());

    adapt_.copy_face_level_curr(face_level.data());

  }

  std::fill(child_face_level_curr_.begin(),
            child_face_level_curr_.end(), 0);

  // Initialize face level counts
  child_face_level_curr_count_.resize(nc*27);
  std::fill(child_face_level_curr_count_.begin(),
            child_face_level_curr_count_.end(), -1);

  child_face_level_next_count_.resize(nc*27);
  std::fill(child_face_level_next_count_.begin(),
            child_face_level_next_count_.end(), -1);

  initialize_child_face_levels_();

  adapt_.update_next_from_curr();
  child_face_level_next_ = child_face_level_curr_;

  const int level = this->level();

  int na3[3];
  size_array(na3,na3+1,na3+2);

  int ic3[3] = {0,0,0};
  if (level > 0) index_.child(level,ic3,ic3+1,ic3+2);

  if (narray != 0) {

    // Create field face of refined data from parent
    int if3[3] = {0,0,0};
    int g3[3];
    cello::field_descr()->ghost_depth(0,g3,g3+1,g3+2);
    Refresh * refresh = new Refresh;
    refresh->add_all_data();
    refresh -> set_adaptive_timestep
      (state_->state_type() == State::Type::Level);

    FieldFace * field_face = new FieldFace
      (level, +1, if3, ic3, g3, refresh, true);

    // Copy refined field data

    field_face -> array_to_face (array, data()->field());

    delete field_face;

  }

  if (simulation) simulation->data_insert_block(this);

  const int np = data()->particle().num_particles();
  if (np > 0) {
    if (simulation) simulation->data_insert_particles(np);
  }

  if (level > 0) {

    control_sync_quiescence (CkIndex_Main::p_adapt_end());

  }

  // Do not migrate the root Block (0,0,0) level (0)
  setMigratable(! index_.is_root());
}

//----------------------------------------------------------------------

void Block::initialize()
{
#ifdef TRACE_BLOCK
  CkPrintf ("TRACE_BLOCK %s initialize()\n",name().c_str());
  fflush(stdout);
#endif

  const bool initial_new = cello::config()->initial_new;

  if (! initial_new) {
    if (cello::is_initial_cycle(state_->cycle(),InitCycleKind::fresh) && level() <= 0) {
      CkCallback callback (CkIndex_Block::r_end_initialize(NULL), thisProxy);
      contribute(0,0,CkReduction::concat,callback);
    }
  }
}

//----------------------------------------------------------------------

void Block::pup(PUP::er &p)
{
  TRACEPUP;

  CBase_Block::pup(p);

  bool up = p.isUnpacking();

  if (up) data_ = new Data;
  p | *data_;

  // child_data_ may be NULL
  bool allocated=(child_data_ != NULL);
  p|allocated;
  if (allocated) {
    if (up) child_data_=new Data;
    // child_data_ guaranteed to be non-NULL: adding check for
    // Coverity static analysis
    if (child_data_) p|*child_data_;
  } else {
    child_data_ = NULL;
  }

  p | index_;
  PUParray(p,array_,3);
  p | level_next_;
  p | *state_;
  p | index_initial_;
  p | children_;
  p | sync_coarsen_;
  p | sync_count_;
  p | sync_max_;
  p | adapt_;
  p | child_face_level_curr_;
  p | child_face_level_next_;
  p | child_face_level_curr_count_;
  p | child_face_level_next_count_;
  p | count_coarsen_;
  p | adapt_step_;
  p | adapt_ready_;
  p | adapt_balanced_;
  p | adapt_changed_;
  // std::vector < MsgAdapt * > adapt_msg_list_;
  p | coarsened_;
  p | is_leaf_;
  p | age_;
  p | ip_next_;
  p | name_;
  p | name8_;
  p | index_method_;
  p | index_solver_;
  p | refresh_;
  // SKIP method_: initialized when needed

  if (up) {
    Simulation * simulation = cello::simulation();
    if (simulation != NULL) simulation->data_insert_block(this);
  }
  p | refresh_sync_list_;

  int len;

  len=refresh_recv_buffer_.size();
  p | len;
  if (up) {
    refresh_recv_buffer_.resize(len);
    for (int i=0; i<len; i++) refresh_recv_buffer_[i].clear();
  }

  len=refresh_send_buffer_.size();
  p | len;
  if (up) {
    refresh_send_buffer_.resize(len);
    for (int i=0; i<len; i++) refresh_send_buffer_[i].clear();
  }

  len=refresh_send_index_.size();
  p | len;
  if (up) {
    refresh_send_index_.resize(len);
    for (int i=0; i<len; i++) refresh_send_index_[i].clear();
  }
  
  p | level_lower_;
  p | level_upper_;

  p | order_index_;
  p | order_count_;
  p | order_next_;
}

//----------------------------------------------------------------------

void Block::ckAboutToMigrate(void)
{
  PERF_METHOD_STOP(method());
  CBase_Block::ckAboutToMigrate();
}

void Block::ckJustMigrated(void)
{
  PERF_METHOD_START(method());
  CBase_Block::ckJustMigrated();
}

//----------------------------------------------------------------------

ItFace Block::it_face
(int min_face_rank,
 Index index,
 const int * ic3,
 const int * if3) throw()
{
  int rank = cello::rank();
  int n3[3];
  int p3[3];
  size_array(n3,n3+1,n3+2);
  cello::hierarchy()->get_periodicity(p3,p3+1,p3+2);
  return ItFace (rank,min_face_rank,p3,n3,index,ic3,if3);
}

//----------------------------------------------------------------------

ItNeighbor Block::it_neighbor (Index index,
                               int min_face_rank,
                               int neighbor_type,
                               int coarse_level,
                               int level_lower,
                               int level_upper,
                               DirType dir_type) throw()
{
  if (min_face_rank == -1) {
    min_face_rank = cello::config()->adapt_min_face_rank;
  }
  int n3[3];
  size_array(&n3[0],&n3[1],&n3[2]);
  int p3[3];
  cello::hierarchy()->get_periodicity(p3,p3+1,p3+2);
  return ItNeighbor
    (this,min_face_rank,p3,n3,index,
     neighbor_type,coarse_level,level_lower,level_upper,
     dir_type);
}

//----------------------------------------------------------------------

Method * Block::method () throw ()
{
  Problem * problem = cello::problem();
  return (index_method_ < cello::num_method()) ?
    cello::method(index_method_) : nullptr;
}

//----------------------------------------------------------------------

Initial * Block::initial () throw ()
{
  Problem * problem = cello::problem();
  Initial * initial = problem->initial(index_initial_);
  return initial;
}

//----------------------------------------------------------------------

void Block::push_solver(int index_solver) throw()
{
  index_solver_.push_back(index_solver);
}

//----------------------------------------------------------------------

int Block::pop_solver() throw()
{
  int index = this->index_solver();
  ASSERT ("Block::pop_solver",
          "Trying to pop element off of empty Block::index_solver_ stack",
          index_solver_.size() > 0);
  index_solver_.resize(index_solver_.size()-1);
  return index;
}

//----------------------------------------------------------------------

Solver * Block::solver () throw ()
{
  Problem * problem = cello::problem();
  Solver * solver = problem->solver(index_solver());
  return solver;
}

//----------------------------------------------------------------------

void Block::print (FILE * fp_in) const
{
  FILE * fp = nullptr;
  if (fp_in == nullptr) {
    fp = fopen ((std::string("CB-")+name_).c_str(),"a");
  } else {
    fp = fp_in;
  }

  const int ip = CkMyPe();

  fprintf (fp,"%d %s PRINT_BLOCK name8_ = %s\n",ip,name_.c_str(),name8().c_str());
  fprintf (fp,"%d %s PRINT_BLOCK data_ = %p\n",ip,name_.c_str(),(void*)data_);
  fprintf (fp,"%d %s PRINT_BLOCK child_data_ = %p\n",ip,name_.c_str(),(void*)child_data_);

  int v3[3];index().values(v3);

  fprintf (fp,"%d %s PRINT_BLOCK index_ = %0x %0x %0x\n",ip,name_.c_str(),v3[0],v3[1],v3[2]);
  fprintf (fp,"%d %s PRINT_BLOCK array_ = %d %d %d\n",ip,name_.c_str(),array_[0],array_[1],array_[2]);
  fprintf (fp,"%d %s PRINT_BLOCK level_next_ = %d\n",ip,name_.c_str(),level_next_);
  fprintf (fp,"%d %s PRINT_BLOCK cycle_ = %d\n",ip,name_.c_str(),state_->cycle());
  fprintf (fp,"%d %s PRINT_BLOCK time_ = %f\n",ip,name_.c_str(),state_->time());
  fprintf (fp,"%d %s PRINT_BLOCK dt_ = %f\n",ip,name_.c_str(),state_->dt());
  fprintf (fp,"%d %s PRINT_BLOCK stop_ = %d\n",ip,name_.c_str(),state_->stopping());
  fprintf (fp,"%d %s PRINT_BLOCK index_initial_ = %d\n",ip,name_.c_str(),index_initial_);
  fprintf (fp,"%d %s PRINT_BLOCK children_.size() = %lu\n",ip,name_.c_str(),children_.size());
  fprintf (fp,"%d %s PRINT_BLOCK child_face_level_curr_.size() = %lu\n",ip,name_.c_str(),child_face_level_curr_.size());
  for (std::size_t i=0; i<child_face_level_curr_.size(); i++) {fprintf (fp,"%d ",child_face_level_curr_[i]);} fprintf (fp,"\n");
  sync_coarsen_.print("PRINT_BLOCK",fp);
  fprintf (fp,"%d %s PRINT_BLOCK sync_count_ %d: ",
           ip, name_.c_str(), (int)sync_count_.size());
  for (std::size_t i=0; i<sync_count_.size(); i++) {fprintf (fp,"%d ",sync_count_[i]);} fprintf (fp,"\n");
  fprintf (fp,"%d %s PRINT_BLOCK sync_max_ %d: ",
           ip, name_.c_str(), (int)sync_max_.size());
  for (std::size_t i=0; i<sync_max_.size(); i++) {fprintf (fp,"%d ",sync_max_[i]);} fprintf (fp,"\n");
  fprintf (fp,"%d %s PRINT_BLOCK child_face_level_next_.size() = %lu\n",ip,name_.c_str(),child_face_level_next_.size());
  for (std::size_t i=0; i<child_face_level_next_.size(); i++) {fprintf (fp,"%d ",child_face_level_next_[i]);} fprintf (fp,"\n");

  fprintf (fp,"%d %s PRINT_BLOCK count_coarsen_ = %d\n",
           ip,name_.c_str(),count_coarsen_);
  fprintf (fp,"%d %s PRINT_BLOCK adapt_step_ = %d\n",
           ip,name_.c_str(),adapt_step_);
  fprintf (fp,"%d %s PRINT_BLOCK adapt_ready_ = %s\n",
           ip,name_.c_str(),adapt_ready_?"true":"false");
  fprintf (fp,"%d %s PRINT_BLOCK adapt_balanced_ = %s\n",
           ip,name_.c_str(),adapt_balanced_?"true":"false");
  fprintf (fp,"%d %s PRINT_BLOCK adapt_changed_ = %d\n",
           ip,name_.c_str(),adapt_changed_);
  fprintf (fp,"%d %s PRINT_BLOCK adapt_msg_list_.size() = %lu\n",
           ip,name_.c_str(),adapt_msg_list_.size());

  fprintf (fp,"%d %s PRINT_BLOCK coarsened_ = %d\n",
           ip,name_.c_str(),coarsened_);
  fprintf (fp,"%d %s PRINT_BLOCK is_leaf_ = %d\n",ip,name_.c_str(),is_leaf_);
  fprintf (fp,"%d %s PRINT_BLOCK age_ = %d\n",ip,name_.c_str(),age_);
  fprintf (fp,"%d %s PRINT_BLOCK ip_next_ = %d\n",ip,name_.c_str(),ip_next_);
  fprintf (fp,"%d %s PRINT_BLOCK index_method_ = %d\n",
           ip,name_.c_str(),index_method_);
  fprintf (fp,"%d %s PRINT_BLOCK index_solver_.size() = %lu\n",
           ip,name_.c_str(),index_solver_.size());
  adapt_.print(std::string("Adapt-")+name_,this,fp);

  for (std::size_t i=0; i<refresh_.size(); i++) {
    refresh_[i]->print(fp);
  }


  if (fp_in == nullptr) {
    fclose (fp);
  }
}

//=====================================================================

void Block::compute_derived(const std::vector< std::string>& field_list
                            /* = std::vector< std::string>() */ ) throw ()
/// @param      field_list     list of fields that may be derived to compute
{
  TRACE("Block::compute_derived()");

  // compute all derived fields on this block

  Field field = data()->field();

  int nderived = field.groups()->size("derived");

  if (nderived > 0){

    Problem * problem = cello::problem();
    Config   * config  = (Config *) cello::config();

    // this is not a good way of doing this...
    //   should contruct list of fields from full list
    //   rather than copying this loop twice...
    if (field_list.size() > 0){
      for (size_t i = 0; i < field_list.size(); i++){
        std::string name = field_list[i];
        if (field.groups()->is_in(name,"derived")){
          Compute * compute = problem->create_compute(name,
                                                      config);
          compute->compute(this);
          delete compute; // must be done
        }
      }
    } else{ // else check full field list and compute all
      // derived fields
      for(int i = 0; i < field.field_count(); i++){
        std::string name = field.field_name(i);
        if (field.groups()->is_in(name,"derived")){
          // call the appropriate compute object
          Compute * compute = problem->create_compute(name,
                                                      config);
          compute->compute(this);
          delete compute; // must be done
        }
      }
    }
  } // end if derived

  return;
}

//======================================================================

void Block::apply_initial_(MsgRefine * msg) throw ()
{
#ifdef TRACE_BLOCK
  CkPrintf ("TRACE_BLOCK %s apply_initial()\n",name().c_str());
  fflush(stdout);
#endif
  if (! cello::is_initial_cycle(state_->cycle(),InitCycleKind::fresh)) {

    msg->update(data());

  } else {
    TRACE("Block::apply_initial_()");
    Simulation * simulation = cello::simulation();
    if (simulation->phase() == phase_initial) {
      // Create child blocks if this block refines during the initialization
      // phase.
      create_initial_child_blocks();

      // Tell the root Simulation object this block is inserted and ready 
      // to initialize data.
      proxy_simulation[0].p_initial_block_created();
    } else {
      initial_begin();
    }
  }
}

//----------------------------------------------------------------------

void Block::initial_begin()
{
  const bool initial_new = cello::config()->initial_new;

  if (initial_new) {
    initial_new_begin_();

  } else {

    // Apply initial conditions
    for (int k=0; k<cello::num_initial(); k++) {
      cello::initial(k)->enforce_block(this,cello::hierarchy());
    }
  }
}

//----------------------------------------------------------------------

Block::~Block()
{
  Simulation * simulation = cello::simulation();

  Monitor * monitor = simulation ? simulation->monitor() : NULL;

  const int level = this->level();

  if (level > 0) {

    // Send restricted data to parent

    int ic3[3];
    index_.child(level,ic3,ic3+1,ic3+2);

    int n;
    char * array;
    int if3[3]={0,0,0};
    int g3[3]={0,0,0};
    Refresh * refresh = new Refresh;
    refresh->add_all_data();
    refresh -> set_adaptive_timestep
      (state()->state_type() == State::Type::Level);

    FieldFace * field_face = new FieldFace
      ( level, -1, if3,ic3,g3,refresh, true);

    field_face->face_to_array(data()->field(),&n,&array);
    delete field_face;

    const Index index_parent = index_.index_parent();

    // --------------------------------------------------
    // ENTRY: #2 Block::~Block()-> Block::p_refresh_child()
    // ENTRY: parent if level > 0
    // --------------------------------------------------
    thisProxy[index_parent].p_refresh_child(n,array,ic3);
    // --------------------------------------------------

    delete [] array;
  }

  delete data_;
  data_ = 0;

  delete child_data_;
  child_data_ = 0;

  if (simulation) simulation->data_delete_block(this);

}

//----------------------------------------------------------------------

void Block::p_refresh_child
(
 int    n,
 char * buffer,
 int    ic3[3]
 )
{
  PERF_START(iperf_refresh_child);
  int if3[3] = {0,0,0};
  int  g3[3] = {0,0,0};
  Refresh * refresh = new Refresh;
  refresh->add_all_data();
  refresh -> set_adaptive_timestep
    (state()->state_type() == State::Type::Level);

  FieldFace * field_face = new FieldFace
    (level(), -1, if3, ic3, g3, refresh, true);
  // Adjust level for child block
  field_face->set_level (level()+1);

  field_face -> array_to_face (buffer, data()->field());
  delete field_face;
  PERF_STOP(iperf_refresh_child);
}

//----------------------------------------------------------------------

void Block::init_adapt_(Adapt * adapt_parent)
{
  const int level = index_.level();
  const int rank = cello::rank();

  int p3[3],b3[3];
  cello::hierarchy()->get_periodicity(p3,p3+1,p3+2);
  cello::hierarchy()->root_blocks(b3,b3+1,b3+2);

  adapt_.set_rank(rank);
  adapt_.set_min_level(cello::min_level());
  adapt_.set_max_level(cello::max_level());
  adapt_.set_index(index_);
  adapt_.set_periodicity(p3);
  adapt_.set_valid(true);

  if ( (level <= 0) && cello::is_initial_cycle(state_->cycle(),InitCycleKind::fresh) ) {
    // If root-level (or below) block in first simulation cycle,
    // initialize neighbors to be all adjacent root-level blocks
    int nb3[3],np3[3],ib3[3];
    cello::hierarchy()->root_blocks(nb3,nb3+1,nb3+2);
    cello::hierarchy()->get_periodicity(np3,np3+1,np3+2);
    index_.array(ib3,ib3+1,ib3+2);
    // Initialize face index loop limits
    int ifm3[3],ifp3[3];
    for (int i=0; i<3; i++) {
      if (i < rank) {
        if (np3[i]) {
          // If periodic then block always has neighbor
          ifm3[i] = -1;
          ifp3[i] = +1;
        } else {
          // If not periodic then no block neighbor at domain ends
          ifm3[i] = (ib3[i] - 1 >= 0)     ? -1 : 0;
          ifp3[i] = (ib3[i] + 1 < nb3[i]) ? +1 : 0;
        }
      } else {
        // limits 0 for unused dimensions
        ifm3[i] = ifp3[i] = 0;
      }
    }
    int if3[3];
    for (if3[2]=ifm3[2]; if3[2]<=ifp3[2]; ++if3[2]) {
      for (if3[1]=ifm3[1]; if3[1]<=ifp3[1]; ++if3[1]) {
        for (if3[0]=ifm3[0]; if3[0]<=ifp3[0]; ++if3[0]) {
          if (if3[0] || if3[1] || if3[2]) {
            Index index_neighbor = index_.index_neighbor(if3,nb3);
            adapt_.insert_neighbor(index_neighbor);
          }
        }
      }
    }
  
  } else if (level > 0) {
    // else if a refined Block, initialize adapt from its incoming
    // parent block
    int ic3[3];
    index_.child(level,ic3,ic3+1,ic3+2);
    adapt_.refine(*adapt_parent,ic3);
#ifdef DEBUG_ADAPT
    CkPrintf ("DEBUG_ADAPT %s Block() level > 0\n",
              name().c_str());
    adapt_parent->print("init_adapt parent",this);
    adapt_.print("init_adapt child after",this);
#endif    
  }

  // replace neighbor blocks with their child blocks if they refine
  // during the initialization phase.
  int max_initial_level = cello::config()->mesh_max_initial_level;
  for (int level_i=level; level_i < max_initial_level; level_i++) {
    std::vector<Index> neighbors = adapt_.index_neighbors();
    for (int i=0; i<(int) neighbors.size(); i++) {
      Index neighbor_index = neighbors.at(i);
      if (neighbor_index.level() == level_i) {
        if (refine_during_initialization(neighbor_index))
          adapt_.refine_neighbor(neighbor_index);
      }
    }
  }
}

//----------------------------------------------------------------------

void Block::init_refresh_()
{
  const int count = cello::simulation()->refresh_count();
  refresh_sync_list_.resize(count);
  refresh_recv_buffer_.resize(count);
  refresh_send_buffer_.resize(count);
  refresh_send_index_.resize(count);
  for (int i=0; i<count; i++) {
    refresh_sync_list_[i].reset();
  }
}

//----------------------------------------------------------------------

std::string Block::name(Index index) const throw()
{
  int blocking[3] = {1,1,1};
  cello::hierarchy()->root_blocks(blocking,blocking+1,blocking+2);

  const int level = index.level();
  for (int i=-1; i>=level; i--) {
    blocking[0] /= 2;
    blocking[1] /= 2;
    blocking[2] /= 2;
  }

  int bits[3] = {0,0,0};

  blocking[0]--;
  blocking[1]--;
  blocking[2]--;

  if (blocking[0]) do { ++bits[0]; } while (blocking[0]/=2);
  if (blocking[1]) do { ++bits[1]; } while (blocking[1]/=2);
  if (blocking[2]) do { ++bits[2]; } while (blocking[2]/=2);

  std::string name = std::string("B" + index.bit_string(level,cello::rank(),bits));
  return name;
}

//----------------------------------------------------------------------

std::string Block::name8(Index index) const throw()
{
  int a3[3];
  int t3[3];
  index.array(a3,a3+1,a3+2);
  index.tree(t3,t3+1,t3+2);

  const int min_level = cello::config()->mesh_min_level;

  std::string name8 = "[#";
  for (int level=std::min(index.level(),0)-min_level-1; level>=0; level--) {
    int shift = level;
    int ax = (a3[0] >> shift) & 1;
    int ay = (a3[1] >> shift) & 1;
    int az = (a3[2] >> shift) & 1;
    char digit = '0' + ax+2*(ay+2*az);
    name8 += digit;
  }
  name8 += ":";
  for (int level=1; level<=index.level(); level++) {
    int shift = (INDEX_BITS_TREE-level);
    int tx = (t3[0] >> shift) & 1;
    int ty = (t3[1] >> shift) & 1;
    int tz = (t3[2] >> shift) & 1;
    char digit = tx+2*(ty+2*tz);
    name8 += '0' + digit;
  }
  name8 += "]";
  return name8;
}

//----------------------------------------------------------------------

void Block::size_array (int * nx, int * ny, int * nz) const throw ()
{
  cello::hierarchy()->root_blocks(nx,ny,nz);
}

//----------------------------------------------------------------------

int Block::ip_home () const
{
  int ax,ay,az;
  cello::hierarchy()->root_blocks(&ax,&ay,&az);
  return thisIndex.ip_home(ax,ay,az);
}

//----------------------------------------------------------------------

void Block::lower
(double * xm, double * ym, double * zm) const throw ()
{
  int  ix, iy, iz;
  int  nx, ny, nz;

  index_global (&ix,&iy,&iz,&nx,&ny,&nz);

  Hierarchy * hierarchy = cello::hierarchy();
  double xdm, ydm, zdm;
  hierarchy->lower(&xdm,&ydm,&zdm);
  double xdp, ydp, zdp;
  hierarchy->upper(&xdp,&ydp,&zdp);

  double ax = 1.0*ix/nx;
  double ay = 1.0*iy/ny;
  double az = 1.0*iz/nz;

  double xbm = (1.0-ax)*xdm + ax*xdp;
  double ybm = (1.0-ay)*ydm + ay*ydp;
  double zbm = (1.0-az)*zdm + az*zdp;

  if (xm) (*xm) = xbm;
  if (ym) (*ym) = ybm;
  if (zm) (*zm) = zbm;
}

//----------------------------------------------------------------------

void Block::upper
(double * xp, double * yp, double * zp) const throw ()
{
  int  ix, iy, iz;
  int  nx, ny, nz;

  index_global (&ix,&iy,&iz,&nx,&ny,&nz);

  Hierarchy * hierarchy = cello::hierarchy();
  double xdm, ydm, zdm;
  hierarchy->lower(&xdm,&ydm,&zdm);
  double xdp, ydp, zdp;
  hierarchy->upper(&xdp,&ydp,&zdp);

  double ax = 1.0*(ix+1)/nx;
  double ay = 1.0*(iy+1)/ny;
  double az = 1.0*(iz+1)/nz;

  double xbp = (1.0-ax)*xdm + ax*xdp;
  double ybp = (1.0-ay)*ydm + ay*ydp;
  double zbp = (1.0-az)*zdm + az*zdp;

  if (xp) (*xp) = xbp;
  if (yp) (*yp) = ybp;
  if (zp) (*zp) = zbp;
}

//----------------------------------------------------------------------

void Block::cell_width
(double * dx, double * dy, double * dz) const throw()
{
  double xm,ym,zm;
  lower(&xm,&ym,&zm);
  double xp,yp,zp;
  upper(&xp,&yp,&zp);
  data()->field_data()->cell_width(xm,xp,dx, ym,yp,dy, zm,zp,dz);
}

//----------------------------------------------------------------------

void Block::index_global
( Index index,
  int *ix, int *iy, int *iz,
  int *nx, int *ny, int *nz ) const
{
  const int level = index.level();
  index.array(ix,iy,iz);
  size_array (nx,ny,nz);

  if (level < 0 ) {
    for (int i=level; i<0; i++) {
      if (ix) (*ix) = ((*ix) >> 1);
      if (iy) (*iy) = ((*iy) >> 1);
      if (iz) (*iz) = ((*iz) >> 1);
      if (nx) (*nx) >>= 1;
      if (ny) (*ny) >>= 1;
      if (nz) (*nz) >>= 1;
    }
  }  else if (0 < level) {
    for (int i=0; i<level; i++) {
      int bx,by,bz;
      index.child(i+1,&bx,&by,&bz);
      if (ix) (*ix) = ((*ix) << 1) | bx;
      if (iy) (*iy) = ((*iy) << 1) | by;
      if (iz) (*iz) = ((*iz) << 1) | bz;
      if (nx) (*nx) <<= 1;
      if (ny) (*ny) <<= 1;
      if (nz) (*nz) <<= 1;
    }
  }
}

//----------------------------------------------------------------------

Index Block::index_from_global(int ix, int iy, int iz, int level, int min_level)
{
  Index index;
  index.set_array(ix >> level, iy >> level, iz >> level);
  index.set_level(level);
  for (int i = 0; i < level - min_level; i++) {
    int l = level - i;
    index.set_child(l, (ix >> i) & 1, (iy >> i) & 1, (iz >> i) & 1, min_level);
  }

  return index;
}

//----------------------------------------------------------------------

void Block::is_on_boundary (bool is_boundary[3][2]) const throw()
{

  // Boundary * boundary = simulation()->problem()->boundary();
  // bool periodic = boundary->is_periodic();

  int n3[3];
  size_array (&n3[0],&n3[1],&n3[2]);

  const int level = this->level();
  // adjust array size for negative levels
  if (level < 0) {
    int shift = -level;
    n3[0] = n3[0] >> shift;
    n3[1] = n3[1] >> shift;
    n3[2] = n3[2] >> shift;
  }

  for (int axis=0; axis<3; axis++) {
    for (int face=0; face<2; face++) {
      is_boundary[axis][face] =
	index_.is_on_boundary(axis,2*face-1,n3[axis]);
    }
  }
}

//----------------------------------------------------------------------

void Block::set_child_face_level_curr
(const int ic3[3], const int if3[3], int level, int count)
{
  int i = ICF3(ic3,if3);
  if (count < 0) {
    child_face_level_curr_count_[i] = count;
  }
  if (count >= child_face_level_curr_count_[i]) {
    child_face_level_curr_[i]       = level;
    child_face_level_curr_count_[i] = count;
  }
}

//----------------------------------------------------------------------

void Block::set_child_face_level_next
(const int ic3[3], const int if3[3], int level, int count)
{
  int i = ICF3(ic3,if3);
  if (count < 0) {
    child_face_level_next_count_[i] = count;
  }
  if (count >= child_face_level_next_count_[i]) {
    child_face_level_next_[i]       = level;
    child_face_level_next_count_[i] = count;
  }
}

//----------------------------------------------------------------------

void Block::verify_neighbors()
{
  if (! is_leaf()) return;

  ItNeighbor it_neighbor = this->it_neighbor(index_);

  int num_neighbors = 0;
  int tf3[3];
  while (it_neighbor.next(tf3)) {
     ++num_neighbors;
  }
  ASSERT2 ("Block::verify_neighbors()",
           "Neighbor count mismatch between Adapt %d and face_level_ %d",
           adapt_.num_neighbors(), num_neighbors,
           (adapt_.num_neighbors() == num_neighbors));

  while (it_neighbor.next(tf3)) {
     Index index_neighbor = it_neighbor.index();
     char buffer[256];
     snprintf (buffer,80,"%s Neighbor mismatch between Adapt and face_level_",
               name().c_str());
     ASSERT ("Block::verify_neighbors()",
             buffer,
             adapt_.is_neighbor(index_neighbor));
     ++num_neighbors;
  }
  ASSERT2 ("Block::verify_neighbors()",
           "Neighbor count mismatch between Adapt %d and face_level_ %d",
           adapt_.num_neighbors(), num_neighbors,
           (adapt_.num_neighbors() == num_neighbors));
}

//======================================================================

void Block::determine_boundary_
(
 bool bndry[3][2],
 bool * fxm, bool * fxp,
 bool * fym, bool * fyp,
 bool * fzm, bool * fzp
 )
{
  // return bndry[] array of faces on domain boundary

  is_on_boundary (bndry);

  int nx,ny,nz;
  data()->field_data()->size (&nx,&ny,&nz);

  // Determine in which directions we need to communicate or update boundary

  if (fxm && bndry[0][0]) *fxm = (nx > 1);
  if (fxp && bndry[0][1]) *fxp = (nx > 1);
  if (fym && bndry[1][0]) *fym = (ny > 1);
  if (fyp && bndry[1][1]) *fyp = (ny > 1);
  if (fzm && bndry[2][0]) *fzm = (nz > 1);
  if (fzp && bndry[2][1]) *fzp = (nz > 1);
}

//----------------------------------------------------------------------

void Block::update_boundary_ ()
{
  bool is_boundary[3][2];
  bool fxm=0,fxp=0,fym=0,fyp=0,fzm=0,fzp=0;

  determine_boundary_(is_boundary,&fxm,&fxp,&fym,&fyp,&fzm,&fzp);

  for (int k=0; k<cello::num_boundary(); k++) {
    Boundary * boundary = cello::boundary(k);
    // Update boundaries
    if ( fxm ) boundary->enforce(this,face_lower,axis_x);
    if ( fxp ) boundary->enforce(this,face_upper,axis_x);
    if ( fym ) boundary->enforce(this,face_lower,axis_y);
    if ( fyp ) boundary->enforce(this,face_upper,axis_y);
    if ( fzm ) boundary->enforce(this,face_lower,axis_z);
    if ( fzp ) boundary->enforce(this,face_upper,axis_z);
  }
}

//----------------------------------------------------------------------

void Block::facing_child_(int jc3[3], const int ic3[3], const int if3[3]) const
{
  jc3[0] = if3[0] ? 1 - ic3[0] : ic3[0];
  jc3[1] = if3[1] ? 1 - ic3[1] : ic3[1];
  jc3[2] = if3[2] ? 1 - ic3[2] : ic3[2];
  TRACE9("facing_child %d %d %d  child %d %d %d  face %d %d %d",
	 jc3[0],jc3[1],jc3[2],ic3[0],ic3[1],ic3[2],if3[0],if3[1],if3[2]);
}

//----------------------------------------------------------------------

Index Block::neighbor_
(
 const int of3[3],
 Index *   ind
 ) const
{
  Index index = (ind != 0) ? (*ind) : index_;

  int na3[3];
  size_array (&na3[0],&na3[1],&na3[2]);
  // const bool periodic  = simulation()->problem()->boundary()->is_periodic();
  Index in = index.index_neighbor (of3,na3);
  return in;
}

//----------------------------------------------------------------------

void Block::check_leaf_()
{
  if (level() >= 0 &&
      ((  is_leaf() && children_.size() != 0) ||
       (! is_leaf() && children_.size() == 0))) {

    WARNING3("Block::check_leaf_()",
	     "%s: is_leaf() == %s && children_.size() == %lu",
	     name_.c_str(), is_leaf()?"true":"false",
	     children_.size());
  }
}

//----------------------------------------------------------------------

bool Block::check_position_in_block
(const double &x, const double &y, const double &z,
 bool include_ghost // default - false
 )
{

  double xm,ym,zm;
  lower(&xm,&ym,&zm);
  double xp,yp,zp;
  upper(&xp,&yp,&zp);

  bool result = false;

  if (include_ghost){
    int gx, gy, gz;
    double hx,hy,hz;
    data()->field().ghost_depth(0,&gx,&gy,&gz);
    cell_width(&hx,&hy,&hz);

    xm -= gx*hx;
    ym -= gy*hy;
    zm -= gz*hz;
    xp += gx*hx;
    yp += gy*hy;
    zp += gz*hz;
  }

  if (  ((x >= xm) && (x < xp)) &&
        ((y >= ym) && (y < yp)) &&
        ((z >= zm) && (z < zp))) result = true;

  return result;
}

//----------------------------------------------------------------------

bool Block::refine_during_initialization(Index index) const throw()
{
  int level = index.level();
  if (level >= 0) {
    
    if (level + 1 <= (int) cello::config()->refined_regions_lower.size()) {

      std::vector<int> lower = cello::config()->refined_regions_lower.at(level);
      std::vector<int> upper = cello::config()->refined_regions_upper.at(level);

      int ix, iy, iz, nx, ny, nz;
      index_global(index, &ix, &iy, &iz, &nx, &ny, &nz);

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
