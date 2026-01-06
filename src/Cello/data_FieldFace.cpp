// See LICENSE_CELLO file for license and copyright information

/// @file     data_FieldFace.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2011-04-12
/// @brief    Implementation of the FieldFace class

#include "cello.hpp"
#include "data.hpp"

#define FORTRAN_STORE

//----------------------------------------------------------------------

// #define CONFIG_SMP_MODE

static CmiNodeLock field_face_node_lock;
void mutex_init_field_face()
{  field_face_node_lock = CmiCreateLock(); }

//======================================================================

long FieldFace::counter[CONFIG_NODE_SIZE] = {0};

//----------------------------------------------------------------------

#define FORTRAN_NAME(NAME) NAME##_

extern "C" void FORTRAN_NAME(field_face_store_4)
  (float * field, float * array, int * m3, int * n3, int * accumulate);
extern "C" void FORTRAN_NAME(field_face_store_8)
  (double * field, double * array, int * m3, int * n3, int * accumulate);
extern "C" void FORTRAN_NAME(field_face_store_16)
  (long double * field, long double * array, int * m3, int * n3, int * accumulate);

//----------------------------------------------------------------------

enum enum_op_type {
  op_unknown,
  op_load,
  op_store
};


//----------------------------------------------------------------------

#ifdef CHECK_COARSE
#   undef  CHECK_COARSE
#   define CHECK_COARSE(FIELD,index_field)                 \
  { \
    cello_float * coarse = (cello_float *)FIELD.coarse_values(index_field);     \
    ASSERT ("CHECK_COARSE","coarse array is null", (coarse != nullptr)); \
  }
#else
#   define CHECK_COARSE(FIELD,index_field)  /* ... */
#endif

//----------------------------------------------------------------------

FieldFace::FieldFace (int rank) throw()
  : rank_( rank ? rank : cello::rank() ),
    level_(0),
    face_type_(0),
    refresh_(NULL),
    new_refresh_(false)
{
  ++counter[cello::index_static()]; 
  for (int i=0; i<3; i++) {
    face_[i] = 0;
    ghost_[i] = 0;
    child_[i] = 0;
  }

}

//----------------------------------------------------------------------

FieldFace::~FieldFace() throw ()
{
  --counter[cello::index_static()];

  if (new_refresh_) {
    delete refresh_;
    refresh_ = NULL;
  }
}

//----------------------------------------------------------------------

FieldFace::FieldFace(const FieldFace & field_face) throw ()
  : rank_(0),
    level_(0),
    face_type_(0),
    refresh_(NULL),
    new_refresh_(false)
{
  ++counter[cello::index_static()];

  copy_(field_face);
}

//----------------------------------------------------------------------

FieldFace & FieldFace::operator= (const FieldFace & field_face) throw ()
/// @param     field_face  Source object of the assignment
///
/// @return    The target assigned object
{
  copy_(field_face);
  return *this;
}

//----------------------------------------------------------------------

void FieldFace::copy_(const FieldFace & field_face)
{
  for (int i=0; i<3; i++) {
    face_[i]  = field_face.face_[i];
    ghost_[i] = field_face.ghost_[i];
    child_[i] = field_face.child_[i];
  }
  rank_       = field_face.rank_;
  level_      = field_face.level_;
  face_type_  = field_face.face_type_;
  refresh_    = field_face.refresh_;
  // new_refresh_ must not be true in more than one FieldFace to avoid
  // multiple deletes
  new_refresh_  = false;
}

//----------------------------------------------------------------------

void FieldFace::pup (PUP::er &p)
{

  // NOTE: change this function whenever attributes change

  TRACEPUP;

  PUParray(p,face_,3);
  PUParray(p,ghost_,3);
  PUParray(p,child_,3);
  p | rank_;
  p | level_;
  p | face_type_;
  p | refresh_;
  p | new_refresh_;
}

//======================================================================

