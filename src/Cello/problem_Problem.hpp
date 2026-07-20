// See LICENSE_CELLO file for license and copyright information

/// @file     problem_Problem.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2012-03-02
/// @brief    [\ref Problem] Declaration of the Problem class
///
/// This class is used as a container for classes that define the
/// problem being solved.  These classes include the following:
///
///    Boundary:    Boundary conditions
///    Initial:     Initial conditions
///    Physics:     Physics related objects (e.g. cosmology)
///    Refine:      Refinement criteria
///    Solver:      List of linear solvers
///    Method:      List of physics methods
///    Refresh:     List of ghost zone refresh objects
///    Output:      List of output functions
///    Refinement:  How the mesh hierarchy is to be refined
///    Stopping:    Stopping criteria
///    Units:       Physical units

#ifndef PROBLEM_PROBLEM_HPP
#define PROBLEM_PROBLEM_HPP

class Boundary;
class Factory;
class Initial;
class Physics;
class Input;
class Method;
class Output;
class Parameters;
class ParticleDescr;
class Prolong;
class Refine;
class Refresh;
class Restrict;
class Simulation;
class Solver;
class Stopping;
class Units;
class Compute;

class Problem : public PUP::able
{

  /// @class    Problem
  /// @ingroup  Problem
  /// @brief    [\ref Problem]

public: // interface

  /// Constructor
  Problem() throw();

  /// Destructor
  virtual ~Problem() throw();

  /// CHARM++ function for determining the specific class in the class hierarchy
  PUPable_decl(Problem);

  /// CHARM++ migration constructor for PUP::able

  Problem (CkMigrateMessage *m)
    : PUP::able(m),
      boundary_list_(),
      is_periodic_(false),
      initial_list_(),
      physics_list_(),
      refine_list_(),
      stopping_(nullptr),
      solver_list_(),
      method_list_(),
      output_list_(),
      prolong_list_(),
      restrict_list_(),
      units_(nullptr),
      index_refine_(0),
      index_output_(0),
      id_refresh_initial_(0)
  {}

  /// CHARM++ Pack / Unpack function
  void pup (PUP::er &p);

  /// Return the boundary object
  size_t num_boundary() const throw()
  { return boundary_list_.size(); }

  Boundary * boundary(int i) const throw()
  { return boundary_list_.at(i); }

  /// Return whether the problem is periodic
  bool is_periodic () const throw()
  { return is_periodic_; }

  size_t num_initial() const throw()
  { return initial_list_.size(); }
  /// Return the ith initialization object
  Initial * initial(int i) const throw()
  {  return initial_list_.at(i); }

  size_t num_physics() const throw()
  { return physics_list_.size(); }

  /// Return the ith physics object
  Physics * physics(int i) const throw()
  { return physics_list_.at(i); }
  /// Return the named physics object if present
  Physics * physics (std::string_view type) const throw();

  size_t num_refine() const throw()
  { return refine_list_.size(); }
  /// Return the ith refine object
  Refine * refine(int i) const throw()
  { return refine_list_.at(i); }

  size_t num_output() const throw()
  { return output_list_.size(); }
  /// Return the ith output object
  Output * output(int i) const throw()
  { return output_list_.at(i); }

  int num_solvers () const throw()
  { return solver_list_.size(); }
  /// Return the ith solver object
  Solver * solver(int i) const throw()
  { return solver_list_.at(i); }

  int num_methods () const throw()
  { return method_list_.size(); }
  /// Return the ith method object
  Method * method(int i) const throw()
  { return method_list_.at(i); }
  /// Return the named method object if present
  Method * method (std::string_view name) const throw();

  // Return whether a method object with given name exists for this problem
  bool method_exists(std::string_view name) const throw();

  /// Return the index of the method in the method list, or -1 if
  /// it's absent
  int method_index(std::string_view name) const throw();

  /// Return the number of times the given name is in the method list
  int method_count(std::string_view name) const throw();

  // Returns true iff named methods appear exactly once in the method
  // list and are in the specified order
  bool methods_in_order
  (std::string_view name1, std::string_view name2) const throw()
  {
    return ( (method_count(name1) == 1) &&
             (method_count(name2) == 1) &&
             (method_index(name1) < method_index(name2)) );
  }

  size_t num_prolong() const throw()
  { return prolong_list_.size(); }

  /// Return the ith prolong object
  Prolong * get_prolong(int i = 0) const throw()
  { return prolong_list_.at(i); }

  size_t num_restrict() const throw()
  { return restrict_list_.size(); }

  /// Return the ith restrict object
  Restrict * get_restrict(int i = 0) const throw()
  { return restrict_list_.at(i); }

  //--------------------------------------------------
  // OUTPUT
  //--------------------------------------------------

