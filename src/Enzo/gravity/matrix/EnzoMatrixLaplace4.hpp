// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMatrixlaplace4.hpp 
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     2015-04-02
/// @brief    [\ref Compute] Declaration of the 4th order Laplace operator

#ifndef COMPUTE_MATRIX_LAPLACE4_HPP
#define COMPUTE_MATRIX_LAPLACE4_HPP

class EnzoMatrixLaplace4 : public Matrix 
{
  /// @class    EnzoMatrixLaplace4
  /// @ingroup  Compute
  /// @brief    [\ref Compute] Interface to an application compute / analysis / visualization function.

public: // interface

  /// Create a new EnzoMatrixLaplace4
  EnzoMatrixLaplace4 () throw()
    : Matrix()
  {}

  /// Destructor
  virtual ~EnzoMatrixLaplace4() throw()
  {}

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoMatrixLaplace4);

  /// CHARM++ migration constructor
  EnzoMatrixLaplace4(CkMigrateMessage *m)
    : Matrix(m)
  { }

    /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  { TRACEPUP;
    PUP::able::pup(p);
  }

public: // virtual functions

  /// Apply the matrix to a vector Y <-- A*X
  virtual void matvec (int id_y, int id_x,
                       Field field, double hx, double hy, double hz,
                       int g0=1) throw();

  /// Low-level matvec, useful for non-Block arrays (e.g. Block-local
  /// multigrid).  Must call set_cell_width and set_dimensions first
  /// manually!
  virtual void matvec (precision_type precision, void * y, void * x,
                       Field field, double hx, double hy, double hz,
                       int g0=1) throw();

  /// Extract the diagonal into the given field
  virtual void diagonal (int id_x,
                         Field field, double hx, double hy, double hz,
                         int g0=1) throw();

  /// Whether the matrix is singular or not
  virtual bool is_singular() const throw()
  { return true; }

  /// How many ghost zones required for matvec
  virtual int stencil_width() const throw()
  { return 2; }

  virtual double stencil_value (int ix, int iy, int iz,
                                double hx, double hy, double hz) const override;

protected: // functions

  void matvec_ (enzo_float * Y, enzo_float * X,
                Field field, double hx, double hy, double hz,
                int g0) const throw();

  void diagonal_ (enzo_float * X,
                  Field field, double hx, double hy, double hz,
                  int g0) const throw();

protected: // attributes

};

#endif /* COMPUTE_MATRIX_LAPLACE4_HPP */