void FieldFace::face_to_array ( Field field, int * n, char ** array) throw()
{
  ASSERT("FieldFace::face_to_array()",
	 "field_src.size() must be > 0",
	 refresh_->any_fields());

  *n = num_bytes_array(field);
  *array = new char [*n];

  ASSERT("FieldFace::face_to_array()",
	 "array size must be > 0",
	 *n > 0);

  face_to_array (field, *array);

}

//----------------------------------------------------------------------
void FieldFace::face_to_array ( Field field,char * array) throw()
{
  size_t index_array = 0;

  auto field_list_src = refresh_->field_list_src(level_,face_type_);
  auto field_list_dst = refresh_->field_list_dst(level_,face_type_);

  for (size_t i_f=0; i_f < field_list_src.size(); i_f++) {
    const size_t index_field = field_list_src[i_f];
    CHECK_COARSE(field,index_field);

    precision_type precision = field.precision(index_field);

    char * field_face = field.values(index_field);
    char * array_face  = &array[index_array];

    int m3[3],g3[3],c3[3];

    field.dimensions (index_field,m3,m3+1,m3+2);
    field.ghost_depth(index_field,g3,g3+1,g3+2);
    field.centering  (index_field,c3,c3+1,c3+2);

    const bool accumulate = refresh_->accumulate(i_f);

    int i3[3], n3[3];
    field.size(n3,n3+1,n3+2);
    Box box(rank_,n3,g3);
    box.set_centering(c3);

    set_box_(&box);

    box_adjust_accumulate_(&box,accumulate,g3);

    box.compute_region();
    // limits for Send block 
    //    box.get_start_size(i3,n3,BlockType::receive,BlockType::send);
    bool lpad;
    box.get_start_size(i3,n3,BlockType::send,BlockType::send,lpad=true);

    // scale by density if needed to convert to conservative form
    mul_by_density_(field,index_field,i3,n3,m3);

    if (face_type_ < 0) {

      // Restrict field to array

      int nc3[3] = { (n3[0]+1)/2, (n3[1]+1)/2,(n3[2]+1)/2 };

      int i3_array[3] = {0,0,0};

      index_array += restrict()->apply
	(precision, 
	 array_face,nc3,i3_array,nc3, 
	 field_face,m3,i3, n3);

    } else {

      union { float * a4; double * a8; long double * a16; };
      union { float * f4; double * f8;long double * f16;  };
      a4 = (float *) array_face;
      f4 = (float *) field_face;

      // Copy field to array
      if (precision == precision_single) {
	index_array += load_ ( a4,  f4,  m3,n3,i3, accumulate);
      } else if (precision == precision_double) {
	index_array += load_ ( a8,  f8,  m3,n3,i3, accumulate);
      } else if (precision == precision_quadruple) {
	index_array += load_ ( a16, f16, m3,n3,i3, accumulate);
      } else {
	ERROR("FieldFace::face_to_array", "Unsupported precision");
      }
    }

    // unscale by density if needed to convert back from conservative form
    div_by_density_(field,index_field,i3,n3,m3);

  }
}

//----------------------------------------------------------------------

