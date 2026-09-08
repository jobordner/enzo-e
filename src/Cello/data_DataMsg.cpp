// See LICENSE_CELLO file for license and copyright information

/// @file     data_DataMsg.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2015-12-22
/// @brief    

#include "data.hpp"

//----------------------------------------------------------------------
int64_t DataMsg::counter[CONFIG_NODE_SIZE] = {0};
//----------------------------------------------------------------------

//----------------------------------------------------------------------
#define ENABLE_SMP_NODE_LOCK
//----------------------------------------------------------------------

#if defined(CONFIG_SMP_MODE) and defined(ENABLE_SMP_NODE_LOCK)
static CmiNodeLock node_lock_data_msg;
void mutex_init_data_msg()
{ node_lock_data_msg = CmiCreateLock(); }
#   define SMP_NODE_LOCK   CmiLock(node_lock_data_msg);
#   define SMP_NODE_UNLOCK CmiUnlock(node_lock_data_msg);
#else
void mutex_init_data_msg() { }
#   define SMP_NODE_LOCK   /* ... */
#   define SMP_NODE_UNLOCK /* ... */
#endif

#ifdef ENABLE_SMP_LOCK
#ifdef CONFIG_SMP_MODE
static CmiNodeLock node_lock_data_msg;
#endif
#endif

//----------------------------------------------------------------------

DataMsg::DataMsg() 
  : field_face_list_(),
    field_face_delete_(),
    field_array_list_(),
    field_data_(nullptr),
    field_data_delete_   (false),
    particle_data_(nullptr),
    particle_data_delete_(false),
    face_fluxes_list_(),
    face_fluxes_delete_(),
    coarse_data_(),
    scalar_data_long_double_(),
    scalar_data_double_(),
    scalar_data_int_(),
    scalar_data_long_long_(),
    scalar_data_sync_(),
    scalar_data_index_()
{
  ++counter[cello::index_static()];
  field_array_list_.clear();
  field_face_list_.clear();
  field_face_delete_.clear();
}

//----------------------------------------------------------------------

DataMsg::~DataMsg()
{
  --counter[cello::index_static()];

  int k=0;
  for ( auto fd : field_face_delete_) {
    if (fd) delete field_face_list_[k];
    ++k;
  }
  if (field_data_delete_) {
    delete field_data_;
    field_data_ = nullptr;
  }
  if (particle_data_delete_) {
    delete particle_data_;
    particle_data_ = nullptr;
  }

  k=0;
  for ( auto & ffd : face_fluxes_delete_ ) {
    if (ffd) delete face_fluxes_list_[k];
    ++k;
  }
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
  PERF_SMP_START(iperf_smp_data_msg);

  SMP_NODE_LOCK;

  *data->scalar_data_long_double() = scalar_data_long_double_;
  *data->scalar_data_double() = scalar_data_double_;
  *data->scalar_data_int() = scalar_data_int_;
  *data->scalar_data_long_long() = scalar_data_long_long_;
  *data->scalar_data_sync() = scalar_data_sync_;
  *data->scalar_data_index() = scalar_data_index_;

  SMP_NODE_UNLOCK;

  PERF_SMP_STOP(iperf_smp_data_msg);
}

//----------------------------------------------------------------------

