// See LICENSE_CELLO file for license and copyright information

/// @file     data_ParticleData.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-10-12
/// @brief    [\ref Data] Declaration of the ParticleData class

#ifndef DATA_PARTICLE_DATA_HPP
#define DATA_PARTICLE_DATA_HPP

class ParticleData {

  /// @class    ParticleData
  /// @ingroup  Data
  /// @brief    [\ref Data]

  friend class Particle;

public: // interface

  static int64_t counter[CONFIG_NODE_SIZE];
  static int64_t id_counter[CONFIG_NODE_SIZE];

  /// Constructor
  ParticleData();

  /// Destructor
  ~ParticleData();

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  /// Return the attribute array for the given particle type and batch
  char * attribute_array (const ParticleDescr *pd, int it, int ia, int ib);
  const char * attribute_array (const ParticleDescr *pd, int it, int ia, int ib) const
  {
    return (const char *)
      ((ParticleData*)this) -> attribute_array (pd,it,ia,ib);
  }

  /// Return the number of batches of particles for the given type.

  int num_batches (int it) const;

  /// Return the number of particles in the given batch, of the given
  /// type, or total on the block.

  int num_particles (const ParticleDescr *) const;
  int num_particles (const ParticleDescr *, int it) const;
  int num_particles (const ParticleDescr *, int it, int ib) const;

  int num_local_particles (const ParticleDescr *) const;
  int num_local_particles (const ParticleDescr *, int it) const;
  int num_local_particles (const ParticleDescr *, int it, int ib) const;

  /// Create the given number of particles of the given type.  Always
  /// creates them at the end instead of filling up any unused
  /// particle spaces in earlier batches, to ease initialization via
  /// index()

  int insert_particles (const ParticleDescr *, int it, int np, const bool copy = false);

  /// Delete the given particles in the batch according to mask
  /// attribute.  Compresses the batch if particles deleted, so batch
  /// may have fewer than max number of particles.  Other batches
  /// remain unchanged.

  int delete_particles (const ParticleDescr *, int it, int ib, const bool * m = 0);

  /// Scatter particles among an array of other Particle structures.
  /// Typically used for preparing to send particles that have gone
  /// out of the block to neighboring blocks.  Particles are not deleted,
  /// so must be done so manually, e.g. using delete_particles.

  void scatter (const ParticleDescr *, int it, int ib,
		int np, const bool * mask, const int * index,
		int n,  ParticleData * particle_array[], const bool copy = false);

  /// Gather particles from an array of other Particle structures.
  /// Typically used after receiving particles from neighboring blocks
  /// that have entered this block.  Return the total number of particles
  /// inserted

  int gather (const ParticleDescr *, int it, int n, ParticleData * particle_array[]);

  /// Compress particles in batches so that all batches except
  /// possibly the last have batch_size() particles.  May be performed
  /// periodically to recover unused memory from multiple insert/deletes

  void compress (const ParticleDescr *);
  void compress (const ParticleDescr *, int it);

  /// Remove any empty batches created after deleting particles. Must
  /// be called by user code /after/ deleting particles
  void cull_empty_batches(const ParticleDescr * particle_descr);
  void cull_empty_batches(const ParticleDescr * particle_descr, int it);

  /// Return the storage "efficiency" for particles of the given type
  /// and in the given batch, or average if batch or type not specified.
  /// 1.0 means no wasted storage, 0.5 means twice as much storage
  /// is being used.  Defined as 1 / overhead().

  float efficiency (const ParticleDescr *);
  float efficiency (const ParticleDescr *, int it);
  float efficiency (const ParticleDescr *, int it, int ib);

  /// Return the storage "overhead" for particles of the given type
  /// and in the given batch, or average if batch or type not specified.
  /// 1.0 means no wasted storage, 2.0 means twice as much storage
  /// is being used.  Defined as 1 / overhead().

  float overhead (const ParticleDescr *);
  float overhead (const ParticleDescr *, int it);
  float overhead (const ParticleDescr *, int it, int ib);

  /// Add a new type to the attribute_array.  Should only be called
  /// by Particle.

