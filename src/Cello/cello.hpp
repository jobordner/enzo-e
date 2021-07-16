// See LICENSE_CELLO file for license and copyright information

#ifndef CELLO_HPP
#define CELLO_HPP

/// @file    cello.hpp
/// @author  James Bordner (jobordner@ucsd.edu)
/// @date    Thu Nov 11 17:08:38 PST 2010
/// @brief   Include Cello global configuration settings
///
/// This file includes system includes; defines global template functions
/// such as MIN(), MAX(), and INDEX(); and global enumerated types.
/// It also initializes precision-related defines, including default_precision
/// Some global functions and constants are define with cello namespace,
/// sich as cello:pi and cello::err_rel(), etc.

//----------------------------------------------------------------------
// SYSTEM INCLUDES
//----------------------------------------------------------------------

#include <stdio.h>
#include <math.h>

#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <execinfo.h>

#include <charm++.h>

#include "pup_stl.h"

#include "cello_Sync.hpp"

// #define DEBUG_CHECK

class Config;
class CProxy_Block;
class FieldDescr;
class Grouping;
class Hierarchy;
class Monitor;
class Output;
class Parameters;
class ParticleDescr;
class Problem;
class Refresh;
class ScalarDescr;
class Simulation;
class Solver;
class Units;

#ifdef CELLO_DEBUG
# define TRACE_ONCE                                           \
  {                                                             \
    static bool first = true;                                   \
    if (first) {                                                \
      first = false;                                            \
      CkPrintf ("TRACE_ONCE %s:%d\n",__FILE__,__LINE__);        \
    }                                                           \
  }
#else
# define TRACE_ONCE /* ... */
#endif

//----------------------------------------------------------------------
// TEMPLATE FUNCTIONS
//----------------------------------------------------------------------

template <class T>
inline T MIN(const T &a, const T &b) 
{  return a < b ? a : b; }

template <class T>
inline T MAX(const T &a, const T &b) 
{  return a > b ? a : b; }

inline int INDEX(int ix,int iy,int iz,int nx,int ny) 
{  return ix+nx*(iy+ny*iz); }

inline int INDEX2(int ix,int iy,int nx) 
{  return ix+nx*iy; }

template <class T>
inline void SWAP(T &a, T &b) 
{  T t = a; a=b; b=t; }

//----------------------------------------------------------------------
// ENUMERATED TYPES
//----------------------------------------------------------------------

/// @enum     face_enum
/// @brief    Face [lower|upper]
enum face_enum {
  face_lower = 0,
  face_upper = 1,
  face_all
};

/// @enum     axis_enum
/// @brief    Axis [x|y|z]
enum axis_enum {
  axis_x = 0,
  axis_y = 1,
  axis_z = 2,
  axis_all
};
typedef int axis_type;

/// @enum     refresh_type
/// @brief    Type of mesh refinement--coarsen, refine, or stay the same
enum refresh_type {
  refresh_unknown,
  refresh_coarse,
  refresh_same,
  refresh_fine
};

/// @enum     reduce_enum
/// @brief    Reduction operator, used for image projections
enum reduce_enum {
  reduce_unknown, /// Unknown reduction
  reduce_min,     /// Minimal value along the axis
  reduce_max,     /// Maximal value along the axis
  reduce_avg,     /// Average value along the axis
  reduce_sum,     /// Sum of values along the axis
  reduce_set      /// Value of last processed (used for mesh plotting)
};

typedef int reduce_type;


/// @enum precision_enum
/// @brief list of known floating-point precision, used for Field
enum precision_enum {
  // @@@ KEEP IN SYNCH WITH precision_name in cello.cpp
  precision_unknown,     //  unknown precision
  precision_default,     //  default precision
  precision_single,      //  32-bit field data
  precision_double,      //  64-bit field data
  precision_extended80,  //  80-bit field data
  precision_extended96,  //  96-bit field data
  precision_quadruple,   // 128-bit field data
};
typedef int precision_type;

#ifdef CONFIG_PRECISION_SINGLE
   typedef float       cello_float;
#elif  CONFIG_PRECISION_DOUBLE
   typedef double      cello_float;
