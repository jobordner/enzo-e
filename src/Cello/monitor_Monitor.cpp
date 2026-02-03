// See LICENSE_CELLO file for license and copyright information

/// @file      monitor_Monitor.cpp
/// @author    James Bordner (jobordner@ucsd.edu)
/// @date      Thu Feb 21 12:45:56 PST 2009
/// @brief     Routines for simple output of text, plots, and graphs

#include "cello.hpp"

#include "monitor.hpp"
#include "parameters.hpp"

#include "../../auto_config.def"

//----------------------------------------------------------------------
Monitor Monitor::instance_[CONFIG_NODE_SIZE]; // singleton design pattern)
//----------------------------------------------------------------------

Monitor::Monitor()
  : timer_(new Timer),
    mode_(monitor_mode_root),
    group_default_(monitor_mode_all),
    include_proc_(true),
    include_time_(true),
    level_(2),
    schedule_(nullptr),
    cycle_(0),
    time_(0.0),
    mute_set_(),
    only_set_()

{
  timer_->start();
}

//----------------------------------------------------------------------

Monitor::~Monitor()
{
  delete timer_;
  timer_ = 0;
}

//----------------------------------------------------------------------

void Monitor::header () const
{
  if (level_ == 0) return;
  print ("","==============================================");
  print (""," ");
  print ("","  .oooooo.             oooo  oooo");
  print (""," d8P'  `Y8b            `888  `888");
  print ("","888           .ooooo.   888   888   .ooooo.");
  print ("","888          d88' `88b  888   888  d88' `88b");
  print ("","888          888ooo888  888   888  888   888");
  print ("","`88b    ooo  888    .o  888   888  888   888");
  print (""," `Y8bood8P'  `Y8bod8P' o888o o888o `Y8bod8P'");
  print (""," ");
  print ("","A Parallel Adaptive Mesh Refinement Framework");
  print (""," ");
  print ("","  Laboratory for Computational Astrophysics");
  print ("","        San Diego Supercomputer Center");
  print ("","     University of California, San Diego");
  print (""," ");
  print ("","See 'LICENSE_CELLO' for software license information");
  print ("","");

  // Get date text

  time_t rawtime;
  struct tm * t;
  time(&rawtime);
  t = localtime (&rawtime);
  const char * month[] =
    {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

  print ("","BEGIN CELLO: %s %02d %02d:%02d:%02d",
	 month[t->tm_mon],
	 t->tm_mday,
	 t->tm_hour,
	 t->tm_min,
	 t->tm_sec);
  print ("Input","File name            %s", cello::parameters()->file_name().c_str());
  // Print all recognized configuration settings

  print ("Define","Simulation processors %d",CkNumPes());

  // Parallel type defines

  print ("Define","CELLO_PREC          %s",CELLO_PREC);

  print ("Define","CC                  %s",CELLO_CC);
  print ("Define","CFLAGS              %s",CELLO_CFLAGS);
  print ("Define","CPPDEFINES          %s",CELLO_CPPDEFINES);
  print ("Define","CXX                 %s",CELLO_CXX);
  print ("Define","CXXFLAGS            %s",CELLO_CXXFLAGS);
  print ("Define","FORTRANFLAGS        %s",CELLO_FORTRANFLAGS);
  print ("Define","FORTRAN             %s",CELLO_FORTRAN);
  // print ("Define","FORTRANLIBS         %s",CELLO_FORTRANLIBS);
  // print ("Define","LIBPATH             %s",CELLO_LIBPATH);
  // print ("Define","LINKFLAGS           %s",CELLO_LINKFLAGS);
  print ("Define","BUILD HOST          %s",CELLO_HOST);
  print ("Define","BUILD DIR           %s",CELLO_DIR);
  // print ("Define","BUILD DATE (UTC)    %s",CELLO_DATE);
  // print ("Define","BUILD TIME (UTC)    %s",CELLO_TIME);
  print ("Define","CELLO_VERSION       %s", CELLO_VERSION);
#ifdef CONFIG_HAVE_VERSION_CONTROL
  print ("Define","CHANGESET           %s",CELLO_CHANGESET);
  print ("Define","BRANCH              %s",CELLO_BRANCH);
#endif
  print ("Define","CHARM_PATH          %s",CHARM_PATH);
  // print ("Define","CHARM_VERSION       %d",CHARM_VERSION);
  // print ("Define","CHARM_BUILD         %s",CHARM_BUILD);
  print ("Define","CONFIG_NODE_SIZE    %d",CONFIG_NODE_SIZE);
#ifdef CONFIG_SMP_MODE
  print ("Define","CONFIG_SMP_MODE     %s","Yes");
#else
  print ("Define","CONFIG_SMP_MODE     %s","no");
#endif
  print ("CHARM","CkNumPes()           %d",CkNumPes());
  print ("CHARM","CkNumNodes()         %d",CkNumNodes());
  print ("CHARM","CkNumHosts()         %d",CmiNumPhysicalNodes());
}

//----------------------------------------------------------------------

int Monitor::is_active(const char * component) const throw ()
{
  // Return false if component is inactive

  int component_active = mute_set_.find(std::string(component)) == mute_set_.end();
  if (!only_set_.empty())
    component_active = only_set_.find(std::string(component)) != only_set_.end();

  if (! component_active) return false;

  // Return false if not scheduled

  bool is_scheduled = (schedule_ && 
		     schedule_->write_this_cycle(cycle_,time_));

  if (schedule_ && !is_scheduled) return false;

  // Return false if only writing from ip 0

  if (mode_ == monitor_mode_root && CkMyPe() != 0)
    return false;

  return true;
}

//----------------------------------------------------------------------

void Monitor::write
( FILE * fp, const char * component, const char * format,  ... ) const
{

  if (is_active(component)) {

    va_list fargs;

    // Process any input arguments

    char message[MONITOR_LENGTH+1];

    va_start(fargs,format);
    vsnprintf (message,MONITOR_LENGTH, format,fargs);
    va_end(fargs);

    write_ (fp, component,message);
  }
}

//----------------------------------------------------------------------

void Monitor::write_ (FILE * fp, const char * component, const char * message) const
{

  // Get parallel process text

  char process[MONITOR_LENGTH] = "";

  if (include_proc_) snprintf (process,MONITOR_LENGTH,"%0d ",CkMyPe());

  // Get time

  char time[10] = "";

  if (include_time_) snprintf (time,10,"%08.2f ",timer_->value());

  // Print

  const char newline = (strcmp(message,"")==0) ? ' ' : '\n';
  if (fp == stdout) {
    PARALLEL_PRINTF
      ("%s%s%s %s%c",     process, time, component, message,newline);
  } else {
    fprintf
      (fp,"%s%s%s %s%c",  process, time, component, message,newline);
  }
}

//----------------------------------------------------------------------

void Monitor::write_verbatim
(
 FILE * fp,
 const char * component,
 const char * message
 ) const
{
  if (is_active(component)) {

    // Get parallel process text

    char buffer_process[MONITOR_LENGTH] = "";

    snprintf (buffer_process,MONITOR_LENGTH,"%0d",CkMyPe());

    // Get time

    char buffer_time[10];

    snprintf (buffer_time,10,"%08.2f",timer_->value());

    // Print

    if (fp == stdout) {
      PARALLEL_PRINTF
	("%s %s %s %s\n",
	 buffer_process, buffer_time, component, message);
    } else {
      fprintf
	(fp,"%s %s %s %s\n",
	 buffer_process, buffer_time, component, message);
    }
  }

}

//----------------------------------------------------------------------

void Monitor::print (const char * component, const char * message, ...) const
{

  if (is_active(component)) {

    va_list fargs;

    // Process any input arguments

    char buffer_message[MONITOR_LENGTH+1];

    va_start(fargs,message);
    vsnprintf (buffer_message,MONITOR_LENGTH, message,fargs);
    va_end(fargs);

    write (stdout, component, buffer_message);
  }
}

//----------------------------------------------------------------------

void Monitor::print_verbatim (const char * component, const char * message) const
{
  write_verbatim (stdout, component, message);
}
