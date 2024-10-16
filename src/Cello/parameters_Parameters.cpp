// See LICENSE_CELLO file for license and copyright information

/// @file     parameters_Parameters.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     Thu Jul  9 15:38:43 PDT 2009
/// @brief    Read in a parameter file and access parameter values

#include "cello.hpp"

#include "parameters.hpp"

Parameters g_parameters;

// #define TEMP_BYPASS_DESTRUCTOR

//----------------------------------------------------------------------

Parameters::Parameters(Monitor * monitor) 
  throw()
  : parameter_map_(),
    parameter_tree_("Cello"),
    monitor_(monitor),
    lmonitor_(true)
{
  if (! monitor_) lmonitor_ = false;
}

//----------------------------------------------------------------------

Parameters::Parameters(const char * file_name,
		       Monitor * monitor) 
  throw()
  : Parameters(monitor)
{
  read(file_name);
}

//----------------------------------------------------------------------

Parameters::~Parameters()
///
{
  // Iterate over all parameters, deleting their values


#ifdef TEMP_BYPASS_DESTRUCTOR
  for (auto it_param =  parameter_map_.begin();
       it_param != parameter_map_.end();
       ++it_param) {
    ASSERT1("Parameters::~Parameters",
            "parameter \"%s\" associated with a nullptr. This isn't allowed",
            (it_param->first).c_str(), (it_param->second != nullptr));
    delete it_param->second;
  }
#endif
}

//----------------------------------------------------------------------

void Parameters::pup (PUP::er &p)
{
  TRACEPUP;

  // pup parameter_map_:
  int n = 0;
  if (!p.isUnpacking()) {
    // Figure out size
    for (auto it_param =  parameter_map_.begin();
	 it_param != parameter_map_.end();
	 ++it_param) {
      n++;
    }
  }
  p | n;
  if (!p.isUnpacking()) {
    for (auto it_param =  parameter_map_.begin();
	 it_param != parameter_map_.end();
	 ++it_param) {
      std::string name = it_param->first;
      Param * param = it_param->second;
      ASSERT1("Parameters::pup",
              "parameter \"%s\" associated with a nullptr. This isn't allowed",
              name.c_str(), (param != nullptr));
      p | name;
      p | *param;
    } 
  } else {
    for (int i=0; i<n; i++) {
      std::string name;
      p | name;
      Param * param = new Param;
      p | *param;
      parameter_map_[name] = param;
    }
  }

  // previously we didn't pup parameter_tree_. That was probably because
  // ParamNode::pup had bugs (they should be fixed now)
  p | parameter_tree_;

  // this method assumes that the monitor_ and lmonitor_ methods are already
  // correctly initialized
}

//----------------------------------------------------------------------

void Parameters::read ( const char * file_name )
/// @param    file_name An opened input parameter file or stdin
{

  FILE * file_pointer = fopen(file_name,"r");

  ASSERT1("Parameters::read",
	  "Error opening parameter file '%s' for reading",
	  file_name,
	  ( file_pointer != NULL ));

  struct param_struct * parameter_list = 
    cello_parameters_read(file_name,file_pointer);

  struct param_struct * node = parameter_list -> next; // skip sentinel
  struct param_struct * prev = node;

  while (node->type != enum_parameter_sentinel) {

    Param * param = new Param;

    param->set(node);

    std::string full_parameter = "";
    for (int i=0;  (i < MAX_GROUP_DEPTH) && node->group[i] != 0; i++) {
      full_parameter = full_parameter + node->group[i] + ":";
    }

    full_parameter = full_parameter + node->parameter;

    new_param_(full_parameter,param);

    node = node->next;
    
    // free not delete since allocated in parse.y
    for (int i=0; (i < MAX_GROUP_DEPTH) && prev->group[i]; i++) {
      free (prev->group[i]);
    }
    free (prev);

    prev = node;
    
  }

  // assert: node->type == enum_parameter_sentinel

  free (node);
  node = NULL;

  fclose(file_pointer);

  if (lmonitor_) monitor_->print ("Parameters","read in %s",file_name);
}

//----------------------------------------------------------------------

