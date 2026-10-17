// See LICENSE_CELLO file for license and copyright information

/// @file     data_FieldDescr.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2009-11-17
/// @brief    [\ref Data] Declaration for the FieldDescr class

#ifndef DATA_FIELD_DESCR_HPP
#define DATA_FIELD_DESCR_HPP

class FieldDescr 
{

  /// @class    FieldDescr
  /// @ingroup  Data
  /// @brief    [\ref Data] Interface for the FieldDescr class
  ///
  /// This class is used to store information about Fields in general,
  /// including field names, how they are centered (cell-centered,
  /// face-centered, on corners, etc), and number of ghost zones.
  /// There is one FieldDescr object per Simulation object.  Actual
  /// Field data are stored in FieldData objects, which are each
  /// associated with a unique Block of data.

  friend class Field;

public: // functions

  /// Initialize a FieldDescr object
  FieldDescr() throw()
  : name_(),
    num_permanent_(0),
    num_temporary_(0),
    id_(),
    groups_(),
    centering_(),
    ghost_depth_(),
    coalesced_(false),
    history_(0),
    history_id_()
  {
    for (int i=0; i<3; i++) {
      ghost_depth_default_[i] = 0;
    }
  }

  /// Destructor
  ~FieldDescr() throw()
  { }

  /// Copy constructor
  FieldDescr(const FieldDescr & field_descr) throw();

  /// Assignment operator
  FieldDescr & operator= (const FieldDescr & field_descr) throw();

  /// CHARM++ Pack / Unpack function
  inline void pup (PUP::er &p)
  {

    TRACEPUP;

    // NOTE: change this function whenever attributes change
    p | name_;
    p | num_permanent_;
    p | num_temporary_;
    p | id_;
    p | groups_;
    p | centering_;
    p | ghost_depth_;
    PUParray(p,ghost_depth_default_,3);
    p | coalesced_;
    p | history_;
    p | history_id_;
  }

  /// Set centering for a field
  void set_centering(int id_field, int cx, int cy=0, int cz=0) throw();

  /// Set default ghost_depth
  void set_default_ghost_depth(int gx, int gy=0, int gz=0) throw();

  /// Set ghost_depth for a field
  void set_ghost_depth(int id_field, int gx, int gy=0, int gz=0) throw();

  /// Set whether a field arrays for child blocks are allocated as a single
  /// array
  void set_coalesced (bool coalesced) throw()
  { coalesced_ = coalesced; }
  bool coalesced() const throw()
  { return coalesced_; }

  /// Insert a new permanent field
  int insert_permanent(const std::string & name_field) throw();

  /// Insert a new temporary field (does not require a name)
  int insert_temporary(const std::string & name_field = "") throw();

  /// Return the number of fields
  int field_count() const throw();

  /// Return name of the ith field
  std::string field_name(int id_field) const throw();

  /// Return whether the field has been inserted
  bool is_field(const std::string & name) const throw();

  /// Return the integer handle for the named field. If history > 0 return
  /// older version if available
  int field_id(const std::string & name, int history = 0) const throw();

  //----------------------------------------------------------------------
  // History
  //----------------------------------------------------------------------

  /// Set the history depth for storing old field values
  void set_history (int history) throw();

  void reset_history(int history) throw()
  {
    history_ = 0; // 0 forces recalculation in set_history()
    set_history(history);
  }

  /// Return the history depth for storing old field values
  int num_history () const throw()
  { return history_; }

  /// Return the temporary field id for given history "age" (0 is
  /// current, 1 first generation, etc.) for field ip
  int history_id (int ip, int age) const throw()
  {
    const int np = num_permanent();
    return (age == 0 || is_temporary(ip)) ?
      ip : history_id_[ip + np*(age-1)];
  }

  /// Return the current field (history = 0) corresponding to the given id
  /// with history >= 0.
  int curr_id (int id) const throw()
  {
    if (history_age(id) == 0) return id;
    size_t i=0;
    const int np = num_permanent();
    for (i=0; i<history_id_.size(); i++)
      if (history_id_[i] == id) break;
    return i % np;
  }

  /// Return the age of the given field id
  int history_age (int ip) const throw()
  {
    // (age == 0)
    const int np = num_permanent();
    if (ip < np) {
      return 0;
    } else {
      // (age > 0) or temporary
      size_t i=0;
      //    find inverse map i of history_id_[i] = ip
      for (i=0; i<history_id_.size(); i++)
        if (history_id_[i]==ip) break;
      //     if found, return computed age
      //     otherwise it's a non-saved temporary so must be age 0
      return (i < history_id_.size()) ?
        (1 + i/np) : 0;
    }

  }

  //----------------------------------------------------------------------
  // Properties
  //----------------------------------------------------------------------

  Grouping * groups () { return & groups_; }
  const Grouping * groups () const { return & groups_; }

  /// centering of given field
  void centering(int id_field, int * cx, int * cy = 0, int * cz = 0) const 
    throw();

  /// return whether the field variable is centered in the cell
  bool is_centered(int id_field) const
  {
    return (std::get<0>(centering_[id_field]) == 0 &&
            std::get<1>(centering_[id_field]) == 0 &&
            std::get<2>(centering_[id_field]) == 0); }

  /// depth of ghost zones of given field
  int  ghost_depth(int id_field, int * gx = 0, int * gy = 0, int * gz = 0) const 
    throw();

  /// Whether the field refers to a valid permanent field
  bool is_permanent (std::string field) const throw()
  { return is_permanent(field_id(field)); }
  /// Whether the field id refers to a valid permanent field
  bool is_permanent (int ip) const throw()
  {
    const int np = num_permanent();
    return ( (0 <= ip) && (ip < np)); }

  int num_permanent() const
  { return num_permanent_; }
  int num_temporary() const
  { return num_temporary_; }
  /// Whether the field refers to a valid temporary field
  bool is_temporary (std::string field) const throw()
  {  return is_temporary(field_id(field));  }
  /// Whether the field id refers to a valid temporary field
  bool is_temporary (int it) const throw()
  {
    const int np = num_permanent();
    const int nt = num_temporary();
    return ((np <= it) && (it < np + nt)); }

private: // functions

  void copy_(const FieldDescr & field_descr) throw();

  int insert_(const std::string & name_field,
	      bool is_permanent = true) throw();

private: // attributes

  /// String identifying each field
  std::vector<std::string> name_;

  /// Number of permanent fields declared
  int num_permanent_;

  /// Number of temporary fields declared
  int num_temporary_;

  /// Index of each field in name_
  std::map<std::string,int> id_;

  /// Groupings of fields
  Grouping groups_;

  /// cell centering for each field
  std::vector<std::tuple<int,int,int> > centering_;

  /// Ghost depth of each field, or -1 if using default
  std::vector<std::tuple<int,int,int> > ghost_depth_;

  /// Default ghost depth if not specified
  int ghost_depth_default_[3];

  /// Whether fields of sibling blocks are allocated as a single
  /// array
  bool coalesced_;

  /// Number of generations of history to save
  int history_;

  /// Temporary fields used for history.  Non-permuted.
  std::vector<int> history_id_;

};

#endif /* DATA_FIELD_DESCR_HPP */
