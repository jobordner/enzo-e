// See LICENSE_CELLO file for license and copyright information

/// @file     problem_Refresh.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @author   Stefan Arridge (stefan.arridge@gmail.com)
/// @date     2014-11-04 22:24:46
/// @brief    [\ref Problem] Declaration of the Refresh class
///

#ifndef PROBLEM_REFRESH_HPP
#define PROBLEM_REFRESH_HPP

class Box;
class Prolong;
class Restrict;
class ItNeighbor;

class Refresh : public PUP::able {

  /// @class    Refresh
  /// @ingroup  Problem
  /// @brief    [\ref Problem]

  // private:
  /// empty constructor for charm++ pup()
  // Refresh() throw()
  // : all_fields_(false),
  //   field_list_src_(),
  //   field_list_dst_(),
  //   all_particles_(false),
  //   particles_are_copied_(false),
  //   particle_list_(),
  //   all_fluxes_(false),
  //   ghost_depth_(0),
  //   min_face_rank_(0),
  //   neighbor_type_(neighbor_leaf),
  //   accumulate_(false),
  //   sync_type_   (sync_unknown),
  //   sync_id_ (-1),
  //   active_(true),
  //   callback_(0) ,
  //   level_(0),
  //   root_level_(0),
  //   level_lower_(0),
  //   level_upper_(std::numeric_limits<int>::max()),
  //   id_refresh_(-1),
  //   id_prolong_(0),
  //   id_restrict_(0)
  // {
  // }

  /// Create an initialized Refresh object
  Refresh
  (int ghost_depth,
   int min_face_rank,
   int neighbor_type,
   int sync_type,
   int sync_id,
   bool active=true) throw()
    : all_fields_(false),
      field_list_src_(),
      field_list_dst_(),
      all_particles_(false),
      particles_are_copied_(false),
      particle_list_(),
      all_fluxes_(false),
      ghost_depth_(ghost_depth),
      min_face_rank_(min_face_rank),
      neighbor_type_(neighbor_type),
      accumulate_(false),
      sync_type_(sync_type),
      sync_id_(sync_id),
      active_(active),
      callback_(0),
      level_(0),
      root_level_(0),
      adaptive_timestep_(false),
      level_lower_(0),
      level_upper_(std::numeric_limits<int>::max()),
      global_(false),
      advanced_time_(false),
      id_refresh_(-1),
      id_prolong_(0),
      id_restrict_(0),
      final_sync_(false)
  {
  }

public: // interface

  static Refresh * create
  (int ghost_depth,
   int min_face_rank,
   int neighbor_type,
   int sync_type,
   int sync_id,
   bool active=true)
  {
    return new Refresh
      (ghost_depth,
       min_face_rank,
       neighbor_type,
       sync_type,
       sync_id,
       active);
  }

  /// empty constructor for charm++ pup()
  Refresh() throw()
    : all_fields_(false),
      field_list_src_(),
      field_list_dst_(),
      all_particles_(false),
      particles_are_copied_(false),
      particle_list_(),
      all_fluxes_(false),
      ghost_depth_(0),
      min_face_rank_(0),
      neighbor_type_(neighbor_leaf),
      accumulate_(false),
      sync_type_   (sync_unknown),
      sync_id_ (-1),
      active_(true),
      callback_(0) ,
      level_(0),
      root_level_(0),
      adaptive_timestep_(false),
      level_lower_(0),
      level_upper_(std::numeric_limits<int>::max()),
      global_(false),
      advanced_time_(false),
      id_refresh_(-1),
      id_prolong_(0),
      id_restrict_(0),
      final_sync_(false)
  {
  }

  /// CHARM++ PUP::able declaration
  PUPable_decl(Refresh);