void Parameters::write ( const char * file_name, int type )
/// @param  file_name   An opened output parameter file or stdout
{
  FILE * fp = fopen(file_name,"w");
  ASSERT1("Parameters::write",
	  "Error opening parameter file '%s' for writing",
	  file_name,
	  ( fp != NULL ) );

  write (fp,type);
  fclose(fp);
}

//----------------------------------------------------------------------

void Parameters::write ( FILE * fp, int type )
/// @param  file_name   An opened output parameter file or stdout
{
  // "Previous" groups are empty
  int n_prev = 0;
  std::string group_prev[MAX_GROUP_DEPTH];

  // Initialize "current" groups as empty
  std::string group_curr[MAX_GROUP_DEPTH];

  // Initialize indentation variables
  const std::string indent_amount = "    ";
  int indent_size = 4;
  std::string indent_string = " ";
  int group_depth = 0;

  // Loop over parameters

  for (auto it_param =  parameter_map_.begin();
       it_param != parameter_map_.end();
       ++it_param) {

    // Ensure that the current parameter have a value
    ASSERT1("Parameters::write",
            "parameter \"%s\" associated with a nullptr. This isn't allowed",
            (it_param->first).c_str(), (it_param->second != nullptr));

    // Determine groups of the current parameter
    int n_curr = extract_groups_(it_param->first,group_curr);

    // Determine the first group that differs, if any
    int i_group;
    for (i_group = 0;
         i_group < n_prev && i_group < n_curr &&
           group_prev[i_group] == group_curr[i_group];
         i_group++) {
      // (Intentionally blank)
    }

    // End old groups

    for (int i=i_group; i<n_prev; i++) {
      --group_depth;
      indent_string = indent_string.substr(0, indent_string.size()-indent_size);
      if (type != param_write_monitor) {
        fprintf (fp, "%s}%c\n",indent_string.c_str(),
                 (group_depth==0) ? '\n' : ';' );
      }
    }

    // Begin new groups

    for (int i=i_group; i<n_curr; i++) {
      if (type != param_write_monitor) {
        const char * format =
          (type==param_write_libconfig) ? "%s%s : {\n" : "%s%s {\n";
        fprintf (fp, format ,indent_string.c_str(),
                 group_curr[i].c_str());
      }
      indent_string = indent_string + indent_amount;
      ++group_depth;
    }

    // Print parameter
    if (type == param_write_monitor) {
      Monitor monitor;
      monitor.set_mode(monitor_mode_all);
      // display Monitor prefix if full_names
      monitor.write(fp,"Parameters","");
      for (int i=0; i < n_curr; i++) {
        fprintf (fp,"%s:",group_curr[i].c_str());
      }
    } else {
      fprintf (fp,"%s",indent_string.c_str());
    }
    it_param->second->write(fp,it_param->first,type);

    // Copy current groups to previous groups (inefficient)
    n_prev = n_curr;
    for (int i=0; i<n_prev; i++) {
      group_prev[i] = group_curr[i];
    }

  }

  // End old groups

  for (int i=0; i<n_prev; i++) {
    indent_string = indent_string.substr(indent_size,std::string::npos);
    if (type != param_write_monitor) {
      fprintf (fp, "%s}\n",indent_string.c_str());
    }
    --group_depth;
  }

}

//----------------------------------------------------------------------

int Parameters::value_integer 
( const parameter_path_type & path,
  const parameter_name_type & name,
  int         deflt ) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   deflt     Default parameter value
/// @return  Return integer parameter value if it exists, deflt if not
{
  Param * param = this->param(path,name);

  ASSERT1 ("Parameters::value_integer",
	   "Parameter %s is not an integer", name.c_str(),
	   ( ! param || param->is_type(parameter_integer)));

  char deflt_string[MAX_PARAMETER_FILE_WIDTH];
  snprintf (deflt_string,MAX_PARAMETER_FILE_WIDTH,"%d",deflt);
  monitor_access_(path,name,deflt_string);
  return (param != NULL) ? param->get_integer() : deflt;
}

//----------------------------------------------------------------------