  /// reset output index to 0
  void output_reset() throw()
  { index_output_ = -1; }

  /// Process the next output object if any, else proceed with simulation
  void output_next(Simulation * simulation) throw();

  /// Reduce output, using p_output_write to send data to writing processes
  void output_wait(Simulation * simulation) throw();

  /// Receive data from non-writing process, write to disk, close, and
  /// proceed with next output
  void output_write (Simulation * simulation, int n, char * buffer) throw();

  /// Return the stopping object
  Stopping * stopping() const throw() { return stopping_; }

  /// Return the Units object
  Units * units() const throw() { return units_; }

  /// Initialize the boundary conditions object
  void initialize_boundary(Config * config,
			   Parameters * parameters) throw();

  /// Initialize the initial conditions object
  void initialize_initial(Config * config,
			  Parameters * parameters) throw();

  /// Initialize any physics-related objects
  void initialize_physics(Config * config,
			  Parameters * parameters) throw();

  /// Initialize the refine object
  void initialize_refine(Config * config,
			 Parameters * parameters) throw();

  /// Initialize the stopping object
  void initialize_stopping(Config * config ) throw();

  /// Initialize the output objects
  void initialize_output(Config * config,
			 const Factory * factory) throw();

  /// Initialize the method objects
  void initialize_method(Config * config,
			 const Factory * factory) throw();

  /// Initialize Solver objects
  void initialize_solver(Config * config) throw();

  /// Initialize the prolong objects
  void initialize_prolong(Config * config) throw();

  /// Initialize the restrict objects
  void initialize_restrict(Config * config) throw();

  /// Initialize the units object
  void initialize_units(Config * config ) throw();

  int new_prolong (std::string type) throw();
  int new_restrict (std::string type) throw();

  /// Create named compute object
  virtual Compute *  create_compute
  (std::string_view type,
   Config * config) throw();

  int id_refresh_initial () const
  { return id_refresh_initial_; }

protected: // functions

  /// Deallocate components
  void deallocate_() throw();

  /// Create named boundary object
  virtual Boundary * create_boundary_
  (std::string_view type,
   int index,
   Config * config,
   Parameters * parameters
   ) throw ();

  /// Create named initialization object
  virtual Initial *  create_initial_
  (std::string_view type,
   int index,
   Config * config,
   Parameters * parameters) throw ();

  /// Create named physics object
  virtual Physics *  create_physics_
  (std::string_view type,
   int index,
   Config * config,
   Parameters * parameters) throw ();

  /// Create named stopping object
  virtual Stopping * create_stopping_
  (std::string_view type, Config * config) throw ();

  /// Create named refine object
  virtual Refine * create_refine_
  (std::string_view type,
   int index,
   Config * config,
   Parameters * parameters) throw ();

  /// Create named solver object
  virtual Solver *   create_solver_
  (std::string_view type,
   int index_solver,
   Config * config) throw ();

  /// Create named method object
  virtual Method *   create_method_
  (std::string_view type,
   int index_method,
   Config * config,
   const Factory * factory) throw ();

  /// Create named output object
  virtual Output *   create_output_
  (std::string_view type,
   int index,
   Config * config,
   const Factory * ) throw ();

  /// Create named prolongation object
  virtual Prolong * create_prolong_ (std::string_view type) throw ();

  /// Create named restrictation object
  virtual Restrict * create_restrict_ (std::string_view type) throw ();

  /// Create named units object
  virtual Units * create_units_ (Config * config) throw ();

  /// Method that gets called at the end of initialize_physics
  virtual void initialize_physics_coda_(Config * config,
                                        Parameters * parameters) throw()
  { }

protected: // attributes

  /// Boundary conditions object for each (axis,face)
  std::vector<Boundary *> boundary_list_;

  /// Whether the problem is fully periodic or not
  bool is_periodic_;

  /// Initial conditions objects
  std::vector<Initial *> initial_list_;

  /// Physics objects
  std::vector<Physics *> physics_list_;

  /// Refinement criteria objects
  std::vector<Refine *> refine_list_;

  /// Stopping criteria
  Stopping * stopping_;

  /// List of solver objects
  std::vector<Solver *> solver_list_;

  /// List of method objects
  std::vector<Method *> method_list_;

  /// Output objects
  std::vector<Output *> output_list_;

  /// Prolongation object
  std::vector<Prolong *> prolong_list_;

  /// Restriction object
  std::vector<Restrict *> restrict_list_;

  /// Units
  Units * units_;

  /// Index of currently active Refine object
  int index_refine_;

  /// Index of currently active Output object (may be -1)
  int index_output_;

  /// Id of initial refresh
  int id_refresh_initial_;
};

#endif /* PROBLEM_PROBLEM_HPP */

