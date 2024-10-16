// See LICENSE_CELLO file for license and copyright information

/// @file     parameters_Parameters.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     Thu Jul  9 15:44:21 PDT 2009
/// @brief    [\ref Parameters] Declaration for the Parameters class

#ifndef PARAMETERS_PARAMETERS_HPP
#define PARAMETERS_PARAMETERS_HPP

/// @def      MAX_PARAMETER_FILE_WIDTH
/// @brief    Maximum allowed width of a line in a parameter file
#define MAX_PARAMETER_FILE_WIDTH 255

class Monitor;
class Parameters {

  /// @class    Parameters
  /// @ingroup  Parameters
  /// @brief    [\ref Parameters] Read in a parameter file and access
  /// parameter values

public: // interface


  /// Create an empty Parameters object
  Parameters(Monitor * monitor = 0) throw();

  /// Create a new Parameters object and read parameters from the given file
  Parameters(const char * file_name, 
	     Monitor * monitor = 0) throw();

  // At this time, we explicitly delete the copy/move constructor/assignment
  // methods (it may make sense to define some of these in the future)
  Parameters(const Parameters &) = delete;
  Parameters(Parameters&&) = delete;
  Parameters & operator= (const Parameters & ) = delete;
  Parameters & operator= (Parameters&& ) = delete;

  /// Delete a Parameters object (singleton design pattern)
  ~Parameters();

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  /// Read in parameters from a file
  void read (const char * file_name);
  /// Write parameters to a file
  void write (const char * file_name, int write_type = param_write_cello);
  void write (FILE * fp, int write_type = param_write_cello);

  int value
  ( const parameter_path_type & path,
    const parameter_name_type  & name, int deflt) throw()
  { return value_integer(path,name,deflt); }
  int value
  ( const parameter_name_type  & name, int deflt) throw()
  { return value_integer(root_group,name,deflt); }

  double value
  ( const parameter_path_type & path,
    const parameter_name_type & name, double deflt) throw()
  { return value_float(path,name,deflt); }
  double value
  ( const parameter_name_type & name, double deflt) throw()
  { return value_float(root_group,name,deflt); }

  bool value
  ( const parameter_path_type & path,
    const parameter_name_type & name, bool deflt) throw()
  { return value_logical(path,name,deflt); }
  bool value
  ( const parameter_name_type & name, bool deflt) throw()
  { return value_logical(root_group,name,deflt); }

  std::string value
  ( const parameter_path_type & path,
    const parameter_name_type & name, const char * deflt) throw()
  { return value_string(path,name,deflt); }
  std::string value
  ( const parameter_name_type & name, const char * deflt) throw()
  { return value_string(root_group,name,deflt); }

  std::string value
  ( const parameter_path_type & path,
    const parameter_name_type & name, const std::string & deflt) throw()
  { return value_string(path,name,deflt); }
  std::string value
  ( const parameter_name_type & name, const std::string & deflt) throw()
  { return value_string(root_group,name,deflt); }

  int value
  ( int i,
    const parameter_path_type & path,
    const parameter_name_type & name, int deflt) throw()
  { return list_value_integer(i,path,name,deflt); }
  int value
  ( int i,
    const parameter_name_type & name, int deflt) throw()
  { return list_value_integer(i,root_group,name,deflt); }

  double value
  ( int i,
    const parameter_path_type & path,
    const parameter_name_type & name, double deflt) throw()
  { return list_value_float(i,path,name,deflt); }
  double value
  ( int i,
    const parameter_name_type & name, double deflt) throw()
  { return list_value_float(i,root_group,name,deflt); }

  bool value
  ( int i,
    const parameter_path_type & path,
    const parameter_name_type & name, bool deflt) throw()
  { return list_value_logical(i,path,name,deflt); }
  bool value
  ( int i,
    const parameter_name_type & name, bool deflt) throw()
  { return list_value_logical(i,root_group,name,deflt); }

  std::string value
  ( int i,
    const parameter_path_type & path,
    const parameter_name_type & name, const char * deflt) throw()
  { return list_value_string(i,path,name,deflt); }
  std::string value
  ( int i,
    const parameter_name_type & name, const char * deflt) throw()
  { return list_value_string(i,root_group,name,deflt); }

  std::string value
  ( int i,
    const parameter_path_type & path,
    const parameter_name_type & name, const std::string & deflt) throw()
  { return list_value_string(i,path,name,deflt); }
  std::string value
  ( int i,
    const parameter_name_type & name, const std::string & deflt) throw()
  { return list_value_string(i,root_group,name,deflt); }