void FieldFace::array_to_face (char * array, Field field) throw()
{
  size_t index_array = 0;

  auto field_list_src = refresh_->field_list_src(level_,face_type_);
  auto field_list_dst = refresh_->field_list_dst(level_,face_type_);

  for (size_t i_f=0; i_f < field_list_dst.size(); i_f++) {
    size_t index_field = field_list_dst[i_f];

    CHECK_COARSE(field,index_field);

    precision_type precision = field.precision(index_field);

    char * field_ghost =  field.values( index_field);

    char * array_ghost  = array + index_array;

    int m3[3],g3[3],c3[3];

    field.dimensions (index_field,m3,m3+1,m3+2);
    field.ghost_depth(index_field,g3,g3+1,g3+2);
    field.centering  (index_field,c3,c3+1,c3+2);

    const bool accumulate = refresh_->accumulate(i_f);

    int i3[3], n3[3];

    // adjust face relative to sender
    invert_face();

    field.size(n3,n3+1,n3+2);
    Box box (rank_,n3,g3);
    set_box_(&box);
    box.set_centering(c3);
    invert_face();

    box_adjust_accumulate_(&box,accumulate,g3);

    bool lpad;
    box.get_start_size(i3,n3,BlockType::receive,BlockType::receive,lpad=false);

    if (face_type_ > 0) {

      // Prolong array to field

      ASSERT ("FieldFace::array_to_face()",
              "No prolongation operator",
              (prolong() != nullptr));

      int ic3[3];
      int nc3[3];
      box.get_start_size(ic3,nc3,BlockType::send,BlockType::send,lpad=true);
      int mc3[3] = {nc3[0],nc3[1],nc3[2]};
      // reset ic3 for array
      ic3[0] = 0;
      ic3[1] = 0;
      ic3[2] = 0;

      // adjust for full-block interpolation to child

      prolong()->apply
        (precision,
         field_ghost,m3, i3,  n3,
         array_ghost,mc3,ic3, nc3,
         accumulate);

      index_array += cello::sizeof_precision(precision)*
        nc3[0]*nc3[1]*nc3[2];

    } else {

      // Copy array to field
      union { float * as4; double * as8; long double * as16; };
      union { float * fd4; double * fd8; long double * fd16; };
      as4 = (float *) array_ghost;
      fd4 = (float *) field_ghost;

      // Copy field to array

      if (precision == precision_single) {
	index_array += store_ ( fd4,  as4,  m3,n3,i3, accumulate);
      } else if (precision == precision_double) {
	index_array += store_ ( fd8,  as8,  m3,n3,i3, accumulate);
      } else if (precision == precision_quadruple) {
	index_array += store_ ( fd16, as16, m3,n3,i3, accumulate);
      } else {
	ERROR("FieldFace::array_to_face()", "Unsupported precision");
      }
    }

    // unscale by density if needed to convert back from conservative form
    div_by_density_(field,index_field,i3,n3,m3);

  }
  // Interpolate fields in time if needed when adaptive time-stepping
  // (note invert_face parameter is set since at receiving end)
  time_interpolate_(field,field_list_dst,true);
}

//----------------------------------------------------------------------