void Parameters::set_integer 
( const parameter_path_type & path,
  const parameter_name_type & name,
  int         value ) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   value     Value to set the parameter
{
  Param * param = this->param(path,parameter_name_(path,name));

  ASSERT1 ("Parameters::set_integer",
	   "Parameter %s is not an integer", name.c_str(),
	   ( ! param || param->is_type(parameter_integer)));

  if ( ! param ) {
    param = new Param;
    new_param_ (parameter_name_(path,name),param);
  }

  param->set_integer_(value);
  monitor_write_(path,name);
}

//----------------------------------------------------------------------

double Parameters::value_float
( const parameter_path_type & path,
  const parameter_name_type & name,
  double      deflt ) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   deflt     Default parameter value
/// @return  Return floating point (double) parameter value if it exists, deflt if not
{
  Param * param = this->param(path,name);

  ASSERT1 ("Parameters::value_float",
	   "Parameter %s is not a float", name.c_str(),
	   ( ! param || param->is_type(parameter_float)));

  char deflt_string[MAX_PARAMETER_FILE_WIDTH];
  // '#' format character forces a decimal point
  snprintf (deflt_string,MAX_PARAMETER_FILE_WIDTH,FLOAT_FORMAT,deflt);
  monitor_access_(path,name,deflt_string);
  return (param != NULL) ? param->get_float() : deflt;
}

//----------------------------------------------------------------------

void Parameters::set_float
( const parameter_path_type & path,
  const parameter_name_type & name,
  double      value ) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   value     Value to set the parameter
{
  Param * param = this->param(path,name);

  ASSERT1 ("Parameters::set_float",
	   "Parameter %s is not a float", name.c_str(),
	   (!param || param->is_type(parameter_float)));

 if ( ! param ) {
   param = new Param;
   new_param_ (parameter_name_(path,name),param);
 }

 param->set_float_(value);
 monitor_write_(path,name);
}

//----------------------------------------------------------------------

bool Parameters::value_logical 
( const parameter_path_type & path,
  const parameter_name_type & name,
  bool        deflt ) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   deflt     Default parameter value
/// @return  Return logical parameter value if it exists, deflt if not
{
  Param * param = this->param(path,name);

  ASSERT2 ("Parameters::value_logical",
	   "Parameter %s is type %d not a logical",
	   name.c_str(),param->type(),
	   ( ! param || param->is_type(parameter_logical)));

  char deflt_string[MAX_PARAMETER_FILE_WIDTH];
  snprintf (deflt_string,MAX_PARAMETER_FILE_WIDTH,
            "%s",deflt ? "true" : "false");
  monitor_access_(path,name,deflt_string);
  return (param != NULL) ? param->get_logical() : deflt;
}

//----------------------------------------------------------------------

void Parameters::set_logical
(
  const parameter_path_type & path,
  const parameter_name_type & name,
  bool        value ) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   value     Value to set the parameter
{
  Param * param = this->param(path,name);

  ASSERT1 ("Parameters::set_logical",
	   "Parameter %s is not logical", name.c_str(),
	   (!param || param->is_type(parameter_logical)));

  if ( ! param ) {
    param = new Param;
    new_param_ (parameter_name_(path,name),param);
  }

  param->set_logical_(value);
  monitor_write_(path,name);
}

//----------------------------------------------------------------------

std::string Parameters::value_string 
( const parameter_path_type & path,
  const parameter_name_type & name,
  std::string deflt ) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   deflt     Default parameter value
/// @return  Return string parameter value if it exists, deflt if not
{
  Param * param = this->param(path,name);

  ASSERT1 ("Parameters::value_string",
	   "Parameter %s is not a string", name.c_str(),
	   ( ! param || param->is_type(parameter_string)));

  monitor_access_(path,name,deflt);
  return (param != NULL) ? param->get_string() : deflt;
}

//----------------------------------------------------------------------

void Parameters::set_string
( const parameter_path_type & path,
  const parameter_name_type &  name,
  const char * value ) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   value     Value to set the parameter
{
  Param * param = this->param(path,name);

  ASSERT1 ("Parameters::set_string_string",
	   "Parameter %s is not a string", name.c_str(),
	   ( ! param || param->is_type(parameter_string)));

  if ( ! param ) {
    param = new Param;
    new_param_ (parameter_name_(path,name),param);
  }

  param->set_string_(strdup(value));
  monitor_write_(path,name);
}

//----------------------------------------------------------------------

