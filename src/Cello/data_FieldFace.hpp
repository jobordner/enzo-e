// See LICENSE_CELLO file for license and copyright information

/// @file     data_FieldFace.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2011-04-12
/// @brief    [\ref Data] Interface for the FieldFace class

#ifndef DATA_FIELD_FACE_HPP
#define DATA_FIELD_FACE_HPP

class Refresh;
class Prolong;
class Restrict;
class Box;

class FieldFace {

  /// @class    FieldFace
  /// @ingroup  Data
  /// @brief [\ref Data] Class for loading field faces and storing
  /// field ghosts zones

public: // interface

  static int64_t counter[CONFIG_NODE_SIZE];

  FieldFace (int level, int face_type, 
             int if3[3], int ic3[3], int g3[3],
             Refresh * refresh, bool new_refresh = true)
    : level_(level),
      face_type_(face_type),
      face_{if3[0],if3[1],if3[2]},
      child_{ic3[0],ic3[1],ic3[2]},
      ghost_{g3[0],g3[1],g3[2]},
      refresh_(refresh),
      new_refresh_(new_refresh)
  {
    ++counter[cello::index_static()];
  }

  /// Constructor of uninitialized FieldFace
  FieldFace () throw()
    : level_(0),
      face_type_(0),
      face_{0,0,0},
      ghost_{0,0,0},
      child_{0,0,0},
      refresh_(nullptr),
      new_refresh_(false)
  {
    ++counter[cello::index_static()]; 
  }

  /// Destructor
  ~FieldFace() throw()
  {
    --counter[cello::index_static()];

    if (new_refresh_) {
      delete refresh_;
      refresh_ = nullptr;
    }
  }


  /// Copy constructor
  FieldFace(const FieldFace & FieldFace) throw();

  /// Assignment operator
  FieldFace & operator= (const FieldFace & FieldFace) throw();

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  //----------------------------------------------------------------------

  /// Return whether or not to include ghost zones along each axis
  void ghost (int *gx, int *gy, int *gz)
  {
    *gx = ghost_[0];
    *gy = ghost_[1];
    *gz = ghost_[2];
  }
  /// Return the face
  inline void face (int * fx, int * fy = 0, int * fz = 0)
  {
    if (fx) (*fx) = face_[0]; 
    if (fy) (*fy) = face_[1]; 
    if (fz) (*fz) = face_[2]; 
  }

  /// Invert the face direction depending on source / destination
  /// perspective
  inline void invert_face ()
  {
    face_[0] = -face_[0];
    face_[1] = -face_[1];
    face_[2] = -face_[2];
  }

  /// Return level of associated Block
  int level () const { return level_; }

  /// Return the face type: > 0 for finer-level block along the face,
  /// < 0 for coarser-level face block, or 0 for same level
  int face_type() const { return face_type_; }

  /// Return the Refresh object
  Refresh * refresh () const { return refresh_; }

  /// Create an array with the field's face data
  void face_to_array(Field field, int * n, cello_float ** array) throw();

  /// Use existing array for field's face data
  void face_to_array (Field field, cello_float * array) throw();

  /// Copy the input array data to the field's ghost zones

  void array_to_face(cello_float * array, Field field) throw();

  /// Copy directly the face from source FieldData to destination FieldData
  void face_to_face (Field field_src, Field field_dst);

  /// Calculate the number of elements in the packed array
  int num_elements_array (Field field) throw();

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

  void print (const char * message);

  //--------------------------------------------------

private: // functions

  const Prolong * prolong_ () const
  { return refresh_->get_prolong(); }

  const Restrict * restrict_ () const
  { return refresh_->get_restrict(); }

  /// copy data
  void copy_(const FieldFace & field_face); 

  /// Load field block face into the field_face array; return number
  /// of bytes copied
  size_t load_ (      cello_float * array_face,
                const cello_float * field_face,
		int nd3[3], int nf3[3], int im3[3],
		bool accumulate) throw();

  /// Copy the field_face array into the field block ghosts; return
  /// number of bytes copied
  size_t store_ (      cello_float * field_ghosts,
                 const cello_float * array_ghosts, 
		 int nd3[3], int nf3[3], int im3[3],
		 bool accumulate) throw();

  /// Copy a field block face into another block's ghost zones
  void copy_ (      cello_float * vd, int md3[3], int nd3[3], int id3[3],
	      const cello_float * vs, int ms3[3], int ns3[3], int is3[3],
	      bool accumulate) throw();

  /// Multiply the given field by density to convert to conservative
  /// form if needed
  void mul_by_density_
  (Field field, int index_field,
   const int i3[3], const int n3[3], const int m3[3]);

  /// Divide the given field by density to convert back to original
  /// form if needed
  void div_by_density_
  (Field field, int index_field,
   const int i3[3], const int n3[3], const int m3[3]);

  /// Initialize the associated Box object box_ using current
  /// attributes. Invert face direction if at destination's
  /// perspective
  void set_box_(Box * box, bool invert = false);

  /// Adjust box for accumulating values instead of assigning them
  void box_adjust_accumulate_ (Box * box, int accumulate, int g3[3]);

  /// Whether this FieldFace operation involves interpolating in
  /// time for adaptive timestepping
  bool include_history_() const;

  /// Perform interpolation in time on prolonged fields if needed in
  /// adaptive timestepping
  void time_interpolate_(Field field,
                         const std::vector<int> & field_list,
                         bool invert_face=false);

private: // attributes

  /// Level of the associated block
  int level_;

  /// Face type: finer (> 0) , coarser (< 0), or same (0)
  int face_type_;

  /// Select face, including edges and corners (-1,-1,-1) to (1,1,1)
  int face_[3];

  /// Whether to include ghost zones along x,y,z axes
  int ghost_[3];

  /// Child index (0,0,0) to (1,1,1) if restriction or prolongation are used
  int child_[3];

  /// Refresh object for lists of particles and fields to copy,
  /// and whether to copy or add
  Refresh * refresh_;

  /// Whether refresh object should be deleted in destructor
  bool new_refresh_;

};

#endif /* DATA_FIELD_FACE_HPP */