  /// Return the integer-valued parameter
  int value_integer
  ( const parameter_path_type & path,
    const parameter_name_type & name, int deflt = 0) throw();
  int value_integer
  ( const parameter_name_type & name, int deflt = 0) throw()
  { return value_integer (root_group,name,deflt); }

  /// Return the floating-point valued parameter
  double value_float
  ( const parameter_path_type & path,
    const parameter_name_type & name, double deflt = 0.0) throw();
  double value_float
  ( const parameter_name_type & name, double deflt = 0.0) throw()
  { return value_float (root_group,name,deflt); }

  /// Return the logical-valued parameter
  bool value_logical
  ( const parameter_path_type & path,
    const parameter_name_type & name, bool deflt = false) throw();
  bool value_logical
  ( const parameter_name_type & name, bool deflt = false) throw()
  { return value_logical (root_group,name,deflt); }

  /// Return the string-valued parameter
  std::string value_string
  ( const parameter_path_type & path,
    const parameter_name_type & name,
    std::string deflt = "") throw();
  std::string value_string
  ( const parameter_name_type & name,
    std::string deflt = "") throw()
  { return value_string (root_group,name,deflt); }

  /// Return the length of the list parameter
  int list_length (const parameter_path_type & path,
                   const parameter_name_type & name);
  int list_length (const parameter_name_type & name)
  { return list_length (root_group,name); }

  /// Access an integer list element
  int list_value_integer (int ,
                          const parameter_path_type & path,
                          const parameter_name_type & name,
                          int deflt = 0) throw();
  int list_value_integer (int i,
                          const parameter_name_type & name,
                          int deflt = 0) throw()
  { return list_value_integer (i,root_group,name,deflt); }

  /// Access a floating point list element
  double list_value_float (int ,
                           const parameter_path_type & path,
                           const parameter_name_type & name,
                           double deflt = 0.0) throw();
  double list_value_float (int i,
                           const parameter_name_type & name,
                           double deflt = 0.0) throw()
  { return list_value_float (i,root_group,name,deflt); }

  /// Access a logical list element
  bool list_value_logical (int ,
                           const parameter_path_type & path,
                           const parameter_name_type & name,
                           bool deflt = false) throw();
  bool list_value_logical (int i,
                           const parameter_name_type & name,
                           bool deflt = false) throw()
  { return list_value_logical (i,root_group,name,deflt); }

  /// Access a string list element
  std::string list_value_string
  ( int,
    const parameter_path_type & path,
    const parameter_name_type & name,
    std::string deflt= "") throw();
  std::string list_value_string
  ( int i,
    const parameter_name_type & name,
    std::string deflt= "") throw()
  { return list_value_string (i,root_group,name, deflt); }

  // Assign a value to the integer-valued parameter
  void set_integer ( const parameter_path_type & path,
                     const parameter_name_type & name, int value ) throw();
  void set_integer ( const parameter_name_type & name, int value ) throw()
  { set_integer (root_group,name,value); }

  // Assign a value to the floating-point valued parameter
  void set_float ( const parameter_path_type & path,
                   const parameter_name_type & name, double value ) throw();
  void set_float ( const parameter_name_type & name, double value ) throw()
  { set_float (root_group,name,value); }

  /// Assign a value to the logical-valued parameter
  void set_logical ( const parameter_path_type & path,
                     const parameter_name_type & name, bool value ) throw();
  void set_logical ( const parameter_name_type & name, bool value ) throw()
  { set_logical (root_group,name,value); }

  /// Assign a value to the string-valued parameter
  void set_string ( const parameter_path_type & path,
                    const parameter_name_type & name,
                    const char * value ) throw();
  void set_string ( const parameter_name_type & name,
                    const char * value ) throw()
  { set_string (root_group,name,value); }

  /// Return the length of the list parameter
  void set_list_length (const parameter_path_type & path,
                        const parameter_name_type & name, int length) throw ();
  void set_list_length (const parameter_name_type & name, int length) throw ()
  { set_list_length (root_group,name,length); }
 
  /// Assign a value to an integer-valued list element parameter
  void set_list_integer (int ,
                         const parameter_path_type & path,
                         const parameter_name_type & name , int value) throw();
  void set_list_integer (int i,
                         const parameter_name_type & name , int value) throw()
  { set_list_integer (i,root_group,name,value); }