void Parameters::evaluate_float 
(
 const parameter_path_type & path,
 const parameter_name_type & name,
 int         n, 
 double    * result, 
 double    * deflt,
 double    * x, 
 double    * y, 
 double    * z, 
 double    t) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   n         Length of variable arrays
/// @param   result    Output array of evaluated floating point parameters values if it exists, or deflt if not
/// @param   deflt     Array of default values
/// @param   x         Array of x values
/// @param   y         Array of y values
/// @param   z         Array of z values
/// @param   t         t value
{
  Param * param = this->param(path,name);
  ASSERT1 ("Parameters::evaluate_float",
	   "Parameter %s is not a floating-point expression", name.c_str(),
	   ( ! param || param->is_type(parameter_float_expr)));
  if (param != NULL) {
    param->evaluate_float(n,result,x,y,z,t);
  } else {
    for (int i=0; i<n; i++) result[i] = deflt[i];
  }
}

//----------------------------------------------------------------------

void Parameters::evaluate_logical 
(
 const parameter_path_type & path,
 const parameter_name_type & name,
 int         n, 
 bool      * result, 
 bool      * deflt,
 double    * x, 
 double    * y, 
 double    * z, 
 double    t) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   n         Length of variable arrays
/// @param   result    Output array of evaluated logical parameters values if it exists, or deflt if not
/// @param   deflt     Array of default values
/// @param   x         Array of X values
/// @param   y         Array of Y values
/// @param   z         Array of Z values
/// @param   t         T value
{
  Param * param = this->param(path,name);
  ASSERT1 ("Parameters::evaluate_logical",
	   "Parameter %s is not a logical expression", name.c_str(),
	   (! param || param->is_type(parameter_logical_expr)));
  if (param != NULL) {
    param->evaluate_logical(n,result,x,y,z,t);
  } else {
    WARNING("Parameters::evaluate_logical",
	    "param is NULL but deflt not set");
  //   for (int i=0; i<n; i++) result[i] = deflt[i];
  }
}

//----------------------------------------------------------------------

int Parameters::list_length
(const parameter_path_type & path,
 const parameter_name_type & name)
/// @param   path      Parameter group path
/// @param   name      Parameter name
{
  Param * param = this->param(path,name);
  ASSERT1 ("Parameters::list_length",
	   "Parameter %s is not a list", full_name(path,name).c_str(),
	   ( ! param || param->is_type(parameter_list)));
  return (param != NULL) ? (param->value_list_)->size() : 0;
}

//----------------------------------------------------------------------

int Parameters::list_value_integer 
( int index,
  const parameter_path_type & path,
  const parameter_name_type & name,
  int         deflt ) throw()
/// @param   path      Parameter group path
/// @param   index     Index of the integer list parameter element
/// @param   name      Parameter name
/// @param   deflt     Default parameter value
/// @return  Return integer list parameter element value if it exists, deflt if not
{
  Param * param = this->param(path,name,index);
  ASSERT2 ("Parameters::list_value_integer",
	   "Parameter %s[%d] is not an integer", 
	   name.c_str(),index,
	   ( ! param || param->is_type(parameter_integer)));
  char deflt_string[MAX_PARAMETER_FILE_WIDTH];
  snprintf (deflt_string,MAX_PARAMETER_FILE_WIDTH,"%d",deflt);
  monitor_access_(path,name,deflt_string,index);
  return (param != NULL) ? param->value_integer_ : deflt;
}

//----------------------------------------------------------------------

double Parameters::list_value_float 
( int index,
  const parameter_path_type & path,
  const parameter_name_type & name,
  double      deflt ) throw()
/// @param   index     Index of the floating point (double) list parameter element
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   deflt     Default parameter value
/// @return  Return floating point (double) list parameter element value if it exists, deflt if not
{
  Param * param = this->param(path,name,index);
  ASSERT2 ("Parameters::list_value_float",
	   "Parameter %s[%d] is not a float", 
	   name.c_str(),index,
	   ( ! param || param->is_type(parameter_float)));
  char deflt_string[MAX_PARAMETER_FILE_WIDTH];
  // '#' format character forces a decimal point
  snprintf (deflt_string,MAX_PARAMETER_FILE_WIDTH,FLOAT_FORMAT,deflt);
  monitor_access_(path,name,deflt_string,index);
  return (param != NULL) ? param->value_float_ : deflt;
}

