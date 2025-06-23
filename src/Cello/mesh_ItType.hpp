// See LICENSE_CELLO file for license and copyright information

/// @file     mesh_ItType.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2025-06-23u
/// @brief    [\ref Mesh] Declaration of the ItType ABC
///

#ifndef MESH_IT_TYPE_HPP
#define MESH_IT_TYPE_HPP

class ItType : public PUP::able{

  /// @class    ItType
  /// @ingroup  Mesh
  /// @brief    [\ref Mesh] 

public: // interface

  /// Constructor
  ItType () { }

  /// Charm++ PUP::able declarations
  PUPable_abstract(ItType);

  ItType (CkMigrateMessage *m)
    : PUP::able(m)
  { }

  void pup (PUP::er &p)
  { TRACEPUP;
    PUP::able::pup(p);
  }

  /// Get next face;  return false when done
  virtual bool next (int of3[3]) = 0;

  /// Return the level of the current face
  virtual int face_level () const  throw () = 0;

  /// Return index of child adjacent to fine neighbor
  virtual void child(int ic3[3]) const = 0;

  /// Return the index of the current face
  virtual Index index() const = 0;

  /// Reset the Iterator to the beginning
  virtual void reset() = 0;

  virtual bool is_reset() const = 0;

protected: // attributes

};

#endif /* MESH_ITTYPE_HPP */

