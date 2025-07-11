// See LICENSE_CELLO file for license and copyright information

/// @file     mesh_ItNeighbor.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2013-06-11
/// @brief    [\ref Mesh] Declaration of the ItNeighbor class
///

#ifndef MESH_IT_NEIGHBOR_HPP
#define MESH_IT_NEIGHBOR_HPP

class ItNeighbor : public ItType {

  /// @class    ItNeighbor
  /// @ingroup  Mesh
  /// @brief    [\ref Mesh] 

public: // interface

  /// Constructor
  ItNeighbor
  (
   Block * block,
   int min_face_rank,
   int periodic[3],
   int n3[3],
   Index index,
   int neighbor_type,
   int root_level,
   int level_lower = 0,
   int level_upper = std::numeric_limits<int>::max(),
   DirType dir_type = DirType::Both,
   ScheduleType schedule_type = ScheduleType::Casual);

  ItNeighbor (ItNeighbor &&) = default;
  ItNeighbor & operator = (ItNeighbor &&) = default;

  /// Charm++ PUP::able declarations
  PUPable_decl(ItNeighbor);

  ItNeighbor (CkMigrateMessage *m)
    : ItType (m)
  { }

  /// CHARM++ Pack / Unpack function
  inline void pup (PUP::er &p)
  {
    // NOTE: change this function whenever attributes change
    TRACEPUP;
    ItType::pup(p);
    // const bool up = p.isUnpacking();
    // if (up) block_ = new Block;
    // p | *block_;
    // PUParray(p,of3_,3);
    // PUParray(p,ic3_,3);
    // PUParray(p,ipf3_,3);
    // p | rank_;
    // p | min_face_rank_;
    // PUParray (p,periodic_,3);
    // PUParray (p,n3_,3);
    // p | index_;
    // p | neighbor_type_;
    // p | root_level_;
    // p | level_lower_;
    // p | level_upper_;
    // p | dir_type_;
    // p | schedule_type_;
  }


  // public interface
public:

  /// Reduce another value
  bool next (int of3[3]) override
  {
    const bool retval = next_();
    if (retval) face_(of3);
    return retval;
  }

  /// Return the level of this blocke current face
  int this_level () const
  { return index_.level(); }

  /// Return the level of the current face
  int face_level () const override
  { return block_->face_level(of3_); }

  int face_type() const override
  {
    return face_level() - this_level();
  }

  void child(int ic3[3]) const  override;

  Index index() const  override;

  /// Reset the Iterator to the beginning
  void reset() override;

  bool is_reset() const override;

private: // functions

  void face_(int of3[3]) const ;

  /// go to the next face / child
  bool next_();

  /// go to the next face / child
  void increment_();

  /// go to the next child
  void next_child_();

  /// go to the first face
  void set_first_();

  /// go to the first child
  void set_first_child_();

  /// reset child loop
  void reset_child_();

  /// whether child loop is reset
  bool is_reset_child_() const ;

  /// Whether the current face rank is valid
  bool valid_();

private: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Block that this iterator is associated with
  Block * block_;

  /// Current face
  int of3_[3];

  /// Adjacency child
  int ic3_[3];

  /// Parent face
  int ipf3_[3];

  /// simulation rank
  int rank_;

  /// face rank limit
  int min_face_rank_;

  /// Whether domain is periodic along each face & axis
  int periodic_[3];

  /// Size of the block array
  int n3_[3];

  /// Index
  Index index_;

  /// Neighbor type (neighbor_leaf or neighbor_tree)
  int neighbor_type_;

  /// Level of coarse grid when neighbor_type_ == neighbor_leaf
  int root_level_;

  /// Level range for adaptive time-stepping
  int level_lower_;
  int level_upper_;

  /// Direction for adaptive time-stepping
  DirType dir_type_;

  /// Schedule type for adaptive time-stepping ("eager" or "casual")
  ScheduleType schedule_type_;

};

#endif /* MESH_ITNEIGHBOR_HPP */

