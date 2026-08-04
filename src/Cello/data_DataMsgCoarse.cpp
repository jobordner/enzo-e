// See LICENSE_CELLO file for license and copyright information

/// @file     data_DataMsgCoarse.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2026-07-27
/// @brief

#include "data.hpp"

#define CHECK

//======================================================================

DataMsgCoarse::DataMsgCoarse
  (Field field,
   int iam3[3],int iap3[3],
   int ifms3[3],int ifps3[3],
   int ifmr3[3],int ifpr3[3],
   const std::vector<int> & field_list_src,
   const std::vector<int> & field_list_dst)
{

  for (int i=0; i<3; i++) {
    iam3_[i] = iam3[i];
    iap3_[i] = iap3[i];
    ifms3_[i] = ifms3[i];
    ifps3_[i] = ifps3[i];
    ifmr3_[i] = ifmr3[i];
    ifpr3_[i] = ifpr3[i];
  }
  field_list_src_ = field_list_src;
  field_list_dst_ = field_list_dst;

  const int nf3[3] = {(ifps3[0] - ifms3[0]),
                      (ifps3[1] - ifms3[1]),
                      (ifps3[2] - ifms3[2])};
  const int na3[3] = {(iap3[0] - iam3[0]),
                      (iap3[1] - iam3[1]),
                      (iap3[2] - iam3[2])};
  const int nf = field_list_src.size();
  const int na = na3[0]*na3[1]*na3[2];

#ifdef CHECK
  ASSERT2 ("DataMsgCoarse::set_coarse_array",
           "field_list_src %d and field_list_dst %d must be the same size",
           field_list_src.size(),
           field_list_dst.size(),
           (field_list_src.size() == field_list_dst.size()));
#endif

  field_buffer_.resize(nf*na);
  std::fill(field_buffer_.begin(),field_buffer_.end(),0.0);

  for (int i_f=0; i_f<nf; i_f++) {

    const int index_field = field_list_src_[i_f];

    int mfx,mfy,mfz;
    field.dimensions(index_field,&mfx,&mfy,&mfz);
    cello_float * field_values = (cello_float *) field.values(index_field);
    const int if0 = ifms3[0] + mfx*(ifms3[1] + mfy*ifms3[2]);

    cello_float * field_array = field_buffer_.data() + i_f*na;

    // compute cell width ratio r
    const float r = nf3[0] / na3[0];
    const cello_float rr = (r==1) ? 1.0 : 1.0/cello::num_children();

#ifdef CHECK
    ASSERT1 ("DataMsgCoarse::set_coarse_array",
             "Field-to-coarse array axis ratio r=%g is not 1.0 or 2.0",
             r, (na3[0]*2==nf3[0] || na3[0]==nf3[0]));
#endif

    int rd = r;
    // compute constant factor v for r!=1

    for (int kz=0; kz<nf3[2]; kz++) {
      for (int ky=0; ky<nf3[1]; ky++) {
        for (int kx=0; kx<nf3[0]; kx++) {
          int ka = (kx/rd) + na3[0]*((ky/rd) + na3[1]*(kz/rd));
          int kf = if0 + kx + mfx*(ky + mfy*kz);
          field_array[ka] += rr*field_values[kf];
        }
      }
    }
  }
}

//----------------------------------------------------------------------

int DataMsgCoarse::data_size () const
{
  int size = 0;

  SIZE_VECTOR_TYPE(size,int,field_list_src_);
  SIZE_VECTOR_TYPE(size,int,field_list_dst_);
  SIZE_VECTOR_TYPE(size,cello_float,field_buffer_);

  SIZE_ARRAY_TYPE(size,int,iam3_,3);
  SIZE_ARRAY_TYPE(size,int,iap3_,3);
  SIZE_ARRAY_TYPE(size,int,ifms3_,3);
  SIZE_ARRAY_TYPE(size,int,ifps3_,3);
  SIZE_ARRAY_TYPE(size,int,ifmr3_,3);
  SIZE_ARRAY_TYPE(size,int,ifpr3_,3);

  return size;
}

//----------------------------------------------------------------------

char * DataMsgCoarse::save_data (char * buffer) const
{
  char * pc = buffer;

  SAVE_VECTOR_TYPE(pc,int,field_list_src_);
  SAVE_VECTOR_TYPE(pc,int,field_list_dst_);
  SAVE_VECTOR_TYPE(pc,cello_float,field_buffer_);

  SAVE_ARRAY_TYPE(pc,int,iam3_,3);
  SAVE_ARRAY_TYPE(pc,int,iap3_,3);
  SAVE_ARRAY_TYPE(pc,int,ifms3_,3);
  SAVE_ARRAY_TYPE(pc,int,ifps3_,3);
  SAVE_ARRAY_TYPE(pc,int,ifmr3_,3);
  SAVE_ARRAY_TYPE(pc,int,ifpr3_,3);

  ASSERT2 ("DataMsgCoarse::save_data()",
  	   "Expecting buffer size %d actual size %d",
  	   data_size(),(pc-buffer),
  	   (data_size() == (pc-buffer)));

  // return first byte after filled buffer
  return pc;
}

