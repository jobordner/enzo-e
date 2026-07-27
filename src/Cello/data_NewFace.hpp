// See LICENSE_CELLO file for license and copyright information

/// @file     data_NewFace.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2026-07-25
/// @brief    [\ref Data] Declaration of the NewFace class

#ifndef DATA_NEW_FACE_HPP
#define DATA_NEW_FACE_HPP

class NewFace {

  /// @class    NewFace
  /// @ingroup  Data
  /// @brief    [\ref Data]

public: // interface


  /// Constructor
  NewFace(Index index_this, Index index_that) throw()
    : index_this_(index_this),
      index_that_(index_that)
  {
  }

  NewFace() = default;
  NewFace(const NewFace &) = default;
  NewFace & operator = (const NewFace &) = default;
  NewFace( NewFace&&) = default;
  NewFace & operator = (NewFace &&) = default;
  ~NewFace() = default;

  bool operator == (const NewFace & f);
  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  {
    TRACEPUP;
    p | index_this_;
    p | index_that_;
  }

  //--------------------------------------------------

  /// Return the number of bytes required to serialize the data object
  int data_size () const;

  /// Serialize the object into the provided empty memory buffer.
  /// Returns the next open position in the buffer to simpliiy
  /// serializing multiple objects in one buffer.
  char * save_data (char * buffer) const;

  /// Restore the object from the provided initialized memory buffer data.
  /// Returns the next open position in the buffer to simpliiy
  /// serializing multiple objects in one buffer.
  char * load_data (char * buffer);

  void print()
  {
    // CkPrintf ("[%d:%d %d %d]\n",
    //           bit_array_.level-BIAS,
    //           bit_array_.ax   -BIAS,
    //           bit_array_.ay   -BIAS,
    //           bit_array_.az   -BIAS);
  }
private: // functions


private: // attributes

  // NOTE: change pup() function whenever attributes change

  //  union {
  //    FaceBits bit_array_;
  //    int      int_array_;
  //  };
  Index index_this_;
  Index index_that_;
};

#endif /* DATA_NEW_FACE_HPP */