void FieldFace::face_to_face (Field field_src, Field field_dst)
{
  auto field_list_src = refresh_->field_list_src(level_,face_type_);
  auto field_list_dst = refresh_->field_list_dst(level_,face_type_);

#ifdef CONFIG_SMP_MODE
  PERF_SMP_START(perf_rindex_smp_field_face);
  CmiLock(field_face_node_lock);
#endif

  int n3[3];
  field_src.size (n3,n3+1,n3+2);

  for (size_t i_f=0; i_f < field_list_src.size(); i_f++) {

    size_t index_src = field_list_src[i_f];
    size_t index_dst = field_list_dst[i_f];
    CHECK_COARSE(field_src,index_src);

    int m3[3],g3[3],c3[3];

    field_src.dimensions (index_src,m3,m3+1,m3+2);
    field_src.ghost_depth(index_src,g3,g3+1,g3+2);
    field_src.centering  (index_src,c3,c3+1,c3+2);

    const bool accumulate = refresh_->accumulate(i_f);

    Box box (rank_,n3,g3);
    set_box_(&box);
    box.set_centering(c3);

    box_adjust_accumulate_(&box,accumulate,g3);

    bool lpad;
    int is3[3], ns3[3];
    int id3[3], nd3[3];

    box.get_start_size
      (is3,ns3,BlockType::send,BlockType::send,lpad=true);
    box.get_start_size
      (id3,nd3,BlockType::receive,BlockType::receive,lpad=false);

    // Adjust loop limits if accumulating to include ghost zones
    // on neighbor axes

    precision_type precision = field_src.precision(index_src);

    char * values_src = field_src.values(index_src);
    char * values_dst = field_dst.values(index_dst);

    // scale by density if needed to convert to conservative form
    mul_by_density_(field_src,index_src,is3,ns3,m3);

    if (face_type_ > 0) {

      // Prolong field

      bool need_padding = (g3[0]%2==1) || (g3[1]%2==1) || (g3[2]%2==1);

      ASSERT("FieldFace::face_to_face()",
             "Odd ghost zones not implemented yet: prolong needs padding",
             (! need_padding) );

      // adjust for full-block interpolation to child
      prolong()->apply (precision,
                        values_dst,m3,id3, nd3,
                        values_src,m3,is3, ns3,
                        accumulate);

    } else if (face_type_ < 0) {

      // Restrict field

      restrict()->apply (precision,
                         values_dst,m3,id3, nd3,
                         values_src,m3,is3, ns3,
                         accumulate);

    } else {

      // Copy faces to ghosts

      union { float * fs4; double * fs8; long double * fs16; };
      union { float * fd4; double * fd8; long double * fd16; };
      fs4 = (float *) values_src;
      fd4 = (float *) values_dst;

      // Copy field to array

      if (precision == precision_single) {
	copy_ ( fd4, m3,nd3,id3,fs4, m3, ns3,is3,accumulate);
      } else if (precision == precision_double) {
	copy_ ( fd8, m3,nd3,id3,fs8, m3, ns3,is3,accumulate);
      } else if (precision == precision_quadruple) {
	copy_ ( fd16,m3,nd3,id3,fs16,m3,ns3,is3,accumulate);
      } else {
	ERROR("FieldFace::face_to_face()", "Unsupported precision");
      }
    }
    // unscale by density if needed to convert back from conservative form
    div_by_density_(field_src,index_src,is3,ns3,m3);
    div_by_density_(field_dst,index_dst,id3,nd3,m3);

  }

  // Interpolate fields in time if needed when adaptive time-stepping
  time_interpolate_(field_dst,field_list_dst);

#ifdef CONFIG_SMP_MODE
  CmiUnlock(field_face_node_lock);
  PERF_SMP_STOP(perf_rindex_smp_field_face);
#endif  
}

//----------------------------------------------------------------------

int FieldFace::num_bytes_array(Field field) throw()
{
  int array_size = 0;

  auto field_list_src = refresh_->field_list_src(level_,face_type_);
  auto field_list_dst = refresh_->field_list_dst(level_,face_type_);

  for (size_t i_f=0; i_f < field_list_src.size(); i_f++) {

    size_t index_field = field_list_src[i_f];

    CHECK_COARSE(field,index_field);

    precision_type precision = field.precision(index_field);
    int bytes_per_element = cello::sizeof_precision (precision);

    int m3[3],n3[3],g3[3],c3[3];

    field.dimensions (index_field,m3,m3+1,m3+2);
    field.size                   (n3,n3+1,n3+2);
    field.ghost_depth(index_field,g3,g3+1,g3+2);
    field.centering  (index_field,c3,c3+1,c3+2);

    const bool accumulate = refresh_->accumulate(i_f);

    Box box (rank_,n3,g3);
    set_box_(&box);
    box.set_centering(c3);

    box_adjust_accumulate_(&box,accumulate,g3);

    bool lpad;
    int i3[3];
    box.get_start_size(i3,n3,BlockType::send,BlockType::send,lpad=true);

    array_size += n3[0]*n3[1]*n3[2]*bytes_per_element;

  }

  ASSERT("FieldFace::num_bytes_array()",
	 "array_size must be > 0, maybe field_list.size() is 0?",
	 array_size);

  return array_size;

}

//----------------------------------------------------------------------

int FieldFace::data_size () const
{
  int count = 0;

  SIZE_ARRAY_TYPE(count,int,face_,3);
  SIZE_ARRAY_TYPE(count,int,ghost_,3);
  SIZE_ARRAY_TYPE(count,int,child_,3);

  SIZE_SCALAR_TYPE(count,int,rank_);
  SIZE_SCALAR_TYPE(count,int,level_);
  SIZE_SCALAR_TYPE(count,int,face_type_);

  count += refresh_->data_size(); // refresh_

  return count;

}