  /// CHARM++ migration constructor for PUP::able
  Refresh (CkMigrateMessage *m)
    : PUP::able(m),
      all_fields_(false),
      field_list_src_(),
      field_list_dst_(),
      all_particles_(false),
      particles_are_copied_(false),
      particle_list_(),
      all_fluxes_(false),
      ghost_depth_(0),
      min_face_rank_(0),
      neighbor_type_(0),
      accumulate_(false),
      sync_type_(0),
      sync_id_ (-1),
      active_(true),
      callback_(0),
      level_(0),
      root_level_(0),
      adaptive_timestep_(false),
      level_lower_(0),
      level_upper_(std::numeric_limits<int>::max()),
      global_(false),
      advanced_time_(false),
      id_refresh_(-1),
      id_prolong_(0),
      id_restrict_(0),
      final_sync_(false)
  {
  }

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {

    PUP::able::pup(p);

    p | all_fields_;
    p | field_list_src_;
    p | field_list_dst_;
    p | all_particles_;
    p | particles_are_copied_;
    p | particle_list_;
    p | all_fluxes_;
    p | ghost_depth_;
    p | min_face_rank_;
    p | neighbor_type_;
    p | accumulate_;
    p | sync_type_;
    p | sync_id_;
    p | active_;
    p | callback_;
    p | level_;
    p | root_level_;
    p | adaptive_timestep_;
    p | level_lower_;
    p | level_upper_;
    p | global_;
    p | advanced_time_;
    p | id_refresh_;
    p | id_prolong_;
    p | id_restrict_;
    p | final_sync_;
  }

  //--------------------------------------------------
  // FIELD METHODS
  //--------------------------------------------------

  /// Add a field id to the list of fields to refresh; don't add if it's already
  /// in the list
  void add_field(int id_field) {
    all_fields_ = false;
    if (id_field >= 0) {
      if (std::find (field_list_src_.begin(), field_list_src_.end(), id_field)
	  == field_list_src_.end()) {
	field_list_src_.push_back(id_field);
      }
      if (std::find (field_list_dst_.begin(), field_list_dst_.end(), id_field)
	  == field_list_dst_.end()) {
	field_list_dst_.push_back(id_field);
      }
    }
  }

  /// Add a named field to the list of fields to refresh
  void add_field(std::string field_name);

  /// Add a source and corresponding destination field to refresh;
  /// does not check if fields are already in the lists
  void add_field_src_dst(int id_field_src, int id_field_dst) {
    all_fields_ = false;
    if (id_field_src >= 0 && id_field_dst >= 0) {
      field_list_src_.push_back(id_field_src);
      field_list_dst_.push_back(id_field_dst);
    }
  }

  /// Add named fields to the list of fields to refresh
  void add_field_src_dst(std::string field_src, std::string field_dst);

  /// All fields are refreshed
  void add_all_fields(std::string field_group = "");

  /// Add specified fields
  void set_field_list (std::vector<int> field_list)
  {
    field_list_src_ = field_list;
    field_list_dst_ = field_list;
  }

  /// Return whether all fields are refreshed
  bool all_fields() const
  { return all_fields_; }

  /// Return whether any fields are refreshed
  bool any_fields() const
  { return (all_fields_ || (field_list_src().size() > 0)); }

  std::vector<int> field_list_src(int level = 0,
                                  int face_type = 0) const;
  /// Return the list of destination fields participating in the
  /// Refresh operation
  std::vector<int> field_list_dst(int level = 0,
                                  int face_type = 0) const;

  //--------------------------------------------------
  // PARTICLE METHODS
  //--------------------------------------------------

  /// Add specified fields
  void set_particle_list (std::vector<int> particle_list)
  {
    particle_list_ = particle_list;
  }

  /// Add the given particle type to the list
  void add_particle(int id_particle) {
    all_particles_ = false;
    particle_list_.push_back(id_particle);
  }

  void add_particle(std::string particle_name);

  /// All particles types are refreshed
  void add_all_particles() {
    all_particles_ = true;
  }

  /// Set the value for `particles_are_copied_` to be true
  /// or false, which determines whether or not, for all
  /// particle types participating in the refresh, all particles
  /// are copied to all neighbouring blocks.
  void set_particles_are_copied(bool particles_are_copied) {
    particles_are_copied_ = particles_are_copied;
  }

