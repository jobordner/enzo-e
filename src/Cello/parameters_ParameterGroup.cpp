// See LICENSE_CELLO file for license and copyright information

/// @file     parameters_ParameterGroup.cpp
/// @author   Matthew Abruzzo (matthewabruzzo@gmail.com)
/// @date     Jan 17 2023
/// @brief    [\ref Parameters] Implementation for the ParameterGroup class

#include "cello.hpp"

#include "parameters.hpp"

//----------------------------------------------------------------------

Param * ParameterGroup::param (const parameter_name_type & parameter_name)
{ return wrapped_p_.param(current_group_,parameter_name); }

//----------------------------------------------------------------------

parameter_type ParameterGroup::type
(const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.type(current_group_,parameter_name); }

//----------------------------------------------------------------------

template<>
int ParameterGroup::value
(const parameter_name_type & parameter_name, int deflt) noexcept
{ return wrapped_p_.value_integer(current_group_,parameter_name, deflt); }
template<>
int ParameterGroup::value
(const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.value_integer(current_group_,parameter_name, 0); }

//----------------------------------------------------------------------

template<>
double ParameterGroup::value
(const parameter_name_type & parameter_name, double deflt) noexcept
{ return wrapped_p_.value_float(current_group_,parameter_name, deflt); }
template<>
double ParameterGroup::value
(const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.value_float(current_group_,parameter_name, 0.0); }

//----------------------------------------------------------------------

template<>
bool ParameterGroup::value
(const parameter_name_type & parameter_name, bool deflt) noexcept
{ return wrapped_p_.value_logical(current_group_,parameter_name, deflt); }

//----------------------------------------------------------------------

template<>
std::string ParameterGroup::value
(const parameter_name_type & parameter_name, std::string deflt) noexcept
{ return wrapped_p_.value_string(current_group_,parameter_name, deflt); }
template<>
std::string ParameterGroup::value
(const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.value_string(current_group_,parameter_name, "undefined"); }

//----------------------------------------------------------------------

template<>
const char * ParameterGroup::value
(const parameter_name_type & parameter_name, const char * deflt) noexcept
{ return wrapped_p_.value_string(current_group_,parameter_name, deflt).c_str(); }
template<>
const char * ParameterGroup::value
(const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.value_string(current_group_,parameter_name, "undefined").c_str(); }

//----------------------------------------------------------------------

template<>
int ParameterGroup::value
(int i,const parameter_name_type & parameter_name, int deflt) noexcept
{ return wrapped_p_.list_value_integer(i, current_group_,parameter_name, deflt); }
template<>
int ParameterGroup::value
(int i,const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.list_value_integer(i, current_group_,parameter_name, 0); }

//----------------------------------------------------------------------

template<>
double ParameterGroup::value
(int i,const parameter_name_type & parameter_name, double deflt) noexcept
{ return wrapped_p_.list_value_float(i, current_group_,parameter_name, deflt); }
template<>
double ParameterGroup::value
(int i,const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.list_value_float(i, current_group_,parameter_name, 0.0); }

//----------------------------------------------------------------------

template<>
bool ParameterGroup::value
(int i,const parameter_name_type & parameter_name, bool deflt) noexcept
{ return wrapped_p_.list_value_logical(i, current_group_,parameter_name, deflt); }

//----------------------------------------------------------------------

template<>
std::string ParameterGroup::value
(int i,const parameter_name_type & parameter_name, std::string deflt) noexcept
{ return wrapped_p_.list_value_string(i, current_group_,parameter_name, deflt); }
template<>
std::string ParameterGroup::value
(int i,const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.list_value_string(i, current_group_,parameter_name, ""); }

//----------------------------------------------------------------------

template<>
const char * ParameterGroup::value
(int i,const parameter_name_type & parameter_name, const char * deflt) noexcept
{ return wrapped_p_.list_value_string(i, current_group_,parameter_name, deflt).c_str(); }
template<>
const char * ParameterGroup::value
(int i,const parameter_name_type & parameter_name) noexcept
{ return wrapped_p_.list_value_string(i, current_group_,parameter_name, "").c_str(); }

//----------------------------------------------------------------------

int ParameterGroup::list_length (const parameter_name_type & parameter_name)
{
  return wrapped_p_.list_length(current_group_,parameter_name);
}