//----------------------------------------------------------------------

bool Parameters::list_value_logical 
( int index,
  const parameter_path_type & path,
  const parameter_name_type & name,
  bool        deflt ) throw()
/// @param   index     Index of the logical list parameter element
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   deflt     Default parameter value
/// @return  Return logical list parameter element value if it exists, deflt if not
{
  Param * param = this->param(path,name,index);
  ASSERT2 ("Parameters::list_value_logical",
	   "Parameter %s[%d] is not a logical", 
	   name.c_str(),index,
	   ( ! param || param->is_type(parameter_logical)));
  char deflt_string[MAX_PARAMETER_FILE_WIDTH];
  snprintf (deflt_string,MAX_PARAMETER_FILE_WIDTH,
            "%s",deflt ? "true" : "false");
  monitor_access_(path,name,deflt_string,index);
  return (param != NULL) ? param->value_logical_ : deflt;
}

//----------------------------------------------------------------------

std::string Parameters::list_value_string 
( int index,
  const parameter_path_type & path,
  const parameter_name_type & name,
  std::string deflt ) throw()
/// @param   index     Index of the string list parameter element
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   deflt     Default parameter value
/// @return  Return string list parameter element value if it exists, deflt if not
{
  Param * param = this->param (path,name,index);
  ASSERT2 ("Parameters::list_value_string",
	   "Parameter %s[%d] is not a string",
	   name.c_str(),index,
	   ( ! param || param->is_type(parameter_string)));
  monitor_access_(path,name,deflt,index);
  return (param != NULL) ? param->value_string_ : deflt;
}

//----------------------------------------------------------------------

void Parameters::set_list_length 
(
 const parameter_path_type & path,
 const parameter_name_type & name,
 int         length
 ) throw()
{
  Param * param = this->param(path,name);

  ASSERT1 ("Parameters::set_list_length",
	   "Parameter %s is not a list", name.c_str(),
	   ( ! param || param->is_type(parameter_list)));

  if ( ! param ) {
    param = new Param;
    new_param_ (parameter_name_(path,name),param);
    param->type_ = parameter_list;
    typedef std::vector<class Param *> list_type;
    param->value_list_ = new list_type;
  }
  param->value_list_->resize(length,0);
  for (int i =0; i<length; i++) {
    if ((*(param->value_list_))[i]==0)
      (*(param->value_list_))[i]=new Param;
  }
}

//----------------------------------------------------------------------

void Parameters::set_list_integer 
(
 int         index,
 const parameter_path_type & path,
 const parameter_name_type & name,
 int         value
) throw()
{
  Param * param = this->param(path,name,index);

  if ( ! param ) {
    param = new Param;
    new_param_ (parameter_name_(path,name),param);
  }

  param->set_integer_(value);
}

//----------------------------------------------------------------------


void Parameters::set_list_float
(
 int         index,
 const parameter_path_type & path,
 const parameter_name_type & name,
 double      value
) throw()
{

  Param * param = this->param(path,name,index);

  if ( ! param ) {
    param = new Param;
    new_param_ (parameter_name_(path,name),param);
  }

  param->set_float_(value);
}

//----------------------------------------------------------------------


void Parameters::set_list_logical
(
 int         index,
 const parameter_path_type & path,
 const parameter_name_type & name,
 bool        value
) throw()
{
  Param * param = this->param(path,name,index);

  if ( ! param ) {
    param = new Param;
    new_param_ (parameter_name_(path,name),param);
  }

  param->set_logical_(value);
}

//----------------------------------------------------------------------

void Parameters::set_list_string
(
 int          index,
 const parameter_path_type & path,
 const parameter_name_type &  name,
 const char * value
) throw()
{
  Param * param = this->param(path,name,index);

  if ( ! param ) {
    param = new Param;
    new_param_ (parameter_name_(path,name),param);
  }

  param->set_string_(strdup(value));
}

//----------------------------------------------------------------------

void Parameters::list_evaluate_float 
(
 int index,
 const parameter_path_type & path,
 const parameter_name_type & name,
 int         n, 
 double    * result, 
 double    * deflt,
 double    * x, 
 double    * y, 
 double    * z, 
 double    t
 ) throw()