//----------------------------------------------------------------------

char * FieldFace::save_data (char * buffer) const
{
  char * p = buffer;

  SAVE_ARRAY_TYPE(p,int,face_,3);
  SAVE_ARRAY_TYPE(p,int,ghost_,3);
  SAVE_ARRAY_TYPE(p,int,child_,3);

  SAVE_SCALAR_TYPE(p,int,rank_);
  SAVE_SCALAR_TYPE(p,int,level_);
  SAVE_SCALAR_TYPE(p,int,face_type_);

  p = refresh_->save_data(p);

  ASSERT2("FieldFace::save_data()",
          "Buffer has size %ld but expecting size %d",
          (p-buffer),data_size(),
          ((p-buffer) == data_size()));

  return p;
}

//----------------------------------------------------------------------

char * FieldFace::load_data (char * buffer)
{
  char * p = buffer;

  LOAD_ARRAY_TYPE(p,int,face_,3);
  LOAD_ARRAY_TYPE(p,int,ghost_,3);
  LOAD_ARRAY_TYPE(p,int,child_,3);

  LOAD_SCALAR_TYPE(p,int,rank_);
  LOAD_SCALAR_TYPE(p,int,level_);
  LOAD_SCALAR_TYPE(p,int,face_type_);

  set_refresh(new Refresh,true);

  p = refresh_->load_data(p);

  ASSERT2("FieldFace::save_data()",
          "Buffer has size %ld but expecting size %d",
          (p-buffer),data_size(),
          ((p-buffer) == data_size()));

  return p;
}

//======================================================================

template<class T>
size_t FieldFace::load_
( T * array_face, const T * field_face, 
  int m3[3], int n3[3],int i3[3], bool accumulate ) throw()
{
  // NOTE: don't check accumulate since loading array; accumulate
  // is handled in corresponding store_() at the receiving end
  // add values

  for (int iz=0; iz <n3[2]; iz++)  {
    int kz = iz+i3[2];
    for (int iy=0; iy < n3[1]; iy++) {
      int ky = iy+i3[1];
      for (int ix=0; ix < n3[0]; ix++) {
	int kx = ix+i3[0];
	int index_array = ix +   n3[0]*(iy +   n3[1] * iz);
	int index_field = kx + m3[0]*(ky + m3[1] * kz);
	array_face[index_array] = field_face[index_field];
      }
    }
  }

  return (sizeof(T) * n3[0] * n3[1] * n3[2]);

}

//----------------------------------------------------------------------