#elif  CONFIG_PRECISION_QUAD
   typedef long double cello_float;
#else
#  error "Must define one of CONFIG_PRECISION_[SINGLE|DOUBLE|QUAD]"
#endif

/// @enum type_enum
/// @brief list of known scalar types, including ints as well as floats, used for Field types and Particle attributes
enum type_enum {
  type_unknown,     // unknown type
  type_default,    // "default" floating-point precision, e.g. enzo_float
  type_single,
  type_float = type_single,
  type_double,
  type_extended80,
  type_extended96,
  type_quadruple,
  type_long_double = type_quadruple,
  type_int8,
  type_char = type_int8,
  type_int16,
  type_short = type_int16,
  type_int32,
  type_int = type_int32,
  type_int64,
  type_long_long = type_int64,
  NUM_TYPES
};

#ifdef CONFIG_PRECISION_SINGLE
#   define default_precision precision_single
#   define default_type      type_single
#   define SCALAR_DEFINED
#   define default_precision_string "single"
#endif
#ifdef CONFIG_PRECISION_DOUBLE
#   ifdef SCALAR_DEFINED
#      define SCALAR_ERROR
#   endif
#   define default_precision precision_double
#   define default_type      type_double
#   define SCALAR_DEFINED
#   define default_precision_string "double"
#endif
#ifdef CONFIG_PRECISION_QUAD
#   ifdef SCALAR_DEFINED
#      define SCALAR_ERROR
#   endif
#   define default_precision precision_quad
#   define default_type      type_quad
#   define SCALAR_DEFINED
#   define default_precision_string "quadruple"
#endif

#ifndef SCALAR_DEFINED
#   error None of CONFIG_PRECISION_[SINGLE|DOUBLE|QUAD] defined
#endif

#ifdef ERROR_SCALAR
#   error Multiple CONFIG_PRECISION_[SINGLE|DOUBLE|QUAD] defined
#endif

//----------------------------------------------------------------------
/// Macros for sizing, saving, and restoring data from buffers
//----------------------------------------------------------------------


#define SIZE_SCALAR_TYPE(COUNT,TYPE,VALUE)      \
  {						\
    COUNT += sizeof(TYPE);			\
  }

#define SAVE_SCALAR_TYPE(POINTER,TYPE,VALUE)    \
  {                                             \
    int n;                                      \
    memcpy(POINTER,&VALUE,n=sizeof(TYPE));	\
    POINTER += n;                               \
  }

#define LOAD_SCALAR_TYPE(POINTER,TYPE,VALUE)    \
  {                                             \
    int n;                                      \
    memcpy(&VALUE,POINTER,n=sizeof(TYPE));	\
    POINTER += n;                               \
  }


#define SIZE_ARRAY_TYPE(COUNT,TYPE,ARRAY,LENGTH)        \
  {                                                     \
    COUNT += sizeof(int);                               \
    COUNT += LENGTH*sizeof(TYPE);                       \
  }
#define SAVE_ARRAY_TYPE(POINTER,TYPE,ARRAY,LENGTH)      \
  {                                                     \
    int n,length = LENGTH;                              \
    memcpy(POINTER,&length, n=sizeof(int));             \
    POINTER += n;                                       \
    memcpy(POINTER,&ARRAY[0],n=length*sizeof(TYPE));	\
    POINTER += n;                                       \
  }
#define LOAD_ARRAY_TYPE(POINTER,TYPE,ARRAY,LENGTH)      \
  {                                                     \
    int n;                                              \
    memcpy(&LENGTH, POINTER, n=sizeof(int));		\
    POINTER += n;                                       \
    memcpy(&ARRAY[0],POINTER,n=LENGTH*sizeof(TYPE));	\
    POINTER += n;                                       \
  }

#define SIZE_VECTOR_TYPE(COUNT,TYPE,VECTOR)     \
  {						\
    COUNT += sizeof(int);			\
    COUNT += sizeof(TYPE)*VECTOR.size();        \
  }