  /// Assign a value to the floating-point list-element parameter
  void set_list_float (int ,
                       const parameter_path_type & path,
                       const parameter_name_type & name, double value) throw();
  void set_list_float (int i,
                       const parameter_name_type & name, double value) throw()
  { set_list_float (i,root_group,name,value); }

  ///  Assign a value to the logical-valued list-element parameter
  void set_list_logical (int ,
                         const parameter_path_type & path,
                         const parameter_name_type & name, bool value) throw();
  void set_list_logical (int i,
                         const parameter_name_type & name, bool value) throw()
  { set_list_logical (i, root_group,name,value); }

  /// Assign a value to the string-valued list-element parameter
  void set_list_string (int,
                        const parameter_path_type & path,
                        const parameter_name_type & name,
                        const char * value) throw();
  void set_list_string (int i,
                        const parameter_name_type & name,
                        const char * value) throw()
  { set_list_string (i, root_group,name,value); }


  void done_set_list ( const parameter_path_type & path,
                       const parameter_name_type & name ) throw()
  { monitor_write_(path,name); }
  void done_set_list ( const parameter_name_type & name ) throw()
  { monitor_write_(root_group,name); }

  /// Evaluate the floating-point valued parameter expression
  void evaluate_float 
  (
   const parameter_path_type & path,
   const parameter_name_type & name,
   int         n, 
   double    * result, 
   double    * deflt,
   double    * x, 
   double    * y, 
   double    * z, 
   double    t) throw();
  void evaluate_float 
  (
   const parameter_name_type & name,
   int         n, 
   double    * result, 
   double    * deflt,
   double    * x, 
   double    * y, 
   double    * z, 
   double    t) throw()
  { evaluate_float( root_group, name,n,result,deflt,x,y,z,t); }

  /// Evaluate the logical-valued parameter expression
  void evaluate_logical 
  (
   const parameter_path_type & path,
   const parameter_name_type & name,
   int         n, 
   bool      * result, 
   bool      * deflt,
   double    * x, 
   double    * y, 
   double    * z, 
   double    t) throw();
  void evaluate_logical 
  (
   const parameter_name_type & name,
   int         n, 
   bool      * result, 
   bool      * deflt,
   double    * x, 
   double    * y, 
   double    * z, 
   double    t) throw()
  { evaluate_logical( root_group, name,n,result,deflt,x,y,z,t); }

  /// Evaluate the floating-point valued list element expression
  void list_evaluate_float 
  (
   int ,
   const parameter_path_type & path,
   const parameter_name_type & name,
   int         n, 
   double    * result, 
   double    * deflt,
   double    * x, 
   double    * y, 
   double    * z, 
   double    t
   ) throw();
  void list_evaluate_float 
  (
   int i,
   const parameter_name_type & name,
   int         n, 
   double    * result, 
   double    * deflt,
   double    * x, 
   double    * y, 
   double    * z, 
   double    t
   ) throw()
  { list_evaluate_float( i, root_group, name,n,result,deflt,x,y,z,t); }
  
  /// Evaluate the logical-valued list element expression
  void list_evaluate_logical
  (
   int ,
   const parameter_path_type & path,
   const parameter_name_type & name,
   int         n, 
   bool      * result, 
   bool      * deflt,
   double    * x, 
   double    * y, 
   double    * z, 
   double    t
   )    
    throw();
  void list_evaluate_logical
  (
   int i,
   const parameter_name_type & name,
   int         n, 
   bool      * result, 
   bool      * deflt,
   double    * x, 
   double    * y, 
   double    * z, 
   double    t
   )    
    throw()
  { list_evaluate_logical( i, root_group, name,n,result,deflt,x,y,z,t); }

  /// utility for parsing a parameter that is expected to always be a list
  /// containing strings. An empty vector is returned if the parameter does not
  /// exist or was assigned an empty list. If the parameter was assigned a
  /// value with a different type, or a list entry is not a string, the program
  /// will abort with an error.
  ///
  /// When ``coerce_string_to_list`` is ``false``, the program will abort with
  /// an error if the parameter was assigned a string value. Otherwise, the
  /// string will be coerced to a 1-element vector.
  ///
  /// When suppress_err is true, the method returns an empty vector, even in
  /// the case of an error.
  ///
  /// @note
  /// Overhead from returning a vector is minimal. The compiler will know it
  /// it can implicitly move contents into another vector
  ///
  /// @note
  /// This method encapsulates a parameter-parsing parameter that comes up a
  /// lot! But, this may not be the best place to put this functionality
  std::vector<std::string> value_full_strlist
  ( const parameter_path_type & path,
    const parameter_name_type & name,
    bool coerce_string_to_list,
    bool suppress_err = false)
    throw();
  std::vector<std::string> value_full_strlist
  ( const parameter_name_type & name,
    bool coerce_string_to_list,
    bool suppress_err = false)
    throw()
  {
    return value_full_strlist
      (root_group,name, coerce_string_to_list,suppress_err);
  }