int DataMsg::data_size () const
{
  //--------------------------------------------------
  //  1. determine buffer size (must be consistent with #3)
  //--------------------------------------------------

  int size = 0;

  int n_pd;
  SIZE_SCALAR_TYPE(size,int,n_pd);

  // sizes of field faces
  SIZE_VECTOR_OBJECT_PTR_TYPE(size,FieldFace,field_face_list_);

  // sizes of field arrays
  Field field (cello::field_descr(), field_data_);
  for (auto * ff : field_face_list_) {
    // size of array length
    int n_fa;
    SIZE_SCALAR_TYPE(size,int,n_fa);
    // size of array
    size += ff->num_bytes_array(field);
  }

  // size of particle data
  n_pd = (particle_data_) ?
    particle_data_->data_size(cello::particle_descr()) : 0;
  size += n_pd;

  SIZE_VECTOR_TYPE(size,char,face_fluxes_delete_);
  SIZE_VECTOR_OBJECT_PTR_TYPE(size,FaceFluxes,face_fluxes_list_);

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

  // save field face
  SAVE_VECTOR_OBJECT_PTR_TYPE(pc,FieldFace,field_face_list_);
  
  // save field array
  Field field (cello::field_descr(), field_data_);
  for (auto * ff : field_face_list_) {
    // save array size
    const int n_ff = ff->num_bytes_array(field);
    SAVE_SCALAR_TYPE(pc,int,n_ff);
    // save array
    ff->face_to_array(field,pc);
    pc += n_ff;
  }

  // save particle data
  int n_pd;
  n_pd = (particle_data_) ?
    particle_data_->data_size(cello::particle_descr()) : 0;
  SAVE_SCALAR_TYPE(pc,int,n_pd);

  if (n_pd > 0) {
    pc = particle_data_->save_data(cello::particle_descr(),pc);
  }

  // save fluxes
  SAVE_VECTOR_TYPE(pc,char,face_fluxes_delete_);
  SAVE_VECTOR_OBJECT_PTR_TYPE(pc,FaceFluxes,face_fluxes_list_);

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
  char * pc = buffer;

  // load field face
  LOAD_VECTOR_OBJECT_PTR_TYPE(pc,FieldFace,field_face_list_);

  for (size_t k=0; k<field_face_list_.size(); k++) {
    // load array size
    int n_ff;
    LOAD_SCALAR_TYPE(pc,int,n_ff);
    // load array
    field_face_delete_.push_back(true);
    field_array_list_.push_back(pc);
    pc += n_ff;
  }

  // load particle data
  int n_pd;
  LOAD_SCALAR_TYPE(pc,int,n_pd);

  if (n_pd > 0) {
    particle_data_delete_ = true;
    ParticleData * pd = particle_data_ = new ParticleData;
    pd->allocate(cello::particle_descr());
    pc = pd->load_data(cello::particle_descr(),pc);
  } else {
    particle_data_ = nullptr;
  }

  LOAD_VECTOR_TYPE(pc,char,face_fluxes_delete_);
  LOAD_VECTOR_OBJECT_PTR_TYPE(pc,FaceFluxes,face_fluxes_list_);
  for (auto & ffd : face_fluxes_delete_) {
    ffd = true;
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
  PERF_SMP_START(iperf_smp_data_msg);

  SMP_NODE_LOCK;

  ParticleData * pd = particle_data_;

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

  for (size_t k=0; k<field_face_list_.size(); k++) {

    FieldFace * ff = field_face_list_[k];
    Field field_dst = data->field();

    if (is_local) {

      Field field_src(cello::field_descr(),field_data_);

      ff->face_to_face(field_src, field_dst);

    } else { // ! is_local

      // invert face since incoming not outgoing
      ff->invert_face();

      char * fa = field_array_list_[k];
      ff->array_to_face(fa,field_dst);

    }
  }

  for ( auto & ff : field_face_list_) {
    delete ff;
    ff = nullptr;
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

  SMP_NODE_UNLOCK;

  PERF_SMP_STOP(iperf_smp_data_msg);
}

//----------------------------------------------------------------------

void DataMsg::print (std::string message_str, FILE * fp_in) const
{
  const char * message = message_str.c_str();
  
  FILE * fp = fp_in ? fp_in : stdout;
  
  fprintf (fp,"%s DATA_MSG %p\n",message,(void*)this);
  for ( auto * ff : field_face_list_) {
    fprintf (fp,"%s DATA_MSG field_face_    = %p\n",
             message,(void*)ff);
  }
  for ( auto fd : field_face_delete_) {
    fprintf (fp,"%s DATA_MSG field_face_delete_ = %d\n",
             message,fd?1:0);
  }
  fprintf (fp,"%s DATA_MSG field_data_    = %p\n",
           message,(void*)field_data_);
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
  fprintf (fp,"%s DATA_MSG field_data_delete_ = %d\n",
           message,field_data_delete_?1:0);
  for (auto & coarse_data : coarse_data_) {
    coarse_data.print(message_str,fp_in);
  }
}