/// @param   index     Index into the list
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   n         Length of variable arrays
/// @param   result    Output array of evaluated floating point expression list parameter element values if it exists, or deflt if not
/// @param   deflt     Array of default values
/// @param   x         Array of X values
/// @param   y         Array of Y values
/// @param   z         Array of Z values
/// @param   t         T value
{

  Param * param = this->param(path,name,index);
  ASSERT2 ("Parameters::list_evaluate_float",
	   "Parameter %s[%d] is not a floating-point expression",
	   name.c_str(),index,
	   ( ! param || param->is_type(parameter_float_expr)));
  if (param != NULL) {
    param->evaluate_float(n,result,x,y,z,t);
  // } else {
  //   for (int i=0; i<n; i++) result[i] = deflt[i];
  }
}

//----------------------------------------------------------------------

void Parameters::list_evaluate_logical 
(
 int index,
 const parameter_path_type & path,
 const parameter_name_type & name,
 int         n, 
 bool      * result, 
 bool      * deflt,
 double    * x, 
 double    * y, 
 double    * z, 
 double    t) throw()
/// @param   index     Index into the list
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @param   n         Length of variable arrays
/// @param   result    Output array of evaluated logical expression list parameter element values if it exists, or deflt if not
/// @param   deflt     Array of default values
/// @param   x         Array of X values
/// @param   y         Array of Y values
/// @param   z         Array of Z values
/// @param   t         Array of T values
{
  Param * param = this->param(path,name,index);
  ASSERT2 ("Parameters::list_evaluate_logical",
	   "Parameter %s[%d] is not a logical",
	   name.c_str(),index,
	   ( ! param || param->is_type(parameter_logical_expr)));
  if (param != NULL) {
    param->evaluate_logical(n,result,x,y,z,t);
  } else {
    WARNING("Parameters::list_evaluate_logical",
	    "param is NULL but deflt not set");
  //   for (int i=0; i<n; i++) result[i] = deflt[i];
  }
}

//----------------------------------------------------------------------

std::vector<std::string> Parameters::value_full_strlist
(const parameter_path_type & path,
 const parameter_name_type & name, bool coerce_string_to_list,
 bool suppress_err) throw()
{
  int type = this->type(path,name);

  if (this->param(path,name) == nullptr){
    return {};
  } else if (type == parameter_list) {
    const int len = this->list_length(path,name);
    if (len == 0) return {};

    std::vector<std::string> out(len);
    for (int i=0; i<len; i++) { out[i] = this->value(i, path,name, "none"); }
    return out;

  } else if ((type == parameter_string) && coerce_string_to_list) {
    return { this->value_string(path,name,"none") };
  } else if (suppress_err) {
    return {};
  }

  const char* type_description = (coerce_string_to_list) ?
    "single string or list of strings" : "list of strings";
  ERROR3("Parameters::value_full_strlist",
         ("Parameter \"%s\" expects to be assigned a %s; it was instead "
          "assigned a value of type %d"),
         name.c_str(), type_description, type);
}

//----------------------------------------------------------------------

namespace { // define function local to this file

void validate_abs_group_name_(const char* func_name, const std::string& name)
{
  const std::size_t size = name.size();

  ASSERT(func_name, "\"\" is an invalid absolute group name", size > 0);
  ASSERT1(func_name, "\"%s\" is an invalid group name: contains whitespace",
          name.c_str(), name.find_first_of(" \t\n") == std::string::npos);
  ASSERT1(func_name, "%s is an invalid absolute group name: starts with ':'",
          name.c_str(), name[0] != ':');

  std::size_t grp_start = 0;
  while (grp_start < size) {
    std::size_t rslt = name.find(':', grp_start);
    std::size_t grp_stop = (rslt != std::string::npos) ? rslt : size;
    ASSERT1(func_name,
            "%s is an invalid group name: it has multiple colons in a row",
            name.c_str(), grp_stop > grp_start);

    // grp_stop specifies the location of a colon or is equal to size
    grp_start = grp_stop + 1;
  }
}

} // end anonymous namespace

//----------------------------------------------------------------------