#define SAVE_VECTOR_TYPE(POINTER,TYPE,VECTOR)                   \
  {                                                             \
    int length = VECTOR.size();                                 \
    int n;                                                      \
    memcpy(POINTER,&length, n=sizeof(int));                             \
    POINTER += n;                                                       \
    memcpy(POINTER,(TYPE*)&VECTOR[0],n=length*sizeof(TYPE));            \
    POINTER += n;                                                       \
  }
#define LOAD_VECTOR_TYPE(POINTER,TYPE,VECTOR)                   \
  {                                                             \
    int length;                                                 \
    int n;                                                      \
    memcpy(&length, POINTER, n=sizeof(int));                    \
    POINTER += n;                                               \
    VECTOR.resize(length);                                      \
    memcpy((TYPE*)VECTOR.data(),POINTER,n=length*sizeof(TYPE));	\
    POINTER += n;                                               \
  }
/// Type for CkMyPe(); used for Block() constructor to differentiate
/// from Block(int)
typedef unsigned process_type;

/// Namespace for global constants and functions
namespace cello {

  // Constants

  // pi
  const double pi = 3.14159265358979324;

  // ergs per eV
  const double erg_eV = 1.60217653E-12;

  // eV per erg
  const double eV_erg = 6.24150948E11;

  // Boltzman constant in CGS
  const double kboltz = 1.3806504e-16;

  // Solar mass in CGS
  const double mass_solar = 1.98841586e33;

  // Hydrogen mass in CGS
  const double mass_hydrogen = 1.67262171e-24;

  // Electron mass in CGS
  const double mass_electron = 9.10938291E-28;

  // parsec in CGS
  const double pc_cm  = 3.0856775809623245E18;

  // Kiloparsec in CGS
  const double kpc_cm = 3.0856775809623245E21;

  // Megaparsec in CGS
  const double Mpc_cm = 3.0856775809623245E24;

  // speed of light in CGS
  const double clight = 29979245800.0;

  // Gravitational constant in CGS
  const double grav_constant = 6.67384E-8;

  // year in seconds
  const double yr_s = 3.1556952E7;

  // kyr in seconds
  const double kyr_s = 3.1556952E10;

  // Myr in seconds
  const double Myr_s = 3.1556952E13;

  // Approximate mean molecular weight of metals
  const double mu_metal = 16.0;

  // precision functions
  double machine_epsilon     (int);
  template <class T>
  T err_rel (const T & a, const T & b)
  {  return (a != 0.0) ? fabs((a - b) / a) : fabs(a-b);  }

  template <class T>
  T err_abs (const T & a, const T & b)
  {  return fabs(a-b);  }

  template <class T>
  T sum (const T * array,
         int mx, int my, int mz,
         int ox, int oy, int oz,
         int nx, int ny, int nz)
  {
    T s = 0.0;
    int o = ox + mx*(oy + my*oz);
    for (int iz=0; iz<nz; iz++) {
      for (int iy=0; iy<ny; iy++) {
        for (int ix=0; ix<nx; ix++) {
          int i=ix+mx*(iy+my*iz) + o;
          s += array[i];
        }
      }
    }
    return s;
  }

  template <class T>
  void copy (T * array_d,
          int mdx, int mdy, int mdz,
          int odx, int ody, int odz,
          const T * array_s,
          int msx, int msy, int msz,
          int osx, int osy, int osz,
          int nx, int ny, int nz)
  {
    int os = osx + msx*(osy + msy*osz);
    int od = odx + mdx*(ody + mdy*odz);
    T sum = 0.0;
    int count=0;
    for (int iz=0; iz<nz; iz++) {
      for (int iy=0; iy<ny; iy++) {
        for (int ix=0; ix<nx; ix++) {
          int is = os + ix+msx*(iy+msy*iz);
          int id = od + ix+mdx*(iy+mdy*iz);
          array_d[id] = array_s[is];
          sum += array_d[id];
          ++count ;
        }
      }
    }
  }

  int digits_max(int precision);

  // type_enum functions (prefered)
  int sizeof_type (int);
  int is_type_supported (int);

  extern bool type_is_float(int type);
  extern bool type_is_int(int type);
  extern bool type_is_valid(int type);

  extern const char * type_name[NUM_TYPES];
  extern const int type_bytes[NUM_TYPES];

