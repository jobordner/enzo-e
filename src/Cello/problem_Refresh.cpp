// See LICENSE_CELLO file for license and copyright information

/// @file     problem_Refresh.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2017-08-11
/// @brief    

#include "problem.hpp"

//----------------------------------------------------------------------

void Refresh::add_field(std::string field_name)
{
  const int id_field = cello::field_descr()->field_id(field_name);
  add_field(id_field);
}

//----------------------------------------------------------------------

void Refresh::add_field_src_dst(std::string field_src, std::string field_dst)
{
  const int id_field_src = cello::field_descr()->field_id(field_src);
  const int id_field_dst = cello::field_descr()->field_id(field_dst);
  add_field_src_dst(id_field_src,id_field_dst);
}

//----------------------------------------------------------------------

void Refresh::add_all_fields(std::string field_group)
{
  if (field_group == "") {
    all_fields_ = true;
  } else {
    Grouping * groups = cello::field_groups();
    int n = groups->size(field_group);
    for (int i=0; i<n; i++) {
      std::string field = groups->item(field_group,i);
      add_field(field);
    }
  }
}

//----------------------------------------------------------------------

std::vector<int> Refresh::field_list_src(int level, int face_type) const
{
  std::vector<int> field_list = field_list_src_;
  if (all_fields_) {
    int nf = cello::field_descr()->field_count();
    field_list.resize(nf);
    for (int i=0; i<nf; i++) {
      field_list[i] = i;
    }
  }
  include_history_fields_(field_list,face_type);
  return field_list;
}

//----------------------------------------------------------------------


std::vector<int> Refresh::field_list_dst(int level, int face_type) const
{
  std::vector<int> field_list = field_list_dst_;
  if (all_fields_) {
    int nf = cello::field_descr()->field_count();
    field_list.resize(nf);
    for (int i=0; i<nf; i++) {
      field_list[i] = i;
    }
  }
  include_history_fields_(field_list,face_type);
  return field_list;
}

//----------------------------------------------------------------------

void Refresh::add_particle(std::string particle_type)
{
  const int id_particle = cello::particle_descr()->type_index(particle_type);
  add_field(id_particle);
}
//----------------------------------------------------------------------

void Refresh::box_accumulate_adjust
(Box * box, int if3[3], int g3[3])
{
  if (accumulate_) {
    int gs3[3] = {0};
    for (int i=0; i<cello::rank(); i++) {
      gs3[i] = g3[i];
    }
    box->set_send_ghosts(gs3);
  }
}

//----------------------------------------------------------------------

int Refresh::coarse_padding(const Prolong * prolong_ptr) const
{
  const int pad = accumulate_ ? 0 : prolong_ptr->coarse_padding_();
  return pad;
}

//----------------------------------------------------------------------

Prolong * Refresh::get_prolong ()
{
  Problem * problem = cello::problem();
  Prolong * prolong_ptr = problem ?
    problem->get_prolong(id_prolong_) : nullptr;
  return prolong_ptr ? prolong_ptr : new ProlongLinear;

}

//----------------------------------------------------------------------

Restrict * Refresh::get_restrict ()
{
  Problem * problem = cello::problem();
  Restrict * restrict_ptr = problem ?
    problem->get_restrict(id_restrict_) : nullptr;
  return restrict_ptr ? restrict_ptr : new RestrictLinear;
}
  
//----------------------------------------------------------------------

int Refresh::data_size () const
{
  int count = 0;

  // WARNING: Skipping many fields since data methods are only called
  // when the Refresh object is a member of FieldFace, which in turn
  // only accesses field and particle lists and accumulate_

  SIZE_SCALAR_TYPE(count,int,all_fields_);
  SIZE_VECTOR_TYPE(count,int,field_list_src_);
  SIZE_VECTOR_TYPE(count,int,field_list_dst_);

  SIZE_SCALAR_TYPE(count,int,all_particles_);
  SIZE_SCALAR_TYPE(count,bool,particles_are_copied_);
  SIZE_VECTOR_TYPE(count,int,particle_list_);

  SIZE_SCALAR_TYPE(count,int,all_fluxes_);
  SIZE_SCALAR_TYPE(count,int,ghost_depth_);
  SIZE_SCALAR_TYPE(count,int,min_face_rank_);
  SIZE_SCALAR_TYPE(count,int,neighbor_type_);
  SIZE_SCALAR_TYPE(count,int,accumulate_);
  SIZE_SCALAR_TYPE(count,int,sync_type_);
  SIZE_SCALAR_TYPE(count,int,sync_id_);
  SIZE_SCALAR_TYPE(count,int,active_);
  SIZE_SCALAR_TYPE(count,int,callback_);
  SIZE_SCALAR_TYPE(count,int,level_);
  SIZE_SCALAR_TYPE(count,int,root_level_);
  SIZE_SCALAR_TYPE(count,bool,adaptive_timestep_);
  SIZE_SCALAR_TYPE(count,int,level_lower_);
  SIZE_SCALAR_TYPE(count,int,level_upper_);
  SIZE_SCALAR_TYPE(count,int,global_);
  SIZE_SCALAR_TYPE(count,bool,advanced_time_);
  
  SIZE_SCALAR_TYPE(count,int,id_refresh_);
  SIZE_SCALAR_TYPE(count,int,id_prolong_);
  SIZE_SCALAR_TYPE(count,int,id_restrict_);
  SIZE_SCALAR_TYPE(count,int,final_sync_);

  return count;

}

//----------------------------------------------------------------------

