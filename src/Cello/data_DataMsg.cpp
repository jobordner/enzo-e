// See LICENSE_CELLO file for license and copyright information

/// @file     data_DataMsg.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-12-22
/// @brief    

#include "data.hpp"

int64_t DataMsg::counter[CONFIG_NODE_SIZE] = {0};

#define CHECK

//----------------------------------------------------------------------

DataMsg::DataMsg() 
  : field_face_(nullptr),
    field_face_delete_   (false),
    field_data_u_(nullptr),
    field_data_delete_   (false),
    particle_data_(nullptr),
    particle_data_delete_(false),
    face_fluxes_list_(),
    face_fluxes_delete_(),
    // coarse_field_buffer_(),
    // coarse_field_list_src_(),
    // coarse_field_list_dst_(),
    coarse_data_(),
    scalar_data_long_double_(),
    scalar_data_double_(),
    scalar_data_int_(),
    scalar_data_long_long_(),
    scalar_data_sync_(),
    scalar_data_index_()
{
  ++counter[cello::index_static()];
}

//----------------------------------------------------------------------

DataMsg::~DataMsg()
{
  --counter[cello::index_static()];

  if (field_face_delete_) {
    delete field_face_;
    field_face_ = nullptr;
  }
  if (field_data_delete_) {
    delete field_data_u_;
    field_data_u_ = nullptr;
  }
  if (particle_data_delete_) {
    delete particle_data_;
    particle_data_ = nullptr;
  }

  for (size_t i=0; i<face_fluxes_list_.size(); i++) {
    if (face_fluxes_delete_[i]) {
      delete face_fluxes_list_[i];
      face_fluxes_list_[i] = nullptr;
    }
  }
  face_fluxes_list_.clear();
  face_fluxes_delete_.clear();
}

//----------------------------------------------------------------------

void DataMsg::add_coarse_array
  (Field field,
   int iam3[3], int iap3[3],
   int ifms3[3], int ifps3[3],
   int ifmr3[3], int ifpr3[3],
   const std::vector<int> & field_list_src,
   const std::vector<int> & field_list_dst)
{
  const int i = coarse_data_.size();
  DataMsgCoarse data
    (field,
     iam3, iap3,
     ifms3, ifps3,
     ifmr3, ifpr3,
     field_list_src,
     field_list_dst);

  coarse_data_.push_back(data);

}

//----------------------------------------------------------------------

void DataMsg::set_scalars ( Data * data)
{
  scalar_data_long_double_ = *data->scalar_data_long_double();
  scalar_data_double_ = *data->scalar_data_double();
  scalar_data_int_ = *data->scalar_data_int();
  scalar_data_long_long_ = *data->scalar_data_long_long();
  scalar_data_sync_ = *data->scalar_data_sync();
  scalar_data_index_ = *data->scalar_data_index();
}

//----------------------------------------------------------------------

void DataMsg::update_scalars ( Data * data)
{
  *data->scalar_data_long_double() = scalar_data_long_double_;
  *data->scalar_data_double() = scalar_data_double_;
  *data->scalar_data_int() = scalar_data_int_;
  *data->scalar_data_long_long() = scalar_data_long_long_;
  *data->scalar_data_sync() = scalar_data_sync_;
  *data->scalar_data_index() = scalar_data_index_;
}

//----------------------------------------------------------------------

void DataMsg::get_num_data_ (int & n_ff, int & n_fa, int & n_pd, int & n_fd) const
{
  Field field (cello::field_descr(), field_data_u_);
  FieldFace    * ff = field_face_;
  ParticleData * pd = particle_data_;
  auto & fd = face_fluxes_list_;

  n_ff = (ff) ? ff->data_size() : 0;
  n_fa = (ff) ? ff->num_bytes_array(field) : 0;
  n_pd = (pd) ? pd->data_size(cello::particle_descr()) : 0;
  n_fd = fd.size();
}

//----------------------------------------------------------------------

