// See LICENSE_CELLO file for license and copyright information

//----------------------------------------------------------------------
/// @file     monitor_Monitor.hpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2009-10-05
/// @brief    [\ref Monitor] Declaration of the Monitor class
//----------------------------------------------------------------------

#ifndef MONITOR_MONITOR_HPP
#define MONITOR_MONITOR_HPP

#include "charm++.h"
#include <set>
//----------------------------------------------------------------------
/// @def    MONITOR_LENGTH
/// @brief  Maximum length of monitor text output

#define MONITOR_LENGTH 512
   
//----------------------------------------------------------------------
class Timer; 
class Schedule;
class Monitor {

  /// @class    Monitor
  /// @ingroup  Monitor
  /// @brief    [\ref Monitor] User monitoring of simulation execution status
  ///
  /// The Monitor component is used to communicate information about
  /// the running simulation to the user. Information can be output in
  /// several forms, including text files, HTML files, plots, or other
  /// (generally small) image files. Information is assumed to be from
  /// a correctly-running simulation: anomalous errors or warnings are
  /// output by the Error component.

  //----------------------------------------------------------------------

public:
  
  /// Private constructor of the Monitor object [singleton design pattern]
  /// (MADE PUBLIC FOR Parameters::write())
  Monitor();

  /// Private destructor  of the Monitor object [singleton design pattern]
  /// (MADE PUBLIC FOR Parameters::write())
  ~Monitor();

private:

  friend class Simulation;


//----------------------------------------------------------------------

public: // interface

  /// CHARM++ Pack / Unpack function
  inline void pup (PUP::er &p)
  {
    TRACEPUP;
    WARNING("Monitor::pup","Monitor pup() disabled");

    // // NOTE: change this function whenever attributes change
    // p |  *timer_;
    p | level_;
    p | mode_;
    p | schedule_;
    p | mute_set_;
    p | only_set_;
    p | cycle_;
    p | time_;
    // p |  group_default_;
    // p |  group_active_;

  }

  /// Return an instance of a Monitor object
  static Monitor * instance()
  { 
    return & instance_[cello::index_static()];
  };

  /// Set whether the monitor is active for text output.  Useful for
  /// parallel, e.g. "monitor->set_active(parallel->is_root())"
  void set_mode(int mode) { mode_ = mode; };

  /// Return whether monitoring is active
  int mode() const throw () { return mode_; };

  /// Return whether monitoring is active for this component
  int is_active(const char *) const throw ();

  /// Print the Cello header 
  void header () const;

  /// Write a message to file
  void write (FILE * fp, 
	      const char * component, const char * buffer, ...) const;

  /// Write a message to file
  void write_verbatim (FILE * fp, 
	      const char * component, const char * buffer) const;

  /// Print a message with possible format specifications to stdout
  void print (const char * component, const char * buffer, ...) const;

  const Schedule * schedule() const
  { return schedule_; }

  /// Print a message without format specifications to stdout
  void print_verbatim (const char * component, const char * buffer) const;

private: // functions

  void set_level_ (int level)
  { level_ = level; }

  void update_state_ (int cycle, double time)
  { cycle_ = cycle;
    time_ = time;}

  void set_schedule_ (Schedule * schedule)
  { schedule_ = schedule;; }

  void mute_component_(std::string component)
  {
    mute_set_.insert(component);
  }
  void only_component_(std::string component)
  {
    only_set_.insert(component);
  }

  void write_ (FILE * fp, const char * component, const char * buffer) const;

  //----------------------------------------------------------------------

private: // attributes

  /// Timer for keeping track of time for output
  Timer * timer_; 

  /// Parallel monitoring mode, either unknown, none, root, or all
  int mode_;

  /// Whether default is to output all groups or output no groups
  int group_default_;

  /// Level of output 0:none 1:low 2:medium [default] 3:high
  int level_;

  /// Output monitoring schedule
  Schedule * schedule_;
  /// Current state for cycle
  int cycle_;
  double time_;

  /// Don't output these components
  std::set<std::string> mute_set_;
  /// Only output these components
  std::set<std::string> only_set_;

  //----------------------------------------------------------------------

private: // static attributes

  /// Single instance of the Monitor object [singleton design pattern]
  // static Monitor * instance_;
  static Monitor instance_[CONFIG_NODE_SIZE];

};

#endif /* MONITOR_MONITOR_HPP */

