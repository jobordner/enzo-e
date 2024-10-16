// See LICENSE_CELLO file for license and copyright information

/// @file     parameters_ParameterGroup.cpp
/// @author   Matthew Abruzzo (matthewabruzzo@gmail.com)
/// @date     Jan 17 2023
/// @brief    [\ref Parameters] Implementation for the ParameterGroup class

#include "cello.hpp"

#include "parameters.hpp"

//----------------------------------------------------------------------

Param * ParameterGroup::param (const parameter_name_type & name)
{ return wrapped_p_.param(path_,name); }

//----------------------------------------------------------------------

parameter_type ParameterGroup::type
(const parameter_name_type & name) noexcept
{ return wrapped_p_.type(path_,name); }

//----------------------------------------------------------------------

template<>
int ParameterGroup::value
(const parameter_name_type & name, int deflt) noexcept
{ return wrapped_p_.value_integer(path_,name, deflt); }
template<>
int ParameterGroup::value
(const parameter_name_type & name) noexcept
{ return wrapped_p_.value_integer(path_,name, 0); }

//----------------------------------------------------------------------

template<>
double ParameterGroup::value
(const parameter_name_type & name, double deflt) noexcept
{ return wrapped_p_.value_float(path_,name, deflt); }
template<>
double ParameterGroup::value
(const parameter_name_type & name) noexcept
{ return wrapped_p_.value_float(path_,name, 0.0); }

//----------------------------------------------------------------------

template<>
bool ParameterGroup::value
(const parameter_name_type & name, bool deflt) noexcept
{ return wrapped_p_.value_logical(path_,name, deflt); }

//----------------------------------------------------------------------

template<>
std::string ParameterGroup::value
(const parameter_name_type & name, std::string deflt) noexcept
{ return wrapped_p_.value_string(path_,name, deflt); }
template<>
std::string ParameterGroup::value
(const parameter_name_type & name) noexcept
{ return wrapped_p_.value_string(path_,name, "undefined"); }

//----------------------------------------------------------------------

template<>
int ParameterGroup::value
(int i,const parameter_name_type & name, int deflt) noexcept
{ return wrapped_p_.list_value_integer(i, path_,name, deflt); }
template<>
int ParameterGroup::value
(int i,const parameter_name_type & name) noexcept
{ return wrapped_p_.list_value_integer(i, path_,name, 0); }

//----------------------------------------------------------------------

template<>
double ParameterGroup::value
(int i,const parameter_name_type & name, double deflt) noexcept
{ return wrapped_p_.list_value_float(i, path_,name, deflt); }
template<>
double ParameterGroup::value
(int i,const parameter_name_type & name) noexcept
{ return wrapped_p_.list_value_float(i, path_,name, 0.0); }

//----------------------------------------------------------------------

template<>
bool ParameterGroup::value
(int i,const parameter_name_type & name, bool deflt) noexcept
{ return wrapped_p_.list_value_logical(i, path_,name, deflt); }

//----------------------------------------------------------------------

template<>
std::string ParameterGroup::value
(int i,const parameter_name_type & name, std::string deflt) noexcept
{ return wrapped_p_.list_value_string(i, path_,name, deflt); }
template<>
std::string ParameterGroup::value
(int i,const parameter_name_type & name) noexcept
{ return wrapped_p_.list_value_string(i, path_,name, ""); }

//----------------------------------------------------------------------

int ParameterGroup::list_length (const parameter_name_type & name)
{
  return wrapped_p_.list_length(path_,name);
}

