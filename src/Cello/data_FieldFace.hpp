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

  static long counter[CONFIG_NODE_SIZE];

  FieldFace (int level, int face_type, 
             int if3[3], int ic3[3], int g3[3],
             Refresh * refresh, bool new_refresh = true)
  {
    ++counter[cello::index_static()];
    set_level (level);
    set_face_type (face_type);
    set_child (ic3[0],ic3[1],ic3[2]);
    set_face (if3[0],if3[1],if3[2]);
    set_ghost(g3[0],g3[1],g3[2]);
    set_refresh(refresh,new_refresh);
  }

  /// Constructor of uninitialized FieldFace
  FieldFace () throw();

  /// Destructor
  ~FieldFace() throw();

  /// Copy constructor
  FieldFace(const FieldFace & FieldFace) throw();

  /// Assignment operator
  FieldFace & operator= (const FieldFace & FieldFace) throw();

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  //----------------------------------------------------------------------

  /// Set whether or not to include ghost zones along each axis
  void set_ghost (int gx, int gy, int gz)
  {
    ghost_[0] = gx;
    ghost_[1] = gy;
    ghost_[2] = gz;
  }

  void ghost (int *gx, int *gy, int *gz)
  {
    *gx = ghost_[0];
    *gy = ghost_[1];
    *gz = ghost_[2];
  }
  /// Set the face
  inline void set_face (int fx, int fy = 0, int fz = 0)
  {
    face_[0] = fx;
    face_[1] = fy;
    face_[2] = fz;
  }
  
  /// Set the face
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
  /// Set child if restrict or prolong is required
  void set_child(int icx, int icy=0, int icz=0) throw()
  {
    child_[0] = icx;
    child_[1] = icy;
    child_[2] = icz;
  }

  /// Set level of associated Block
  void set_level (int level)
  { level_ = level; }
  /// Return level of associated Block
  int level () const { return level_; }

  /// Set face type: > 0 for finer level, < 0 for coarser level, 0 for
  /// same level
  void set_face_type (int face_type)
  {  face_type_ = face_type;  }

  int face_type() const { return face_type_; }

  void set_refresh (Refresh * refresh, bool new_refresh)
  {
    refresh_ = refresh;
    new_refresh_ = new_refresh;
  }

  /// Return the Refresh object
  Refresh * refresh () const { return refresh_; }

  /// Create an array with the field's face data
  void face_to_array(Field field, int * n, char ** array) throw();

  /// Use existing array for field's face data
  void face_to_array (Field field, char * array) throw();

  /// Copy the input array data to the field's ghost zones

  void array_to_face(char * array, Field field) throw();

  /// Copy directly the face from source FieldData to destination FieldData
  void face_to_face (Field field_src, Field field_dst);

  /// Calculate the number of bytes needed

  int num_bytes_array (Field field) throw();

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

  const Prolong * prolong_ ()
  { return refresh_->get_prolong(); }

  const Restrict * restrict_ ()
  { return refresh_->get_restrict(); }
  
  /// copy data
  void copy_(const FieldFace & field_face); 

  /// Precision-agnostic function for loading field block face into
  /// the field_face array; returns number of bytes copied
  template<class T>
  size_t load_ (      T * array_face,
                const T * field_face,
		int nd3[3], int nf3[3], int im3[3],
		bool accumulate) throw();

  /// Precision-agnostic function for copying the field_face array into
  /// the field block ghosts; returns number of bytes copied
  template<class T>
  size_t store_ (      T * field_ghosts,
                 const T * array_ghosts, 
		 int nd3[3], int nf3[3], int im3[3],
		 bool accumulate) throw();

  /// Precision-agnostic function for copying a field block face
  /// into another block's ghost zones
  template<class T>
  void copy_ (      T * vd, int md3[3], int nd3[3], int id3[3],
	      const T * vs, int ms3[3], int ns3[3], int is3[3],
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