//----------------------------------------------------------------------

char * DataMsgCoarse::load_data (char * buffer)
{
  // 2. De-serialize message data from input buffer into the allocated
  // message (must be consistent with pack())

  char * pc = buffer;

  LOAD_VECTOR_TYPE(pc,int,field_list_src_);
  LOAD_VECTOR_TYPE(pc,int,field_list_dst_);
  LOAD_VECTOR_TYPE(pc,cello_float,field_buffer_);

  LOAD_ARRAY_TYPE(pc,int,iam3_,3);
  LOAD_ARRAY_TYPE(pc,int,iap3_,3);
  LOAD_ARRAY_TYPE(pc,int,ifms3_,3);
  LOAD_ARRAY_TYPE(pc,int,ifps3_,3);
  LOAD_ARRAY_TYPE(pc,int,ifmr3_,3);
  LOAD_ARRAY_TYPE(pc,int,ifpr3_,3);

  return pc;
}

//----------------------------------------------------------------------

void DataMsgCoarse::update (Data * data)
{
  // Updated coarse array
  const int na3[3] =
    {(iap3_[0] - iam3_[0]),
     (iap3_[1] - iam3_[1]),
     (iap3_[2] - iam3_[2])};

  const int na = na3[0]*na3[1]*na3[2];

  if (na>0) {

    const int na3[3] =
      {(iap3_[0] - iam3_[0]),
       (iap3_[1] - iam3_[1]),
       (iap3_[2] - iam3_[2])};

    const int nf = field_list_dst_.size();

    for (int i_f=0; i_f<nf; i_f++) {

      Field field = data->field();
      const int index_field = field_list_dst_[i_f];
      int m3_c[3];
      field.coarse_dimensions(index_field,m3_c,m3_c+1,m3_c+2);
      const int ic0 = iam3_[0] + m3_c[0]*(iam3_[1] + m3_c[1]*iam3_[2]);

      cello_float * field_array = field_buffer_.data() + i_f*na;
      cello_float * field_values =
        (cello_float *) field.coarse_values(index_field);

      for (int kz=0; kz<na3[2]; kz++) {
        for (int ky=0; ky<na3[1]; ky++) {
          for (int kx=0; kx<na3[0]; kx++) {
            const int ka = kx + na3[0]*(ky +  na3[1]*kz);
            const int kc = ic0 + kx + m3_c[0]*(ky + m3_c[1]*kz);
            field_values[kc] = field_array[ka];
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------

void DataMsgCoarse::print (std::string message_str, FILE * fp_in) const
{
  const char * message = message_str.c_str();

  FILE * fp = fp_in ? fp_in : stdout;
  fprintf (fp,"%s DATA_MSG field_buffer_.sum = %f\n", message,
            std::accumulate(field_buffer_.begin(),
                            field_buffer_.end(),0.0));
  fprintf (fp,"%s DATA_MSG field_list_src_.sum = %d\n", message,
            std::accumulate
            (field_list_src_.begin(),
             field_list_src_.end(),0));
  fprintf (fp,"%s DATA_MSG field_list_dst_.sum = %d\n", message,
            std::accumulate
            (field_list_dst_.begin(),
             field_list_dst_.end(),0));
  fprintf (fp,"%s DATA_MSG coarse iam3_   = %d %d %d\n",
            message,iam3_[0],iam3_[1],iam3_[2]);
  fprintf (fp,"%s DATA_MSG coarse iap3_   = %d %d %d\n",
            message,iap3_[0],iap3_[1],iap3_[2]);
  fprintf (fp,"%s DATA_MSG coarse ifms3_   = %d %d %d\n",
            message,ifms3_[0],ifms3_[1],ifms3_[2]);
  fprintf (fp,"%s DATA_MSG coarse ifps3_   = %d %d %d\n",
            message,ifps3_[0],ifps3_[1],ifps3_[2]);
  fprintf (fp,"%s DATA_MSG coarse ifmr3_   = %d %d %d\n",
            message,ifmr3_[0],ifmr3_[1],ifmr3_[2]);
  fprintf (fp,"%s DATA_MSG coarse ifpr3_   = %d %d %d\n",
            message,ifpr3_[0],ifpr3_[1],ifpr3_[2]);

}