  /// Returns a vector holding the names of all leaf parameters in the
  /// specified parameter group
  const parameter_path_type leaf_parameter_names
  (const parameter_name_type & full_group_name) const throw();

  /// Returns a vector holding the names of all leaf parameters in the current
  /// group
  ///
  /// this overload of the function is deprecated, but exists to maintain
  /// backwards compatibility
  const parameter_path_type leaf_parameter_names(const parameter_path_type &)
    const throw();
  const parameter_path_type leaf_parameter_names() const throw()
  { return leaf_parameter_names(root_group); }

  /// Return the full name of the parameter including group
  const parameter_name_type full_name (const parameter_path_type & path,
                         const parameter_name_type & name) const throw();
  const parameter_name_type full_name (const parameter_name_type & name) const throw()
  { return full_name(root_group,name); }

  /// Return the type of the given parameter
  parameter_type type(const parameter_path_type & path,
                      const parameter_name_type &) throw();
  parameter_type type(const parameter_name_type & name) throw()
  { return type(root_group,name); }

  /// Return the type of the given parameter
  parameter_type list_type(int,
                           const parameter_path_type & path,
                           const parameter_name_type &) throw();
  parameter_type list_type(int i,
                           const parameter_name_type & name) throw()
  { return list_type(i,root_group,name); }

  /// Set whether to output 
  void set_monitor (bool lmonitor) { lmonitor_ = lmonitor; };

  void check() const;

  /// Return the Param pointer for the specified parameter
  Param * param (const parameter_path_type & path,
                 const parameter_name_type & name, int index = -1);
  Param * param (const parameter_name_type & name, int index = -1)
  { return param (root_group,name,index); }

  //--------------------------------------------------

private: // functions

  /// Read in the next line of the input file
  int readline_ (FILE* fp, char * buffer, int n) throw();

  /// Return the full parameter name Group:group:group:...:parameter
  parameter_name_type parameter_name_(const parameter_path_type & path,
				      const parameter_name_type & name)
  {
    bool is_full_parameter = (name.find(":") != std::string::npos);
    return (is_full_parameter ? name : full_name (path,name));
  }
  parameter_name_type parameter_name_(const parameter_name_type & name)
  { return parameter_name_ (root_group,name); }

  /// Return the Param pointer for the specified list parameter element
  Param * list_element_ (const parameter_path_type & path,
                         const parameter_name_type & name, int index) throw();
  Param * list_element_ (const parameter_name_type & name, int index) throw()
  { return list_element_ (root_group,name,index); }

  /// Display through monitor that a parameter was accessed
  void monitor_access_ (const parameter_path_type & path,
                        const parameter_name_type & name,
			std::string deflt_string,
			int index=-1) throw();
  void monitor_access_ (const parameter_name_type & name,
			std::string deflt_string,
			int index=-1) throw()
  { return monitor_access_ (root_group,name,deflt_string,index); }

  /// Display through monitor that a parameter was assigned a value
  void monitor_write_ (const parameter_path_type & path,
                       const parameter_name_type & name) throw();
  void monitor_write_ (const parameter_name_type & name) throw()
  { return monitor_write_ (root_group,name); }

  /// Create a new parameter with the given grouping and name
  void new_param_ ( const parameter_name_type & full_parameter,
		    Param * param ) throw();

  size_t extract_groups_(const parameter_name_type & full_parameter, std::string * group);

  //--------------------------------------------------

private: // attributes

  /// Map parameter name to Param object
  std::map<parameter_name_type, Param *>  parameter_map_;

  /// Parameters represented as a tree with groups as internal nodes
  ParamNode parameter_tree_;

  /// Monitor object for parameters
  Monitor * monitor_; 

  /// Whether monitor should output accessed parameters when requested
  bool lmonitor_;
};

//----------------------------------------------------------------------

extern "C" { 
  /// C function for reading parameters from a file
  struct param_struct * cello_parameters_read(const char *, FILE *);
  /// C function for printing parameters to stdout
  void cello_parameters_print();
}

extern Parameters g_parameters;

//----------------------------------------------------------------------

#endif /* PARAMETERS_PARAMETERS_HPP */

