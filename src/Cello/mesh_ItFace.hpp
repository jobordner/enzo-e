// See LICENSE_CELLO file for license and copyright information

/// @file     mesh_ItFace.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2013-06-11
/// @brief    [\ref Mesh] Declaration of the ItFace class
///

#ifndef MESH_IT_FACE_HPP
#define MESH_IT_FACE_HPP

class ItFace : public ItType {

  /// @class    ItFace
  /// @ingroup  Mesh
  /// @brief    [\ref Mesh] 

public: // interface

  /// Constructor
  ItFace(int rank,
	 int min_face_rank,
	 int periodic[3],
	 int n3[3],
	 Index index,
	 const int * ic3=0,
	 const int * if3=0);

  /// Charm++ PUP::able declarations
  PUPable_decl(ItFace);

  ItFace (CkMigrateMessage *m)
    : ItType (m)
  { }

  inline void pup (PUP::er &p)
  {
    // NOTE: change this function whenever attributes change
    TRACEPUP;
    ItType::pup(p);
    PUParray(p,if3_,3);
    p | ic3_;
    p | ipf3_;
    p | rank_;
    p | min_face_rank_;
    PUParray (p,periodicity_,3);
    PUParray (p,n3_,3);
    p | index_;
  }

  /// Go to the next face if any and return it through of3[]
  bool next (int of3[3]) override
  {
    const bool retval = next_();
    if (retval) face (of3);
    return retval;
  }

  /// Return the current face through of3[]
  void face (int of3[3]) const ;

  /// Return the level of the current face
  int face_level () const override
  { return index_.level(); }

  /// Return the type of face: positive if finer, negative if coarser, 0 if same
  virtual int face_type() const override
  {
    return 0;;
  }

  virtual void child(int ic3[3]) const override
  { }

  Index index() const  override;

  /// Reset the Iterator to the beginning
  void reset() override;

  bool is_reset() const override;

private: // functions

  /// Go to the next face if any
  bool next_ ();

  /// go to the next face
  void increment_();


  /// go to the first face
  void set_first_();

  /// Whether the current face rank is valid
  bool valid_() const;

private: // attributes

  // NOTE: change pup() function whenever attributes change

  /// Current face
  int if3_[3];

  /// Adjacency child
  std::vector<int> ic3_;

  /// Parent face
  std::vector<int> ipf3_;

  /// simulation rank
  int rank_;

  /// face rank limit
  int min_face_rank_;

  /// Periodicity
  int periodicity_[3];

  /// Size of the octree array
  int n3_[3];

  /// Index
  Index index_;

};

#endif /* MESH_ITFACE_HPP */