int DataMsg::data_size () const
{
  //--------------------------------------------------
  //  1. determine buffer size (must be consistent with #3)
  //--------------------------------------------------

  int n_ff,n_fa,n_pd,n_fd;
  get_num_data_ (n_ff,n_fa,n_pd,n_fd);

  int size = 0;

  SIZE_SCALAR_TYPE(size,int,n_ff);
  SIZE_SCALAR_TYPE(size,int,n_fa);
  SIZE_SCALAR_TYPE(size,int,n_pd);
  SIZE_SCALAR_TYPE(size,int,n_fd);

  size += n_ff;
  size += n_fa;
  size += n_pd;

  if (n_fd > 0) {
    SIZE_VECTOR_TYPE(size,char,face_fluxes_delete_);
    SIZE_VECTOR_OBJECT_PTR_TYPE(size,FaceFluxes,face_fluxes_list_);
  }


  SIZE_VECTOR_OBJECT_TYPE(size,DataMsgCoarse,coarse_data_);

  SIZE_OBJECT_TYPE(size,scalar_data_long_double_);
  SIZE_OBJECT_TYPE(size,scalar_data_double_);
  SIZE_OBJECT_TYPE(size,scalar_data_int_);
  SIZE_OBJECT_TYPE(size,scalar_data_long_long_);
  SIZE_OBJECT_TYPE(size,scalar_data_sync_);
  SIZE_OBJECT_TYPE(size,scalar_data_index_);

  return size;
}

//----------------------------------------------------------------------

char * DataMsg::save_data (char * buffer) const
{
  char * pc = buffer;

  int n_ff,n_fa,n_pd,n_fd;
  get_num_data_ (n_ff,n_fa,n_pd,n_fd);

  SAVE_SCALAR_TYPE(pc,int,n_ff);
  SAVE_SCALAR_TYPE(pc,int,n_fa);
  SAVE_SCALAR_TYPE(pc,int,n_pd);
  SAVE_SCALAR_TYPE(pc,int,n_fd);

  // save field face
  if (n_ff > 0) {
    pc = field_face_->save_data (pc);
  }
  // save field array
  Field field (cello::field_descr(), field_data_u_);
  if (n_ff > 0 && n_fa > 0) {
    field_face_->face_to_array(field,pc);
    pc += n_fa;
  }
  // save particle data
  if (n_pd > 0) {
    pc = particle_data_->save_data(cello::particle_descr(),pc);
  }
  // save fluxes
  if (n_fd > 0) {
    SAVE_VECTOR_TYPE(pc,char,face_fluxes_delete_);
    SAVE_VECTOR_OBJECT_PTR_TYPE(pc,FaceFluxes,face_fluxes_list_);
  }

  // Coarse face array for interpolation

  SAVE_VECTOR_OBJECT_TYPE(pc,DataMsgCoarse,coarse_data_);

  SAVE_OBJECT_TYPE(pc,scalar_data_long_double_);
  SAVE_OBJECT_TYPE(pc,scalar_data_double_);
  SAVE_OBJECT_TYPE(pc,scalar_data_int_);
  SAVE_OBJECT_TYPE(pc,scalar_data_long_long_);
  SAVE_OBJECT_TYPE(pc,scalar_data_sync_);
  SAVE_OBJECT_TYPE(pc,scalar_data_index_);

  ASSERT2 ("DataMsg::save_data()",
  	   "Expecting buffer size %d actual size %d",
  	   data_size(),(pc-buffer),
  	   (data_size() == (pc-buffer)));
  
  // return first byte after filled buffer
  return pc;
}

//----------------------------------------------------------------------

char * DataMsg::load_data (char * buffer)
{
  // 2. De-serialize message data from input buffer into the allocated
  // message (must be consistent with pack())

  char * pc = buffer;

  int n_ff,n_fa,n_pd,n_fd;

  LOAD_SCALAR_TYPE(pc,int,n_ff);
  LOAD_SCALAR_TYPE(pc,int,n_fa);
  LOAD_SCALAR_TYPE(pc,int,n_pd);
  LOAD_SCALAR_TYPE(pc,int,n_fd);

  // load field face
  if (n_ff > 0) {
    field_face_delete_ = true;
    field_face_ = new FieldFace;
    pc = field_face_->load_data (pc);
  } else {
    field_face_ = nullptr;
  }

  // load field array
  if (n_fa > 0) {
    field_array_u_ = pc;
    pc += n_fa;
  } else {
    field_array_u_ = nullptr;
  }

  // load particle data
  if (n_pd > 0) {
    particle_data_delete_ = true;
    ParticleData * pd = particle_data_ = new ParticleData;
    pd->allocate(cello::particle_descr());
    pc = pd->load_data(cello::particle_descr(),pc);
  } else {
    particle_data_ = nullptr;
  }
  // load flux data
  if (n_fd > 0) {

    LOAD_VECTOR_TYPE(pc,char,face_fluxes_delete_);
    LOAD_VECTOR_OBJECT_PTR_TYPE(pc,FaceFluxes,face_fluxes_list_);

    for (int i=0; i<n_fd; i++) {
      face_fluxes_delete_[i] = true;
    }
  }

  LOAD_VECTOR_OBJECT_TYPE(pc,DataMsgCoarse,coarse_data_);
 
  LOAD_OBJECT_TYPE(pc,scalar_data_long_double_);
  LOAD_OBJECT_TYPE(pc,scalar_data_double_);
  LOAD_OBJECT_TYPE(pc,scalar_data_int_);
  LOAD_OBJECT_TYPE(pc,scalar_data_long_long_);
  LOAD_OBJECT_TYPE(pc,scalar_data_sync_);
  LOAD_OBJECT_TYPE(pc,scalar_data_index_);

  return pc;
}