template<class T> size_t FieldFace::store_
( T * ghost, const T * array,
  int m3[3], int n3[3],int i3[3], bool accumulate) throw()
{

#ifdef FORTRAN_STORE
  const bool use_fortran_store = true;
#else  
  const bool use_fortran_store = false;
#endif

  // This is to get around a bug on SDSC Comet where this function
  // crashes with -O3 (See bugzilla report #90)

  union {
    float *       ghost_4;
    double *      ghost_8;
    long double * ghost_16;
  };
  union {
    float *       array_4;
    double *      array_8;
    long double * array_16;
  };

  ghost_4 = (float *) ghost;
  array_4 = (float *) array;
  
  int im = i3[0] + m3[0]*(i3[1] + m3[1]*i3[2]);

  int iaccumulate = accumulate ? 1 : 0;

  if (use_fortran_store &&
      (sizeof(T) != sizeof(long double)) ) {

    if (sizeof(T)==sizeof(float)) {
      FORTRAN_NAME(field_face_store_4)(ghost_4 + im,   array_4, m3,n3,
                                       &iaccumulate);
    } else if (sizeof(T)==sizeof(double)) {
      FORTRAN_NAME(field_face_store_8)(ghost_8 + im,   array_8, m3,n3,
                                       &iaccumulate);
    } else if (sizeof(T)==sizeof(long double)) {
      FORTRAN_NAME(field_face_store_16)(ghost_16 + im, array_16, m3,n3,
                                        &iaccumulate);
    } else {
      ERROR1 ("FieldFace::store_()",
              "unknown float precision sizeof(T) = %lu\n",sizeof(T));
    }
  } else {

    if (accumulate) {
      // add values
      for (int iz=0; iz <n3[2]; iz++)  {
        int kz = iz+i3[2];
        for (int iy=0; iy < n3[1]; iy++) {
          int ky = iy+i3[1];
          for (int ix=0; ix < n3[0]; ix++) {
            int kx = ix+i3[0];
            int index_array = ix + n3[0]*(iy + n3[1] * iz);
            int index_field = kx + m3[0]*(ky + m3[1] * kz);
            ghost[index_field] += array[index_array];
          }
        }
      }
    } else {
      // copy values
      for (int iz=0; iz <n3[2]; iz++)  {
        int kz = iz+i3[2];
        for (int iy=0; iy < n3[1]; iy++) {
          int ky = iy+i3[1];
          for (int ix=0; ix < n3[0]; ix++) {
            int kx = ix+i3[0];
            int index_array = ix + n3[0]*(iy + n3[1] * iz);
            int index_field = kx + m3[0]*(ky + m3[1] * kz);
            ghost[index_field] = array[index_array];
          }
        }
      }
    }
  }

  return (sizeof(T) * n3[0] * n3[1] * n3[2]);

}

//----------------------------------------------------------------------

template<class T> void FieldFace::copy_
( T       * vd, int md3[3],int nd3[3],int id3[3],
  const T * vs, int ms3[3],int ns3[3],int is3[3],
  bool accumulate) throw()
{
  const int is0 = is3[0] + ms3[0]*(is3[1] + ms3[1]*is3[2]);
  const int id0 = id3[0] + md3[0]*(id3[1] + md3[1]*id3[2]);
  T * vd0 = vd + id0;
  const T * vs0 = vs + is0;
  const int msx = ms3[0];
  const int msy = ms3[1];
  const int mdx = md3[0];
  const int mdy = md3[1];
  if (accumulate) {
    for (int iz=0; iz <ns3[2]; iz++)  {
      for (int iy=0; iy < ns3[1]; iy++) {
	for (int ix=0; ix < ns3[0]; ix++) {
	  int i_src = ix + msx*(iy + msy*iz);
	  int i_dst = ix + mdx*(iy + mdy*iz);
	  vd0[i_dst] += vs0[i_src];
	}
      }
    }
  } else {
    for (int iz=0; iz <ns3[2]; iz++)  {
      for (int iy=0; iy < ns3[1]; iy++) {
	for (int ix=0; ix < ns3[0]; ix++) {
	  int i_src = ix + msx*(iy + msy*iz);
	  int i_dst = ix + mdx*(iy + mdy*iz);
	  vd0[i_dst] = vs0[i_src];
	}
      }
    }
  }
}

//----------------------------------------------------------------------

void FieldFace::print(const char * message)
{
  CkPrintf (" FieldFace   %s %p\n",message,(void*)this);
  CkPrintf ("    face_    %d %d %d\n",face_[0],face_[1],face_[2]);
  CkPrintf ("    ghost_   %d %d %d\n",ghost_[0],ghost_[1],ghost_[2]);
  CkPrintf ("    child_   %d %d %d\n",child_[0],child_[1],child_[2]);
  CkPrintf ("    face_type_ %d\n",face_type_);
  if (refresh_) refresh_->print();
}

//----------------------------------------------------------------------

