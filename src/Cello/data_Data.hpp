// See LICENSE_CELLO file for license and copyright information

/// @file     data_Data.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2013-03-10
/// @brief    [\ref Data] Declaration of the Data class
///

#ifndef DATA_DATA_HPP
#define DATA_DATA_HPP

class Data {

  friend class Block;
  friend class IoBlock;

  /// @class    Data
  /// @ingroup  Data
  /// @brief    [\ref Data] 

public: // interface

  /// Constructor
  Data(int nx, int ny, int nz,
       int num_field_data,
       double xm, double xp,
       double ym, double yp,
       double zm, double zp) throw();

  /// Constructor
  Data(FieldDescr * field_descr,
       ParticleDescr * particle_descr,
       int nx, int ny, int nz,
       int num_field_data,
       double xm, double xp,
       double ym, double yp,
       double zm, double zp) throw();

  /// Destructor
  ~Data() throw();

  /// Copy constructor
  Data(const Data & data) throw();

  /// Assignment operator
  Data & operator= (const Data & data) throw();

  /// Empty constructor
  Data() { }

  /// CHARM++ Pack / Unpack function
  inline void pup (PUP::er &p)
  {
    TRACEPUP;
    bool up = p.isUnpacking();
    p | num_field_data_;
    // allocate field_data_[] vector first if unpacking
    if (up) field_data_.resize(num_field_data_);
    for (int i=0; i<num_field_data_; i++) {
      if (up) field_data_[i] = new FieldData;
      p | *field_data_[i];
    }
    if (up) {
      particle_data_ = new ParticleData;
    }
    p | *particle_data_;
    PUParray(p,lower_,3);
    PUParray(p,upper_,3);
    // NOTE: change this function whenever attributes change
  }

  /// Return domain lower extent
  inline void lower(double * x, 
		    double * y = 0,
		    double * z = 0) const throw ()
  {
    if (x) *x = lower_[0];
    if (y) *y = lower_[1];
    if (z) *z = lower_[2];
  }

  //----------------------------------------------------------------------

  /// Return domain upper extent
  inline void upper(double * x,
		    double * y = 0,
		    double * z = 0) const throw ()
  {
    if (x) *x = upper_[0];
    if (y) *y = upper_[1];
    if (z) *z = upper_[2];
  }

  //----------------------------------------------------------------------
  // fields
  //----------------------------------------------------------------------

  /// Return the number of FieldData
  int num_field_data() const throw()
  { return num_field_data_; }

  /// Return the ith Field data
  const FieldData * field_data (size_t i=0) const throw()
  { return (i < field_data_.size()) ? field_data_[i] : NULL; }

  /// Return the ith Field data
  FieldData * field_data (size_t i=0) throw()
  { return (i < field_data_.size()) ? field_data_[i] : NULL; }

  // Return the ith Field descriptor
  FieldDescr * field_descr () throw();
  const FieldDescr * field_descr () const throw();

  /// Return the ith Field
  Field field (size_t i=0) throw()
  { return Field(field_descr(),field_data(i)); }


  /// Return the x,y,z,t coordinates of field cell centers
  void field_cells (double * x, double * y, double * z,
		    int gx = 0, int gy = 0, int gz = 0) const;

  /// Return the cell widths of Fields
  void field_cell_width (double * hx, 
			 double * hy = 0,
			 double * hz = 0) const;

  //----------------------------------------------------------------------
  // particles
  //----------------------------------------------------------------------

  /// Return the ith Particle data
  const ParticleData * particle_data () const throw()
  { return particle_data_; }

  /// Return the ith Particle data
  ParticleData * particle_data () throw()
  { return particle_data_; }

  // Return the ith Particle descriptor
  ParticleDescr * particle_descr () throw();
  const ParticleDescr * particle_descr () const throw();

  /// Return the Particle object
  Particle particle () throw()
  { return Particle(particle_descr(),
		    particle_data_); }

  bool any_particles() const
  { return (particle_descr()->num_types() > 0); }

  bool any_fields() const
  { return (field_descr()->field_count() > 0); }

  void allocate () throw();

private: // functions

  void copy_(const Data & data) throw();

private: // attributes

  /// Number of field_data's (required by CHARM++ PUP::er)
  int num_field_data_;

  /// Array of field data
  std::vector<FieldData *> field_data_;

  /// Particle data
  ParticleData * particle_data_;

  /// Lower extent of the box associated with the block [computable]
  double lower_[3];

  /// Upper extent of the box associated with the block [computable]
  double upper_[3];

  // NOTE: change pup() function whenever attributes change

};

#endif /* DATA_DATA_HPP */