const parameter_path_type Parameters::leaf_parameter_names
(const parameter_name_type & full_group_name) const throw()
{
  // validate full_group_name
  validate_abs_group_name_("Parameters::leaf_parameter_names",
                           full_group_name);

  std::string prefix;
  if (full_group_name[full_group_name.size()-1] == ':') { // size always > 0
    prefix = full_group_name;
  } else {
    prefix = full_group_name + ':';
  }

  std::vector<std::string> out;
  std::size_t prefix_size = prefix.size();

  for (auto it_param =  parameter_map_.lower_bound(prefix);
       it_param != parameter_map_.end();
       ++it_param) {

    // If the current key doesn't share the prefix, abort the search
    if (((it_param->first).compare(0, prefix_size, prefix) != 0) ||
	((it_param->first).size() <= prefix_size) ) {
      break;
    }

    ASSERT1("Parameters::leaf_parameter_names",
            "parameter \"%s\" associated with a nullptr. This isn't allowed",
            (it_param->first).c_str(), (it_param->second != nullptr));

    std::string suffix = (it_param->first).substr(prefix_size,
						  std::string::npos);
    if (suffix.find(':') != std::string::npos){
      // this isn't a leaf parameter
      continue;
    } else {
      out.push_back(suffix);
    }
  }
  return out;
}

//----------------------------------------------------------------------

const parameter_path_type Parameters::leaf_parameter_names(const parameter_path_type & path) const throw()
{
  std::string full_group_name = full_name(path,"");
  ASSERT("Parameters::leaf_parameter_names",
         "the version of this method that returns the leaf parameter names in "
         "the current parameter-group can't be called when the current "
         "parameter-group is empty",
         full_group_name != ":");
  return leaf_parameter_names(full_group_name);
}

//----------------------------------------------------------------------

const parameter_name_type Parameters::full_name(const parameter_path_type & path,
                                  const parameter_name_type & name) const throw()
{
  int n = path.size();
  std::string full_name = "";
  for (int i=0; i<n; i++) {
    full_name = full_name + path[i] + ":";
  }
  full_name = full_name + name;
  return full_name;

}
//----------------------------------------------------------------------

parameter_type Parameters::type
(const parameter_path_type & path,
 const parameter_name_type & name) throw()
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @return  Return type of the given parameter
{
  Param * param = this->param(path,name);
  return param ? param->type() : parameter_unknown ;
}

//----------------------------------------------------------------------

parameter_type Parameters::list_type
( 
 int index,
 const parameter_path_type & path,
 const parameter_name_type & name
  ) throw()
/// @param   index     Index into the list
/// @param   path      Parameter group path
/// @param   name      Parameter name
/// @return  Return type of the given parameter
{
  Param * list = this->param(path,name);
  Param * param = NULL;
  if (list != NULL) {
    int list_length = list->value_list_->size();
    if (list != NULL && 0 <= index && index < list_length ) {
      param =  (*(list->value_list_))[index];
    }
  }
  return param ? param->type() : parameter_unknown ;
}

//======================================================================

void Parameters::monitor_access_ 
(
 const parameter_path_type & path,
 const parameter_name_type & name,
 std::string deflt_string,
 int index
 ) throw()
{
  if (! lmonitor_) return;

  Param * param = 0;

  if (index == -1) {
    // not a list element
    param = this->param(path,name);
  } else {
    // a list element
    param = this->param(path,name,index);
  }

  std::string value;

  if ( param != NULL ) {
    value = param->value_to_string(param_write_monitor);
  } else {
    value = deflt_string + " [default]";
  }

  std::string index_string = 
    std::string("[") + std::to_string(index) + std::string("]");

  std::string buffer = std::string("accessed ") +
    parameter_name_(path,name) + index_string + " " + value;

  if (lmonitor_) monitor_->print_verbatim("Parameters",buffer.data());
}

//----------------------------------------------------------------------

void Parameters::monitor_write_
( const parameter_path_type & path,
  const parameter_name_type & name) throw()
{
  Param * param = this->param(path,name);
  char buffer[MONITOR_LENGTH];
  snprintf (buffer,MONITOR_LENGTH,"Parameter write %s = %s",
	   parameter_name_(path,name).c_str(),
	   param ?
	   param->value_to_string(param_write_monitor).c_str() : "[undefined]");

  if (lmonitor_) monitor_->print_verbatim("Parameters",buffer);
}

