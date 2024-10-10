// See LICENSE_CELLO file for license and copyright information

/// @file     parameters_ParameterGroup.hpp
/// @author   Matthew Abruzzo (matthewabruzzo@gmail.com)
/// @date     Jan 17 2023
/// @brief    [\ref Parameters] Declaration for the ParameterGroup class

#ifndef PARAMETERS_PARAMETER_ACCESSOR_HPP
#define PARAMETERS_PARAMETER_ACCESSOR_HPP

class ParameterGroup {

  /// @class    ParameterGroup
  /// @ingroup  Parameters
  /// @brief    [\ref Parameters] Acts as an interface for accessing parameters
  ///           within a parameter group.
  ///
  /// A lightweight wrapper around a Parameters object that provides methods
  /// for accessing parameters in a parameter group, that are stored within the
  /// wrapped object.
  ///
  /// All parameters in a group, all share a common root "parameter_path"
  /// (defined below). The common root "parameter_path" is specified when an
  /// instance of this object is initialized. When a string is passed to one of
  /// the accessor methods, the string is internally appended to the end of the
  /// root "parameter_path" and the result represents the full name of the
  /// queried parameter.
  ///
  /// Instances of this class are intended to be used when initializing objects
  /// in other software components (an instance of this class should be passed
  /// to a factory-method or constructor).
  ///
  /// This class was designed with a few guiding principles:
  ///     1. restricting access to parameters within the associated root path
  ///        This is to discourage design of objects that are configured by
  ///        parameters scattered throughout the parameter file. In rare cases
  ///        (e.g. deprecating a parameter), exceptions need to be made. Thus,
  ///        an "escape-hatch" is provided to directly access the wrapped
  ///        Parameters object.
  ///     2. providing an explicit way to signal that all parameters accessed
  ///        through a given instance of this class share a single (queryable &
  ///        immutable) root-path without requiring the root path to be
  ///        explicitly specified as part of every parameter-path.
  ///        - This capability is very useful when multiple instances of the
  ///          same class (e.g. a Method class) needs to be initialized but
  ///          each instance has different configurations.
  ///        - In that scenario, each instance of the class is typically
  ///          initialized from parameters where the last section(s) of the
  ///          parameter-paths are unchanged, but the root path differs.
  ///     3. We do NOT allow the common root-path to be mutated. Thus, if
  ///        an instance is passed to a helper function, you can always be
  ///        confident that the root-path is unchanged by the helper function
  ///        (without needing to check the implementation of that function).
  ///        If we are ever tempted to mutate the root-path, we should just
  ///        initialize a new ParameterGroup (since they are lightweight)
  ///
  /// @par "parameter_path"
  /// Cello uses a "hierarchical" parameter file: the parameters themselves are
  /// essentially leaf nodes in a tree-hierarchy of "groups" (like how a file
  /// is organized in a directory hierarchy). A parameter_path unambiguously
  /// identifes a parameter in this hierarchy. A given parameter_path lists the
  /// names of ancestor "groups", separated by colons, and lists the name
  /// of the parameter at the end. An example parameter_path is
  /// ``"Method:null:dt"``

public:

  /// construct a new ParameterGroup object
  ParameterGroup(Parameters &p, const parameter_path_type & current_group)
    : wrapped_p_(p),
      current_group_(current_group)
  {
  }

  // default implementations of copy and move constructors
  ParameterGroup(const ParameterGroup&) = default;
  ParameterGroup(ParameterGroup&&) = default;

  // the following operations are deleted (because the class holds a reference
  // and a const member as attributes)
  ParameterGroup() = delete;
  ParameterGroup& operator=(const ParameterGroup&) = delete;
  ParameterGroup& operator=(ParameterGroup&&) = delete;

  /// query the path for the represented group of parameters
  ///
  /// For the sake of clarity, we highlight a few examples:
  /// - if ``*this`` represents the group containing the parameters with paths
  ///   "foo:bar:par_1" and "foo:bar:par_2", then this method would return
  ///   ``"foo:bar``".
  /// - if ``*this`` represents the group containing the parameters with paths
  ///   "foo:par_A" and "foo:par_B", then this method would return ``"foo"``
  /// - if ``*this`` represents the group containing the parameters with paths
  ///   "par_alpha" and "par_beta", then this method would return ``""``
  ///   (Note: by convention, parameters are never actually put in this group)
  const std::string get_group_path() const noexcept
  {
    std::string path="";
    if (current_group_.size()>0) {
      path=current_group_[0];
      for (size_t i=1; i<current_group_.size(); i++) {
        path += std::string(":") + current_group_[i];
      }
    }
    return path;
  }

  template <class T> T value
  (const parameter_name_type & parameter_name) noexcept;
  template <class T> T value
  (const parameter_name_type & parameter_name, T deflt) noexcept;

  template <class T> T value
  (int i, const parameter_name_type & parameter_name) noexcept;
  template <class T> T value
  (int i, const parameter_name_type & parameter_name, T deflt) noexcept;

  /// Return the full name of the parameter (including the root parameter path)
  std::string full_name(const parameter_name_type & parameter) const noexcept
  {
    std::string group_path = get_group_path();
    return (group_path == "") ? parameter : group_path + ":" + parameter;;
  }
  /// Return the type of the given parameter
  parameter_type type(const parameter_name_type & param) noexcept;

  /// Return the Param pointer for the specified parameter
  Param * param (const parameter_name_type & parameter);

  // /// Return the length of the list parameter
  int list_length (const parameter_name_type & parameter);

  /// Returns a vector holding the names of all leaf parameters that share the
  /// root parameter path encapsulated by this object
  std::vector<std::string> leaf_parameter_names() const noexcept
  { return wrapped_p_.leaf_parameter_names(current_group_); }

private:

private: // attributes
  /// the wrapped Parameters object
  ///
  /// the Parameters object is implicitly assumed to outlive the instance
  /// holding this reference
  Parameters &wrapped_p_;

  /// Current group, moved from Parameters for thread safety
  parameter_path_type current_group_;

};

#endif /* PARAMETERS_PARAMETER_ACCESSOR_HPP */