  /// Allocate arrays based on const ParticleDescr attributes
  void allocate(const ParticleDescr * particle_descr)
  {
    const size_t nt = particle_descr->num_types();
    if (attribute_array_.size() < nt) {
      attribute_array_.resize(nt);
    }
    if (attribute_align_.size() < nt) {
      attribute_align_.resize(nt);
    }
    if (particle_count_.size() < nt) {
      particle_count_.resize(nt);
    }
  };

  /// Fill a vector of position coordinates for the given type and batch
  bool position (const ParticleDescr * particle_descr,
		 int it, int ib,
		 double * x, double * y = 0, double * z = 0);

  /// Update positions in a batch a given amount.  Only used in refresh for
  /// updating positions in periodic boundary conditions
  void position_update
  (const ParticleDescr * particle_descr,int it, int ib,
   long double dx, long double dy, long double dz);

  /// Fill a vector of velocity coordinates for the given type and batch
  bool velocity (const ParticleDescr * particle_descr,
		 int it, int ib,
		 double * vx, double * vy = 0, double * vz = 0);

  //--------------------------------------------------

  /// Return the number of bytes required to serialize the data object
  int data_size (const ParticleDescr * particle_descr) const;

  /// Serialize the object into the provided empty memory buffer.
  /// Returns the next open position in the buffer to simplify
  /// serializing multiple objects in one buffer.
  char * save_data (const ParticleDescr * particle_descr, char * buffer) const;

  /// Restore the object from the provided initialized memory buffer data.
  /// Returns the next open position in the buffer to simplify
  /// serializing multiple objects in one buffer.
  char * load_data (const ParticleDescr * particle_descr, char * buffer);

  //--------------------------------------------------

  void debug (const ParticleDescr * particle_descr);

private: /// functions

  /// Return an id (not "index"); for a particle that is guaranteed to
  /// be unique across all processors.  May involve communication.

  /// long long assign_id_ ()

  /// Allocate attribute_array_ block, aligned at 16 byte boundary
  /// with updated attribute_align_
  void resize_attribute_array_ (const ParticleDescr *, int it, int ib, int np);

  void check_arrays_ (const ParticleDescr * particle_descr,
		      std::string file, int line) const;

  /// Copy the given floating point attribute of given type (float,
  /// double, quad, etc.) to the given coordinate double position
  /// array.
  void copy_attribute_float_
  (const ParticleDescr * particle_descr,
   int type, int it, int ib, int ia, double * coord);

  /// Increment the given floating point attribute of given float type
  /// (float, double, quad, etc.) by the given double long constant value.
  void update_attribute_float_
  (const ParticleDescr * particle_descr,
   int type, int it, int ib, int ia, long double da);

  /// Positions may be defined relative to the Block using integer
  /// variables.  If so, copy the position coordinate to the given
  /// double position array, where coordinate range in the Block is
  /// [-1.0, 1.0).  Conversion to double may involve floating-point
  /// errors, especially if position is defined using large ints,
  /// e.g. int64_t
  void copy_position_int_
  (const ParticleDescr * particle_descr,
   int type, int it, int ib, int ia, double * coord);

  /// Update version of copy_position_int_()
  void update_position_int_
  (const ParticleDescr * particle_descr,
   int type, int it, int ib, int ia, int64_t da);

  void write_ifrite (const ParticleDescr * particle_descr,
		     int it, std::string file_name,
		     double xm, double ym, double zm,
		     double xp, double yp, double zp);

  /// Return the batch ib and particle index ip for the first empty spot
  /// for the given particle type it. Return true iff batch is empty
  bool next_particle_space_
  ( const ParticleDescr * particle_descr,int it,int *ib, int *ip);

private: /// attributes

  /// Array of blocks of particle attributes array_[it][ib][iap];
  std::vector< std::vector< std::vector<char> * > > attribute_array_;

  /// Alignment adjustment to correct for 16-byte alignment of
  /// first attribute in each batch

  std::vector< std::vector< char > > attribute_align_;

  /// Number of particles in the batch particle_count_[it][ib];
  std::vector < std::vector < int > > particle_count_;

};

#endif /* DATA_PARTICLE_DATA_HPP */
