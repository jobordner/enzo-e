// See LICENSE_CELLO file for license and copyright information

/// @file     enzo_IoEnzoReader.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2022-02-01
/// @brief    [\ref Io] Declaration of the IoEnzoReader class

#ifndef ENZO_IO_ENZO_READER_HPP
#define ENZO_IO_ENZO_READER_HPP

class IoEnzoReader : public CBase_IoReader {

  /// @class    IoEnzoReader
  /// @ingroup  Io
  /// @brief    [\ref Io] 

public: // interface

  /// Constructor
  IoEnzoReader() throw()
  { }

  /// CHARM++ migration constructor
  IoEnzoReader(CkMigrateMessage *m) : CBase_IoReader(m) {}

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p) 
  { TRACEPUP; CBase_IoReader::pup(p); }

private: // functions


private: // attributes

  // NOTE: change pup() function whenever attributes change

};

#endif /* ENZO_IO_ENZO_READER_HPP */