//----------------------------------------------------------------------

int Parameters::readline_ 
( FILE*  fp, 
  char * buffer, 
  int    buffer_length ) 
  throw()
/// @param   fp                 the opened input file
/// @param   buffer             a character string buffer
/// @param   buffer_length      size of the buffer
/// @return  Whether the end of the file has been reached
{

  int i;

  // Clear the line buffer

  for (i=0; i<buffer_length; i++) {
    buffer[i]=0;
  }

  // Read the next line into the buffer

  int c = 0;
  for (i=0; c != EOF && c != '\n' && i < buffer_length-1; i++) {
    c = fgetc(fp);
    buffer[i] = (std::numeric_limits<char>::min() <= c && 
		 c <= std::numeric_limits<char>::max()) ? c : '\0';
  }

  // Back up i to last character read
  i--;

  // Check for buffer overrun

  ASSERT ("Parameters::readline_",
	  "Input file line is too long",
	  (i+1 >= buffer_length-1));

  // Convert the buffer into a C string

  if (buffer[i] == '\n') buffer[i] = '\0';

  // Return whether or not the end-of-file has been reached

  return (c != EOF);

}

//----------------------------------------------------------------------

/// Return the Param pointer for the specified parameter
Param * Parameters::param
( const parameter_path_type & path,
  const parameter_name_type & name, int index)
{
  if (index == -1) {
    std::string full_name = parameter_name_(path,name);
    // this branch was previously equivalent to:
    //     return parameter_map_[full_name];
    // however, when full_name wasn't a key within parameter_map_, that
    // implicity inserted the key-value pair (full_name, nullptr) within
    // parameter_map_
    //
    // The modern implementation avoids mutating parameter_map_
    auto search = parameter_map_.find(full_name);
    return (search != parameter_map_.end()) ? search->second : nullptr;
  } else {
    Param * list = this->param(path,name);
    Param * param = NULL;
    if (list != NULL) {
      list->set_accessed();
      int list_length = list->value_list_->size();
      if (list != NULL && 0 <= index && index < list_length ) {
	param =  (*(list->value_list_))[index];
      }
    }
    return param;
  }
}

//----------------------------------------------------------------------

size_t Parameters::extract_groups_
(
 const parameter_name_type & name, 
 std::string * group
 )
{
  std::string p = name;
  size_t i_group=0;  // group index for group[]
  size_t i_stop  = p.find(":");
  while (i_stop != std::string::npos &&
	 i_group<MAX_GROUP_DEPTH) {
    group[i_group++] = p.substr(0,i_stop);
    p = p.substr(i_stop+1,std::string::npos);
    i_stop  = p.find(":");
  }
  return i_group;
}

//----------------------------------------------------------------------

void Parameters::new_param_
(
 const parameter_name_type & full_parameter,
 Param * param
 ) throw()
{
  parameter_map_ [full_parameter] = param;

  std::string groups[MAX_GROUP_DEPTH];

  int num_groups = extract_groups_(full_parameter,groups);

  ParamNode * param_node = &parameter_tree_;
  for (int i=0; i<num_groups; i++) {
    param_node = param_node->new_subnode(groups[i]);
  }
  size_t ic = full_parameter.rfind(":");
  std::string parameter = full_parameter;
  if (ic != std::string::npos) {
    parameter = full_parameter.substr(ic+1,std::string::npos);
  }
  param_node = param_node->new_subnode(full_parameter);
}

//----------------------------------------------------------------------

void Parameters::check() const
{
  for (auto it_param =  parameter_map_.begin();
       it_param != parameter_map_.end();
       ++it_param) {
    ASSERT1("Parameters::check",
            "parameter \"%s\" associated with a nullptr. This isn't allowed",
            (it_param->first).c_str(), (it_param->second != nullptr));
    if (! it_param->second->accessed()) {
      WARNING1 ("Parameters::check()",
		"Parmeter \"%s\" not accessed",
		it_param->first.c_str());
    }
  }
}

//----------------------------------------------------------------------