  /// Return whether all particles are refreshed
  bool all_particles() const
  { return all_particles_; }

  /// Return whether, for all particle types participating
  /// in the refresh, all particles are copied to all
  /// neighbouring blocks
  bool particles_are_copied() const
  { return particles_are_copied_; }

  /// Return whether any particles are refreshed
  bool any_particles() const
  { return (all_particles_ || (particle_list_.size() > 0)); }

  /// Return the list of particles participating in the Refresh operation
  const std::vector<int> & particle_list() const {
    return particle_list_;
  }

  /// Add all data
  /// Add flux data
  void add_all_fluxes()
  { all_fluxes_ = true; }

  /// Return whether any (all) fluxes
  bool any_fluxes() const
  { return all_fluxes_; }

    /// Add all data
  void add_all_data()
  {
    add_all_fields();
    add_all_particles();
    add_all_fluxes();
  }

  /// Return whether there are any data to be refreshed
  bool any_data() const
  { return (any_fields() || any_particles() || any_fluxes()); }

  /// Whether this particular Block is participating in the Refresh operation
  void set_active (bool active)
  { active_ = active; }

  bool is_active () const
  { return active_; }

  /// Callback function after refresh is completed
  int callback() const { return callback_; };

  /// Set the callback function for after the refresh operation is completed
  void set_callback(int callback)
  { callback_ = callback; }

  /// Level for neighbor_tree neighbor type
  int level() const { return level_; };

  /// Set the level for neighbor_tree neighbor type
  void set_level(int level)
  { level_ = level; }

  /// Coarse level for neighbor_tree neighbor type
  int root_level() const { return root_level_; };

  /// Set the coarse level for  neighbor_tree neighbor type
  void set_root_level(int root_level)
  { root_level_ = root_level; }

  /// Set whether using adptive timestepping
  void set_adaptive_timestep (bool adaptive_timestep)
  { adaptive_timestep_ = adaptive_timestep; }
  int adaptive_timestep () const
  { return adaptive_timestep_; }

  void set_global(bool global = true)
  {  global_ = global; }
  bool global() const { return global_; }

  void set_advanced_time (bool advanced_time = true)
  { advanced_time_ = advanced_time; }
  bool advanced_time () const
  { return advanced_time_; }

  /// Set the lower and puper limits (plus one) on levels being refreshed
  void set_level_lower(int level_lower)
  { level_lower_ = level_lower; }
  void set_level_upper(int level_upper)
  { level_upper_ = level_upper; }
  int level_lower() const
  //  { return global_ ? 0 : level_lower_; }
  { return level_lower_; }
  int level_upper() const
  //  { return global_ ? 4 : level_upper_; }
  { return level_upper_; }

  bool level_active (int level) const
  {
    //    return (global_ || (level_lower_ <= level && level < level_upper_));
    return (level_lower_ <= level) && (level < level_upper_);
  }

  /// Return the current minimum rank (dimension) of faces to refresh
  /// e.g. 0: everything, 1: omit corners, 2: omit corners and edges
  int min_face_rank() const
  {
    return (accumulate_ || (coarse_padding() > 0) || any_particles()) ?
      0 : min_face_rank_; }

  /// Set the ghost depth
  void set_ghost_depth(int ghost_depth)
  { ghost_depth_ = ghost_depth; }
  
  /// Set a new minimum rank (dimension) of faces to refresh
  /// e.g. 0: everything, 1: omit corners, 2: omit corners and edges
  void set_min_face_rank(int min_face_rank)
  { min_face_rank_ = min_face_rank; }

  /// Return the data field ghost depth
  int ghost_depth() const
  { return ghost_depth_; }

  /// Return the type of neighbors to refresh with: neighbor_leaf for
  /// neighboring leaf node (may be different mesh level) or
  /// neighbor_level for neighboring block in the same level (may be
  /// non-leaf)
  int neighbor_type() const
  { return neighbor_type_; }

