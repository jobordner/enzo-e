// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_EnzoMatrixDiagonal.hpp 
/// @author   James Bordner (jobordner@ucsd.edu) 
/// @date     2015-04-02
/// @brief    [\ref Compute] Declaration of the EnzoMatrixDiagonal class

#ifndef COMPUTE_MATRIX_DIAGONAL_HPP
#define COMPUTE_MATRIX_DIAGONAL_HPP

class EnzoMatrixDiagonal : public Matrix 
{
  /// @class    EnzoMatrixDiagonal
  /// @ingroup  Compute
  /// @brief    [\ref Compute] Interface to an application compute / analysis / visualization function.

public: // interface

  /// Create a new EnzoMatrixDiagonal
  EnzoMatrixDiagonal () throw()
  : Matrix ()
  {}

  /// Destructor
  virtual ~EnzoMatrixDiagonal() throw()
  {}

  /// Charm++ PUP::able declarations
  PUPable_decl(EnzoMatrixDiagonal);

  /// CHARM++ migration constructor
  EnzoMatrixDiagonal(CkMigrateMessage *m)
    : Matrix(m)
  {}

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p)
  { TRACEPUP;
    PUP::able::pup(p);
  }

public: // virtual functions

  /// Apply the matrix to a vector Y <-- A*X
  virtual void matvec (int id_y, int id_x,
                       Field field, double hx, double hy, double hz,
                       int g0 = 1) throw();
  virtual void matvec (precision_type precision,
		       void * y, void * x,
                       Field field, double hx, double hy, double hz,
                       int g0=1) throw();

  /// Extract the diagonal into the given field
  virtual void diagonal (int id_x,
                         Field field, double hx, double hy, double hz,
                         int g0 = 1) throw();

protected: // functions

  void matvec_ (enzo_float * Y, enzo_float * X,
                Field field, double hx, double hy, double hz,
                int g0) const throw();

  void diagonal_ (enzo_float * X,
                  Field field, double hx, double hy, double hz,
                  int g0) const throw();

  /// Whether the matrix is singular or not
  virtual bool is_singular() const throw()
  { return false; }

  /// How many ghost zones required for matvec
  virtual int stencil_width() const throw() override
  { return 0; }

  virtual double stencil_value (int ix, int iy, int iz,
                                double hx, double hy, double hz) const override;

};

#endif /* COMPUTE_MATRIX_DIAGONAL_HPP */