//----------------------------------------------------------------------

void DataMsg::update (Data * data, bool is_local, bool is_kept)
{
  ParticleData * pd = particle_data_;
  FieldFace    * ff = field_face_;
  char         * fa = field_array_u_;

  // Update particles

  if (pd != nullptr) {

    // Insert new particles 
    Particle particle = data->particle();

    int count = 0;
    for (int it=0; it<particle.num_types(); it++) {
      count += particle.gather (it, 1, &pd);
    }
    if (is_kept)
      cello::simulation()->data_insert_particles(count);

  }

  // Update fields

  if (ff != nullptr && fa != nullptr) {

    Field field_dst = data->field();

    if (is_local) {

      Field field_src(cello::field_descr(),field_data_u_);

      ff->face_to_face(field_src, field_dst);

    } else { // ! is_local

      // invert face since incoming not outgoing

      ff->invert_face();

      ff->array_to_face(fa,field_dst);

    }
  }

  if (ff != nullptr) {
    delete field_face_;
    field_face_ = nullptr;
  }

  // Update fluxes

  FluxData * flux_data = data->flux_data();
  if (face_fluxes_list_.size() > 0) {
    for (size_t i=0; i<face_fluxes_list_.size(); i++) {
      FaceFluxes * face_fluxes = face_fluxes_list_[i];
      Face face = face_fluxes->face();
      flux_data->sum_neighbor_fluxes
        (face_fluxes,face.axis(), 1 - face.face(), i);
      if (face_fluxes_delete_[i]) {
        delete face_fluxes;
        face_fluxes_list_[i] = nullptr;
      }
    }
  }
  for (auto & coarse_data : coarse_data_) {
    coarse_data.update (data);
  }
 
}

//----------------------------------------------------------------------

void DataMsg::print (std::string message_str, FILE * fp_in) const
{
  const char * message = message_str.c_str();
  
  FILE * fp = fp_in ? fp_in : stdout;
  
  fprintf (fp,"%s DATA_MSG %p\n",message,(void*)this);
  fprintf (fp,"%s DATA_MSG field_face_    = %p\n",
            message,(void*)field_face_);
  fprintf (fp,"%s DATA_MSG field_data_u_    = %p\n",
            message,(void*)field_data_u_);
  fprintf (fp,"%s DATA_MSG particle_data_ = %p\n",
            message,(void*)particle_data_);
  if (particle_data_) {
    fprintf (fp,"%s DATA_MSG num-particles = %d\n",
              message,particle_data_->num_particles(cello::particle_descr()));
  }
  fprintf (fp,"%s DATA_MSG particle_data_delete_ = %d\n",
            message,particle_data_delete_?1:0);
  fprintf (fp,"%s DATA_MSG |face_fluxes_list_| = %lu\n",
            message,face_fluxes_list_.size());
  fprintf (fp,"%s DATA_MSG |face_fluxes_delete_| = %lu\n",
            message,face_fluxes_delete_.size());
  fprintf (fp,"%s DATA_MSG field_face_delete_ = %d\n",
            message,field_face_delete_?1:0);
  fprintf (fp,"%s DATA_MSG field_data_delete_ = %d\n",
            message,field_data_delete_?1:0);
  for (auto & coarse_data : coarse_data_) {
    coarse_data.print(message_str,fp_in);
  }
}