char * Refresh::save_data (char * buffer) const
{
  char * p = buffer;

  SAVE_SCALAR_TYPE(p,int,all_fields_);
  SAVE_VECTOR_TYPE(p,int,field_list_src_);
  SAVE_VECTOR_TYPE(p,int,field_list_dst_);

  SAVE_SCALAR_TYPE(p,int,all_particles_);
  SAVE_SCALAR_TYPE(p,bool,particles_are_copied_);
  SAVE_VECTOR_TYPE(p,int,particle_list_);

  SAVE_SCALAR_TYPE(p,int,all_fluxes_);
  SAVE_SCALAR_TYPE(p,int,ghost_depth_);
  SAVE_SCALAR_TYPE(p,int,min_face_rank_);
  SAVE_SCALAR_TYPE(p,int,neighbor_type_);
  SAVE_SCALAR_TYPE(p,int,accumulate_);
  SAVE_SCALAR_TYPE(p,int,sync_type_);
  SAVE_SCALAR_TYPE(p,int,sync_id_);
  SAVE_SCALAR_TYPE(p,int,active_);
  SAVE_SCALAR_TYPE(p,int,callback_);
  SAVE_SCALAR_TYPE(p,int,level_);
  SAVE_SCALAR_TYPE(p,int,root_level_);
  SAVE_SCALAR_TYPE(p,bool,adaptive_timestep_);
  SAVE_SCALAR_TYPE(p,int,level_lower_);
  SAVE_SCALAR_TYPE(p,int,level_upper_);
  SAVE_SCALAR_TYPE(p,int,global_);
  SAVE_SCALAR_TYPE(p,bool,advanced_time_);

  SAVE_SCALAR_TYPE(p,int,id_refresh_);
  SAVE_SCALAR_TYPE(p,int,id_prolong_);
  SAVE_SCALAR_TYPE(p,int,id_restrict_);
  SAVE_SCALAR_TYPE(p,int,final_sync_);

  ASSERT2 ("Refresh::save_data\n",
 	   "Actual size %ld does not equal computed size %d",
	   p-buffer,data_size(),
	   ((p-buffer)==data_size()));

  return p;
}

//----------------------------------------------------------------------

char * Refresh::load_data (char * buffer)
{
  char * p = buffer;

  LOAD_SCALAR_TYPE(p,int,all_fields_);
  LOAD_VECTOR_TYPE(p,int,field_list_src_);
  LOAD_VECTOR_TYPE(p,int,field_list_dst_);

  LOAD_SCALAR_TYPE(p,int,all_particles_);
  LOAD_SCALAR_TYPE(p,bool,particles_are_copied_);
  LOAD_VECTOR_TYPE(p,int,particle_list_);

  LOAD_SCALAR_TYPE(p,int,all_fluxes_);
  LOAD_SCALAR_TYPE(p,int,ghost_depth_);
  LOAD_SCALAR_TYPE(p,int,min_face_rank_);
  LOAD_SCALAR_TYPE(p,int,neighbor_type_);
  LOAD_SCALAR_TYPE(p,int,accumulate_);
  LOAD_SCALAR_TYPE(p,int,sync_type_);
  LOAD_SCALAR_TYPE(p,int,sync_id_);
  LOAD_SCALAR_TYPE(p,int,active_);
  LOAD_SCALAR_TYPE(p,int,callback_);
  LOAD_SCALAR_TYPE(p,int,level_);
  LOAD_SCALAR_TYPE(p,int,root_level_);
  LOAD_SCALAR_TYPE(p,bool,adaptive_timestep_);
  LOAD_SCALAR_TYPE(p,int,level_lower_);
  LOAD_SCALAR_TYPE(p,int,level_upper_);
  LOAD_SCALAR_TYPE(p,int,global_);
  LOAD_SCALAR_TYPE(p,bool,advanced_time_);

  LOAD_SCALAR_TYPE(p,int,id_refresh_);
  LOAD_SCALAR_TYPE(p,int,id_prolong_);
  LOAD_SCALAR_TYPE(p,int,id_restrict_);
  LOAD_SCALAR_TYPE(p,int,final_sync_);

  ASSERT2 ("Refresh::load_data\n",
	   "Actual size %ld does not equal computed size %d",
	   p-buffer,data_size(),
	   ((p-buffer)==data_size()));

  return p;
}

//----------------------------------------------------------------------

ItNeighbor Refresh::it_neighbor
(Block * block, DirType dir_type)
{
  int n3[3], p3[3];
  cello::hierarchy()->root_blocks    (n3,n3+1,n3+2);
  cello::hierarchy()->get_periodicity(p3,p3+1,p3+2);
  int level_lower, level_upper;
  if (global_) {
    level_lower = cello::hierarchy()->min_level();
    level_upper = cello::hierarchy()->max_level() + 1;
  } else {
    level_lower = this->level_lower();
    level_upper = this->level_upper();
  }
  return ItNeighbor
    (block,
     min_face_rank(),
     p3,n3,block->index(),
     neighbor_type(),
     root_level(),
     level_lower,
     level_upper,
     dir_type);
}

//----------------------------------------------------------------------

bool Refresh::include_history(int face_type) const
{
  return adaptive_timestep();
}

//----------------------------------------------------------------------

void Refresh::include_history_fields_ (std::vector<int> & field_list,
                                       int face_type) const
{
  // skip if not including field history

  //  if ( ! adaptive_timestep() ) return;
  if (! include_history(face_type) ) return;
  
  // If adaptive timestepping and refining, add history = 1 fields
  // so receiver can interpolate in time

  FieldDescr * field_descr = cello::field_descr();
  const int n = field_list.size();
  for (int k=0; k<n; k++) {
    // Add previous timestep for src field if available
    int id_new = field_list[k];
    int id_old = field_descr->history_id(id_new,1);
    if (field_descr->history_age(id_new) == 0 &&
        field_descr->history_age(id_old) == 1) {
      field_list.push_back(id_old);
    }
  }
}
