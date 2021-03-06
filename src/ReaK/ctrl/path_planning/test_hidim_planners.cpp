
/*
 *    Copyright 2011 Sven Mikael Persson
 *
 *    THIS SOFTWARE IS DISTRIBUTED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE v3 (GPLv3).
 *
 *    This file is part of ReaK.
 *
 *    ReaK is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ReaK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ReaK (as LICENSE in the root folder).  
 *    If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>

#include <ReaK/ctrl/topologies/hyperbox_topology.hpp>
#include <ReaK/ctrl/topologies/se3_topologies.hpp>
#include <ReaK/ctrl/topologies/no_obstacle_space.hpp>
#include <ReaK/ctrl/interpolation/sustained_velocity_pulse.hpp>


// #define RK_DISABLE_RRT_PLANNER
// #define RK_DISABLE_RRTSTAR_PLANNER
// #define RK_DISABLE_PRM_PLANNER
// #define RK_DISABLE_FADPRM_PLANNER
// #define RK_DISABLE_SBASTAR_PLANNER

#include <ReaK/ctrl/path_planning/planner_exec_engines.hpp>
#include <ReaK/ctrl/path_planning/path_planner_options_po.hpp>

#include <ReaK/core/optimization/optim_exceptions.hpp>

#include <ReaK/ctrl/path_planning/basic_sbmp_reporters.hpp>
#include <ReaK/ctrl/path_planning/vlist_sbmp_report.hpp>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;


#ifndef RK_HIDIM_PLANNER_N
#define RK_HIDIM_PLANNER_N 3
#endif



int main(int argc, char** argv) {
  
  using namespace ReaK;
  using namespace pp;
  
  std::string config_file;
  
  po::options_description generic_options("Generic options");
  generic_options.add_options()
    ("help,h", "produce this help message.")
    ("config,c", po::value< std::string >(&config_file)->default_value("test_hidim_planners.cfg"),
                  "configuration file-name (can contain any or all options, will be overriden by command-line options).")
  ;
  
  po::options_description io_options("I/O options");
  io_options.add_options()
    ("output-path,o", po::value< std::string >()->default_value("pp_results"), "specify the output path (default is pp_results)")
  ;
  
  po::options_description mc_options("Monte-Carlo options");
  mc_options.add_options()
    ("monte-carlo,m", "specify that monte-carlo runs should be performed (default is not).")
    ("mc-runs", po::value< std::size_t >()->default_value(100), "number of monte-carlo runs to average out (default is 100).")
  ;
  
  po::options_description single_options("Single-run options");
  single_options.add_options()
    ("single-run,s", "specify that single runs should be performed (default is not).")
  ;
  
  po::options_description planner_select_options = get_planning_option_po_desc();
  planner_select_options.add_options()
    ("max-edge-length", po::value< double >(), "maximum length of edges of the motion-graph (default is 0.2*sqrt(N)).")
  ;
  
  po::options_description generate_options("File generation options");
  generate_options.add_options()
    ("generate-all-files", po::value< std::string >(), "specify that all configuration files should be generated with the given file-name prefix (file-name without suffix and extension).")
    ("generate-planner-options", po::value< std::string >(), "specify that the planner options file should be generated with the given file-name prefix (file-name without extension).")
    
    ("generate-xml",      "if set, output results in XML format (rkx) (default).")
    ("generate-protobuf", "if set, output results in protobuf format (pbuf).")
    ("generate-binary",   "if set, output results in binary format (rkb).")
  ;
  
  po::options_description cmdline_options;
  cmdline_options.add(generic_options).add(io_options).add(mc_options).add(single_options)
                 .add(planner_select_options).add(generate_options);
  
  po::options_description config_file_options;
  config_file_options.add(io_options).add(mc_options).add(single_options).add(planner_select_options);
  
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
  po::notify(vm);
  
  {
    std::ifstream ifs(config_file.c_str());
    if(ifs) {
      po::store(po::parse_config_file(ifs, config_file_options), vm);
      po::notify(vm);
    };
  };
  
  
  if(vm.count("help")) {
    std::cout << cmdline_options << std::endl;
    return 0;
  };
  
  if( vm.count("monte-carlo") + vm.count("single-run")
       + vm.count("generate-all-files") + vm.count("generate-planner-options") < 1 ) {
    std::cout << "Error: There was no action specified! This program is designed to perform Monte-Carlo runs, single runs (with output), or generate the configuration files to construct scenarios. You must specify at least one of these actions to be performed!" << std::endl;
    std::cout << cmdline_options << std::endl;
    return 1;
  };
  
  std::string output_path_name = vm["output-path"].as<std::string>();
  while(output_path_name[output_path_name.length()-1] == '/') 
    output_path_name.erase(output_path_name.length()-1, 1);
  
  fs::create_directory(output_path_name.c_str());
  
  
  planning_option_collection plan_options = get_planning_option_from_po(vm);
  
  std::string knn_method_str = plan_options.get_knn_method_str();
  std::string mg_storage_str = plan_options.get_mg_storage_str();
  std::string planner_qualifier_str = plan_options.get_planner_qualifier_str();
  std::string planner_name_str = plan_options.get_planning_algo_str() + "_" + planner_qualifier_str + "_" + mg_storage_str + "_" + knn_method_str;
  
  double max_radius = 0.2 * std::sqrt(double(RK_HIDIM_PLANNER_N));
  if( vm.count("max-edge-length") )
    max_radius = vm["max-edge-length"].as<double>();
  plan_options.max_random_walk = max_radius;
  
  
  // Do the generations if required:
  
  if( vm.count("generate-all-files") + vm.count("generate-planner-options") > 0 ) {
    std::string file_name;
    if( vm.count("generate-planner-options") == 0 ) {
      file_name = vm["generate-all-files"].as< std::string >() + "_planner";
    } else {
      file_name = vm["generate-planner-options"].as< std::string >();
    };
    if( vm.count("generate-protobuf") ) 
      file_name += ".pbuf";
    else if( vm.count("generate-binary") )
      file_name += ".rkb";
    else 
      file_name += ".rkx";
    
    try {
      (*serialization::open_oarchive(file_name)) << plan_options;
    } catch( std::exception& e ) { 
      std::cerr << "Error: Could not generate the planner options file!" << std::endl;
    };
    if( vm.count("monte-carlo") + vm.count("single-run") == 0 ) // only wanted to generate planner-option file.
      return 0;
  };
  
  
  
  std::string world_ND_name = "world_";
  { std::stringstream ss_tmp;
    ss_tmp << RK_HIDIM_PLANNER_N << "D";
    world_ND_name += ss_tmp.str();
  };
  std::string space_ND_name = "e";
  { std::stringstream ss_tmp;
    ss_tmp << RK_HIDIM_PLANNER_N;
    space_ND_name += ss_tmp.str();
  };
  
  vect<double, RK_HIDIM_PLANNER_N > lb, ub, start_pt, goal_pt;
  for(std::size_t i = 0; i < RK_HIDIM_PLANNER_N; ++i) {
    lb[i] = 0.0;
    ub[i] = 1.0;
    start_pt[i] = 0.05;
    goal_pt[i] = 0.95;
  };
  
  typedef no_obstacle_space< hyperbox_topology< vect<double, RK_HIDIM_PLANNER_N > > > WorldNDType;
  
  shared_ptr< WorldNDType > world_ND =
    shared_ptr< WorldNDType >(
      new WorldNDType(
        world_ND_name + "_no_obstacles",
        hyperbox_topology< vect<double, RK_HIDIM_PLANNER_N > >(world_ND_name, lb, ub),
        max_radius));
  world_ND->set_start_pos(start_pt);
  world_ND->set_goal_pos(goal_pt);
  
  if(vm.count("monte-carlo")) {
    monte_carlo_mp_engine mc_eng(vm["mc-runs"].as<std::size_t>(), planner_name_str, output_path_name + "/" + space_ND_name);
    try {
      execute_p2p_planner(world_ND, plan_options, RK_HIDIM_PLANNER_N, mc_eng, world_ND->get_start_pos(), world_ND->get_goal_pos());
    } catch(std::exception& e) {
      std::cerr << "Error: An exception was raised during the planning:\nwhat(): " << e.what() << std::endl;
      return 2;
    };
  };
  
  if(vm.count("single-run")) {
    vlist_print_mp_engine sr_eng(planner_name_str, output_path_name + "/" + space_ND_name);
    try {
      execute_p2p_planner(world_ND, plan_options, RK_HIDIM_PLANNER_N, sr_eng, world_ND->get_start_pos(), world_ND->get_goal_pos());
    } catch(std::exception& e) {
      std::cerr << "Error: An exception was raised during the planning:\nwhat(): " << e.what() << std::endl;
      return 3;
    };
  };
  
  return 0;
};