void FieldFace::mul_by_density_
(Field field, int index_field,
 const int i3[3], const int n3[3], const int m3[3])
{
  if (field.is_temporary(index_field)) return;

  Grouping * groups = cello::field_groups();
  const std::string field_name = field.field_name(index_field);

  const bool scale_by_density =
    (face_type_ != 0) &&
    groups->is_in (field_name,"make_field_conservative");

  if (scale_by_density) {

    const int index_density = field.field_id ("density");

    char * field_density = field.values(index_density);
    char * field_face =    field.values(index_field);

    union { float * d4; double * d8; long double * d16; };
    union { float * f4; double * f8;long double * f16;  };
    d4 = (float *) field_density;
    f4 = (float *) field_face;

    const precision_type precision = field.precision(index_field);

    if (precision == precision_single) {
      for (int iz=i3[2]; iz<i3[2]+n3[2]; iz++) {
        for (int iy=i3[1]; iy<i3[1]+n3[1]; iy++) {
          for (int ix=i3[0]; ix<i3[0]+n3[0]; ix++) {
            const int i=ix + m3[0]*(iy + m3[1]*iz);
            f4[i] *= d4[i];
          }
        }
      }
    } else if (precision == precision_double) {
      for (int iz=i3[2]; iz<i3[2]+n3[2]; iz++) {
        for (int iy=i3[1]; iy<i3[1]+n3[1]; iy++) {
          for (int ix=i3[0]; ix<i3[0]+n3[0]; ix++) {
            const int i=ix + m3[0]*(iy + m3[1]*iz);
            f8[i] *= d8[i];
          }
        }
      }
    } else if (precision == precision_quadruple) {
      for (int iz=i3[2]; iz<i3[2]+n3[2]; iz++) {
        for (int iy=i3[1]; iy<i3[1]+n3[1]; iy++) {
          for (int ix=i3[0]; ix<i3[0]+n3[0]; ix++) {
            const int i=ix + m3[0]*(iy + m3[1]*iz);
            f16[i] *= d16[i];
          }
        }
      }
    } else {
      ERROR("FieldFace::mul_by_density_()", "Unsupported precision");
    }
  }
}

//----------------------------------------------------------------------

void FieldFace::div_by_density_
(Field field, int index_field,
 const int i3[3], const int n3[3], const int m3[3])
{

  if (field.is_temporary(index_field)) return;

  precision_type precision = field.precision(index_field);

  char *  field_face = field.values (index_field);

  Grouping * groups = cello::field_groups();

  void * field_density = field.values ("density");

  const std::string field_name = field.field_name(index_field);

  const bool scale_by_density =
    (face_type_ != 0) &&
    groups->is_in (field_name,"make_field_conservative");
  if (scale_by_density) {
    union { float * d4; double * d8; long double * d16; };
    union { float * f4; double * f8;long double * f16;  };
    d4 = (float *) field_density;
    f4 = (float *) field_face;

    if (precision == precision_single) {
      for (int iz=i3[2]; iz<i3[2]+n3[2]; iz++) {
        for (int iy=i3[1]; iy<i3[1]+n3[1]; iy++) {
          for (int ix=i3[0]; ix<i3[0]+n3[0]; ix++) {
            const int i=ix + m3[0]*(iy + m3[1]*iz);
            f4[i] /= d4[i];
          }
        }
      }
    } else if (precision == precision_double) {
      for (int iz=i3[2]; iz<i3[2]+n3[2]; iz++) {
        for (int iy=i3[1]; iy<i3[1]+n3[1]; iy++) {
          for (int ix=i3[0]; ix<i3[0]+n3[0]; ix++) {
            const int i=ix + m3[0]*(iy + m3[1]*iz);
            f8[i] /= d8[i];
          }
        }
      }
    } else if (precision == precision_quadruple) {
      for (int iz=i3[2]; iz<i3[2]+n3[2]; iz++) {
        for (int iy=i3[1]; iy<i3[1]+n3[1]; iy++) {
          for (int ix=i3[0]; ix<i3[0]+n3[0]; ix++) {
            const int i=ix + m3[0]*(iy + m3[1]*iz);
            f16[i] /= d16[i];
          }
        }
      }
    } else {
      ERROR("FieldFace::div_by_density_()", "Unsupported precision");
    }
  }
}