  /// Return whether to add neighbor face values to ghost zones or to
  /// copy them.  NOTE only accumulates if source field is different
  /// from destination field
  bool accumulate(int i_f) const
  {
    return accumulate_ && (field_list_src()[i_f] != field_list_dst()[i_f]);
  }

  //  bool accumulate() const
  //  { return accumulate_; }
  
  /// Set whether to add neighbor face values to ghost zones instead of
  /// copying them.
  void set_accumulate(bool accumulate)
  {
    accumulate_ = accumulate;
  }

  // Boxes
  void box_accumulate_adjust (Box * box, int if3[3], int g3[3]);

  //----------------
  // Synchronization
  //----------------

  int sync_type() const
  { return sync_type_; }

  // Return the id of the synchronization object (used for debugging
  // only)
  int sync_id() const
  { return sync_id_; }

  int sync_exit() const
  { return 3*sync_id_+2; }

  /// Return the sync object associated with this refresh object
  Sync * sync( Block * block );

  ItNeighbor it_neighbor (Block * block, DirType dir_type = DirType::Both);

  //----------------------------------------------------------------------

  void print(FILE * fp = nullptr) const
  {
    if (!fp) fp = stdout;
    CkPrintf ("Refresh %p\n",(void*)this);
    CkPrintf ("     all_fields = %d\n",all_fields_);
    CkPrintf ("     src fields:");
    for (size_t i=0; i<field_list_src().size(); i++)
      CkPrintf (" %d",field_list_src()[i]);
    CkPrintf ("\n");
    CkPrintf ("     dst fields:");
    for (size_t i=0; i<field_list_dst().size(); i++)
      CkPrintf (" %d",field_list_dst()[i]);
    CkPrintf ("\n");
    CkPrintf ("     all_particles = %d\n",all_particles_);
    CkPrintf ("     particles:");
    for (size_t i=0; i<particle_list_.size(); i++)
      CkPrintf (" %d",particle_list_[i]);
    CkPrintf ("\n");
    CkPrintf ("     all_fluxes = %d\n",all_fluxes_);
    CkPrintf ("\n");
    CkPrintf ("     ghost_depth = %d\n",ghost_depth_);
    CkPrintf ("     min_face_rank: %d\n",min_face_rank_);
    CkPrintf ("     neighbor_type: %d\n",neighbor_type_);
    CkPrintf ("     accumulate: %d\n",accumulate_);
    CkPrintf ("     sync_type: %d\n",sync_type_);
    CkPrintf ("     sync_id: %d\n",sync_id_);
    CkPrintf ("     id_refresh: %d\n",id_refresh_);
    CkPrintf ("     active: %d\n",active_);
    CkPrintf ("     callback: %d\n",callback_);
    CkPrintf ("     level: %d\n",level_);
    CkPrintf ("     root_level: %d\n",root_level_);
    CkPrintf ("     adaptive_timestep: %d\n",adaptive_timestep_?1:0);
    CkPrintf ("     level_lower: %d\n",level_lower_);
    CkPrintf ("     level_upper: %d\n",level_upper_);
    CkPrintf ("     global: %d\n",global_);
    CkPrintf ("     advanced_time: %d\n",advanced_time_);
  }

  void summary() const;

  /// Return loop limits 0:3 for 4x4x4 particle data array indices
  /// for the given neighbor
  void get_particle_bin_limits
  (int rank,
   int face_type,
   int if3[3], int ic3[3],
   int lower[3], int upper[3])
  {
    for (int axis=0; axis<rank; axis++) {
      if (if3[axis] == -1) {
	lower[axis] = 0;
	upper[axis] = 1;
      } else if (if3[axis] == +1) {
	lower[axis] = 3;
	upper[axis] = 4;
      } else {
	if (face_type == 0) {
	  lower[axis] = 1;
	  upper[axis] = 3;
	} else if (face_type > 0) {
	  lower[axis] = ic3[axis] + 1;
	  upper[axis] = ic3[axis] + 2;
	} else if (face_type < 0) {
	  lower[axis] = 1 - ic3[axis];
	  upper[axis] = 4 - ic3[axis];
	}
      }
    }
  }

