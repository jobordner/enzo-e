#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>

struct Field {
  int cycle;
  std::string block_name;   int block_id;
  std::string region_name;  int region_id;
  int process_id;  double time;  std::string type;

  void print() const {
    printf (" %s %d %s:%d %s:%d %d %.6F\n",
            (type == "[" ? "Start" : "Stop "),
            cycle,
            region_name.c_str(), region_id,
            block_name.c_str(),  block_id,
            process_id,  time);
  }
};

void read_header
( std::ifstream & file,
  std::map<std::string,std::ofstream> & file_list,
  std::map<int,std::string>   & method_list,
  std::map<int,std::string>   & solver_list,
  std::map<int,std::string>   & refresh_list
  )
{
  while (!file.eof()) {

    std::string key;
    // Read key at start of line
    file >> key;
    // Process if header
    if (key.substr(0,1) == "#") {
      int region_id;
      std::string region_name;
      file >> region_id >> region_name;
      std::string file_name = key.substr(1,1)+"-"+std::to_string(region_id)+".data";

      // Create and open output data files

      file_list[file_name].open(file_name);

      if (key == "#M") {
        method_list[region_id] = region_name;
      } else if  (key == "#R") {
        refresh_list[region_id] = region_name;
      } else if  (key == "#S") {
        solver_list[region_id] = region_name;
      }

    } else if (key == "[" || key == "]") {
      // done reading header; put back key for reading fields

      file.putback(key.data()[0]);

      return;
    }
  }
}

//----------------------------------------------------------------------

void read_fields
( std::ifstream & file,
  std::vector<Field> & field_list)
{

  std::string line;
  //  while (!file.eof()) {
  while (std::getline(file,line)) {

    std::stringstream linestream(line);

    // Read key at start of line
    std::string key;

    linestream >> key;
    // Process if header
    if (key == "X") {
      printf ("Done!\n");
      break;
    }
    if (key == "[" || key == "]") {

      // read record
      Field field;

      field.type = key;

      linestream
        >> field.cycle
        >> field.block_name
        >> field.block_id
        >> field.region_name
        >> field.region_id
        >> field.process_id
        >> field.time;

      field_list.push_back(field);
    }
  }
}

//======================================================================

int main ()
{

  std::ifstream file ("PLOG");

  if (! file.is_open()) {
    printf ("ERROR!\n");
    exit(1);
  }

  std::map<std::string,std::ofstream> file_list;

  std::map<int,std::string>   method_list;
  std::map<int,std::string>   solver_list;
  std::map<int,std::string>   refresh_list;

  read_header (file,
               file_list,
               method_list,
               solver_list,
               refresh_list);

  std::vector<Field> field_list;

  read_fields (file,field_list);

  // Sort by cycle / region-type / region-id / block-name / time
  std::sort(field_list.begin(),field_list.end(),
            [](const Field & f1, const Field & f2) {
              bool eq = true;
              if ((f1.cycle < f2.cycle)) return true;
              eq = (f1.cycle == f2.cycle);
              if ((f1.region_name < f2.region_name) && eq ) return true;
              eq = eq && (f1.region_name == f2.region_name);
              if ((f1.region_id < f2.region_id) && eq) return true;
              eq = eq && (f1.region_id == f2.region_id);
              if ((f1.block_name < f2.block_name) && eq) return true;
              eq = eq && (f1.block_name == f2.block_name);
              if ((f1.time < f2.time) && eq) return true;
              return false; });

  int k=0;
  int ip=0;
  double time = 0;
  std::string filename;
  for (auto & file : file_list)
    printf ("files: %s %d\n",file.first.c_str(),
            file.second.is_open());

  for (const Field & field: field_list) {
    if (k % 2 == 0) {
      filename = field.region_name + "-" + std::to_string(field.region_id)+".data";
      ip = field.process_id;
      time = field.time;
    } else {

      file_list[filename] << ip << " " << time << " " << field.time << "\n";

    }
    k++;
  }


  // Close output files

  for (auto & file : file_list) file.second.close();

  // Close input file

  file.close();

  return 0;
}
