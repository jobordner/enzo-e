// See LICENSE_CELLO file for license and copyright information

/// @file     data_DataMsgCoarse.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2026-07-27
/// @brief    [\ref Data] Declaration of the DataMsgCoarse class

#ifndef DATA_DATA_MSG_COARSE_HPP
#define DATA_DATA_MSG_COARSE_HPP

class DataMsg;

class DataMsgCoarse {

  friend DataMsg;
  /// @class    DataMsgCoarse
  /// @ingroup  Data
  /// @brief    [\ref Data] 

public: // interface

  /// Constructor
  DataMsgCoarse() throw() :
    field_buffer_(),
    field_list_src_(),
    field_list_dst_()
  {
    for (int i=0; i<3; i++) {
      iam3_[i]  =0;
      iap3_[i]  =0;
      ifms3_[i]  =0;
      ifps3_[i]  =0;
      ifmr3_[i]  =0;
      ifpr3_[i]  =0;
    }
  }

  DataMsgCoarse & operator= (const DataMsgCoarse & DataMsgCoarse) = default;
  DataMsgCoarse & operator= (DataMsgCoarse && DataMsgCoarse) = default;
  DataMsgCoarse (const DataMsgCoarse & DataMsgCoarse) = default;
  DataMsgCoarse (DataMsgCoarse && DataMsgCoarse) = default;
  ~DataMsgCoarse() throw() = default;

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  DataMsgCoarse
  (Field field,
   int iam3[3],int iap3[3],
   int ifms3[3],int ifps3[3],
   int ifmr3[3],int ifpr3[3],
   const std::vector<int> & field_list_src,
   const std::vector<int> & field_list_dst);

  /// PACKING / UNPACKING

  /// Return the number of bytes required to serialize the data object
  int data_size () const;

  /// Serialize the object into the provided empty memory buffer.
  char * save_data (char * buffer) const;

  /// Restore the object from the provided initialized memory buffer data.
  char * load_data (char * buffer);

  /// Update the Data with the data stored in this DataMsg
  void update (Data * data);

  /// Debugging
  void print (std::string, FILE * fp = nullptr) const;

private: // attributes

  /// Padded coarse array values for prolongation operators that
  /// requiring extra layers of cells around the interpoltaed region
  std::vector<cello_float> field_buffer_;
  /// List of field indices for coarse fields; src / dst for
  /// accum=true
  std::vector<int> field_list_src_;
  std::vector<int> field_list_dst_;

  /// loop limits of the coarse-block array section
  int iam3_[3], iap3_[3];
  /// loop limits for the sending field
  int ifms3_[3], ifps3_[3];
  /// loop limits for the receiving field
  int ifmr3_[3], ifpr3_[3];

};

#endif /* DATA_DATA_MSG_COARSE_HPP */