//----------------------------------------------------------------------

void FieldFace::set_box_(Box * box)
{
  box->set_block(BoxType_receive,face_type_,face_,child_);

  Prolong * prolong = this->prolong();
  int pad = prolong ? refresh_->coarse_padding(prolong) : 0;
  if (face_type_ <= 0) pad = 0;

  box->set_padding(pad);

  box->compute_region();
}

//----------------------------------------------------------------------

void FieldFace::box_adjust_accumulate_ (Box * box, int accumulate, int g3[3])
{

  int gs3[3];

  if (accumulate) {

    gs3[0] = (face_[0]) ? g3[0] : 0;
    gs3[1] = (face_[1]) ? g3[1] : 0;
    gs3[2] = (face_[2]) ? g3[2] : 0;

  } else {

    gs3[0] = (ghost_[0] && ! face_[0]) ? g3[0] : 0;
    gs3[1] = (ghost_[1] && ! face_[1]) ? g3[1] : 0;
    gs3[2] = (ghost_[2] && ! face_[2]) ? g3[2] : 0;

  }

  box->set_send_ghosts(gs3);
  box->compute_block_start(BoxType_receive);
  box->compute_region();
}

//----------------------------------------------------------------------

void FieldFace::time_interpolate_
(Field field,  const std::vector<int> & field_list,
 bool invert)
{
  if (! include_history_()) return;

  const double t_curr = cello::simulation()->state()->time_curr(level_ + 1);
  const double t_prev = cello::simulation()->state()->time_prev(level_);
  const double t_next = cello::simulation()->state()->time_curr(level_);

  const double c_next = (t_curr - t_prev) / (t_next - t_prev);
  const double c_prev = (1.0 - c_next);

  int n3[3];
  field.size (n3,n3+1,n3+2);
  if (t_next != t_prev) {

    for (size_t i_f=0; i_f < field_list.size(); i_f++) {
      const int id_curr = field_list[i_f];
      if (field.history_age(id_curr) == 0) {
        const int id_prev = field.history_id(id_curr,1);
        int m3[3],g3[3],c3[3];

        field.dimensions (id_curr,m3,m3+1,m3+2);
        field.ghost_depth(id_curr,g3,g3+1,g3+2);
        field.centering  (id_curr,c3,c3+1,c3+2);

        const bool accumulate = refresh_->accumulate(i_f);

        if (invert) invert_face();
        Box box (rank_,n3,g3);
        set_box_(&box);
        box.set_centering(c3);
        if (invert) invert_face();

        box_adjust_accumulate_(&box,accumulate,g3);

        bool lpad;
        int i3_f[3], n3_f[3];

        box.get_start_size
          (i3_f,n3_f,BlockType::receive,BlockType::receive,lpad=false);

        const double t_next = cello::simulation()->state()->time(level_);

        // curr initially is coarse next
        // prev is coarse prev
        // curr = curr + prev
        cello_float * field_next = (cello_float *) field.values(id_curr);
        cello_float * field_curr = (cello_float *) field.values(id_curr);
        cello_float * field_prev = (cello_float *) field.values(id_prev);

        for (int iz=i3_f[2]; iz<i3_f[2]+n3_f[2]; iz++) {
          for (int iy=i3_f[1]; iy<i3_f[1]+n3_f[1]; iy++) {
            for (int ix=i3_f[0]; ix<i3_f[0]+n3_f[0]; ix++) {
              int i=ix + m3[0]*(iy + m3[1]*iz);
              field_curr[i] = c_prev*field_prev[i] + c_next*field_next[i];
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------

bool FieldFace::include_history_() const
{
  const bool l_adapt = refresh_->adaptive_timestep();
  const bool l_face = (face_type_ == +1);

  return (l_adapt && l_face);
}