  /// Set the new refresh id in refresh_list_[]
  void set_id(int id_refresh)
  { id_refresh_ = id_refresh; }

  /// return the new refresh id in refresh_list_[]
  int id() const
  { return id_refresh_; }

  /// Return whether the prolongation requires padded coarse array
  int coarse_padding(const Prolong * prolong = nullptr) const;

  /// Set the prolongation operator for refresh
  void set_prolong (int id_prolong)
  { id_prolong_ = id_prolong; }

  /// Return the prolongation operator for refresh
  const Prolong * get_prolong () const;
  /// Return the prolongation id
  int index_prolong () const
  { return id_prolong_; }

  /// Set the restriction operator for refresh
  void set_restrict (int id_restrict)
  { id_restrict_ = id_restrict; }

  /// Return the restriction operator for refresh
  /// Whether to include history fields for this face
  bool include_history(int face_type) const;

  /// Whether to bypass final sync
  int final_sync() { return final_sync_; }

  /// Set whether to bypass final sync
  void set_final_sync (int sync = true)
  { final_sync_ = sync; }

  const Restrict * get_restrict () const;

  //--------------------------------------------------

  /// Return the number of bytes required to serialize the data object
  int data_size () const;

  /// Serialize the object into the provided empty memory buffer.
  /// Returns the next open position in the buffer to simplify
  /// serializing multiple objects in one buffer.
  char * save_data (char * buffer) const;

  /// Restore the object from the provided initialized memory buffer data.
  /// Returns the next open position in the buffer to simplify
  /// serializing multiple objects in one buffer.
  char * load_data (char * buffer);

  //--------------------------------------------------

private: // methods

  void include_history_fields_(std::vector<int> & field_list, int face_type) const;

private: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Whether to refresh all fields, ignoring field_list_*
  int all_fields_;

  /// Indicies of source fields; assumes all_fields_ == false, and
  /// size must be equal to field_list_dst_;
  std::vector <int> field_list_src_;

  /// Indicies of corresponding destination fields; assumes
  /// all_fields_ == false, and size must be equal to field_list_src_;
  std::vector <int> field_list_dst_;

  /// Whether to refresh all particle types, ignoring particle_list_
  int all_particles_;

  /// Whether or not, for all particle types participating in the refresh,
  /// all particles are copied to all neighbouring blocks
  bool particles_are_copied_;
  
  /// Indicies of particles to include
  std::vector <int> particle_list_;

  /// Whether to refresh flux data
  int all_fluxes_;

  /// Ghost zone depth
  int ghost_depth_;

  /// minimum face rank to refresh (2 include facets, 1 also edges, 0
  /// also corners)
  int min_face_rank_;

  /// Which subset of adjacent Blocks to refresh with
  int neighbor_type_;

  /// Whether to copy or add values
  int accumulate_;

  /// Synchronization type
  int sync_type_;

  /// Index for refresh synchronization counter
  int sync_id_;

  /// Whether the Refresh object is active for the block (replaces is_leaf())
  int active_;

  /// Callback after the refresh operation
  int callback_;

  /// This level for level refresh
  int level_;

  /// Coarse level for neighbor_tree type
  int root_level_;

  /// Whether adaptive timestepping is used
  bool adaptive_timestep_;

  /// Level range for adaptive time-stepping
  int level_lower_;
  int level_upper_;

  /// Whether to ignore level range and refresh entire hierarchy
  int global_;

  /// Whether to use t or t+dt when time-interpolating for
  /// adaptive time-stepping. Used for refresh at end of
  /// method list when active levels have advanced but current
  /// time hasn't been updated yet
  bool advanced_time_;

  /// ID in refresh_list_[]
  int id_refresh_;

  /// ids of interpolation and restriction operators
  int id_prolong_;
  int id_restrict_;

  /// Whether to perform a final synchronization (using sync_exit())
  int final_sync_;

};

#endif /* PROBLEM_REFRESH_HPP */