  // precision_enum functions (depreciated)

  int sizeof_precision       (precision_type);
  int is_precision_supported (precision_type);
  extern const char * precision_name[7];

  inline void hex_string(char str[], int length)
  {
    //hexadecimal characters
    char hex_characters[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    static std::default_random_engine generator;
    static std::uniform_int_distribution<int> distribution(0,15);
    int i;
    for(i=0;i<length;i++)
      {
        str[i]=hex_characters[distribution(generator)];
      }
    str[length]=0;
  }

  /// Check a number for zero, NAN, and inf

  template <class T>
  void check (T value, const char * message,
	      const char * file, int line)
  {
    if (std::fpclassify(value) == FP_ZERO) {
      printf ("WARNING: %s:%d %s zero\n", file,line,message);	
    }					
    if (std::fpclassify(value) == FP_NAN) {
      printf ("WARNING: %s:%d %s nan\n", file,line,message);	
    }					
    if (std::fpclassify(value) == FP_INFINITE) {
      printf ("WARNING: %s:%d %s inf\n", file,line,message);	
    }
#ifdef DEBUG_CHECK    
    if (sizeof(value)==sizeof(float))
      CkPrintf ("DEBUG_CHECK %s = %25.15g\n",message,value);
    if (sizeof(value)==sizeof(double))
      CkPrintf ("DEBUG_CHECK %s = %25.15lg\n",message,value);
    if (sizeof(value)==sizeof(long double))
      CkPrintf ("DEBUG_CHECK %s = %25.15Lg\n",message,value);
#endif    
  }

  inline int index_static()
  { return CkMyPe() % CONFIG_NODE_SIZE; }

  inline void af_to_xyz (int axis, int face, int r3[3])
  {
    r3[0] = (axis==0) ? 2*face-1 : 0;
    r3[1] = (axis==1) ? 2*face-1 : 0;
    r3[2] = (axis==2) ? 2*face-1 : 0;
  }

  /// Return a pointer to the Simulation object on this process
  Simulation *    simulation();
  /// Return a proxy for the Block chare array of Blocks
  CProxy_Block    block_array();
  /// Return a pointor to the Problem object defining the problem being solved
  Problem *       problem();
  /// Return a pointer to the Hierarchy object defining the mesh hierarchy
  Hierarchy *     hierarchy();
  /// Return a pointer to the Config object containing user parameters values
  const Config *  config();
  /// Return a pointer to the Parameters object
  const Parameters *  parameters();
  /// Return a pointer to the FieldDescr object defining fields on Blocks
  FieldDescr *    field_descr();
  /// Return a pointer to the Groupings object defining field groups
  Grouping *      field_groups();
  /// Return a pointer to the ParticledDescr object defining particles on Blocks
  ParticleDescr * particle_descr();
  /// Return a pointer to the Groupings object defining particle groups
  Grouping *      particle_groups();
  /// Return a pointer to the Monitor object for writing output to stdout
  Monitor *       monitor();
  /// Return a pointer to the Units object
  Units *         units();
  /// Return reference to in indexed Refresh object
  Refresh *       refresh(int ir);

  /// Return the ScalarDescr object defining Block long double Scalar data values
  ScalarDescr *   scalar_descr_long_double();
  /// Return the ScalarDescr object defining Block double Scalar data values
  ScalarDescr *   scalar_descr_double();
  /// Return the ScalarDescr object defining Block int Scalar data values
  ScalarDescr *   scalar_descr_int();
  /// Return the ScalarDescr object defining Block Sync counter Scalar data values
  ScalarDescr *   scalar_descr_sync();
  /// Return the ScalarDescr object defining Block pointer Scalar data values
  ScalarDescr *   scalar_descr_void();

  /// Return the ith Output object
  Output *        output (int index);
  /// Return the ith Solver
  Solver *        solver(int index);
  /// Return the dimensional rank of the simulation
  int             rank ();
  /// Return the number of children each Block may have
  int             num_children();
  /// Return the number of Blocks on this process
  size_t          num_blocks_process();
  /// Return the cell volume at the given level relative to the root level
  double          relative_cell_volume (int level);
}

#endif /* CELLO_HPP */
