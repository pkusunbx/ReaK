/**
 * \file planning_visitors.hpp
 * 
 * This library defines 
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date July 2013
 */

/*
 *    Copyright 2013 Sven Mikael Persson
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

#ifndef REAK_PLANNING_VISITORS_HPP
#define REAK_PLANNING_VISITORS_HPP

#include "base/defs.hpp"
#include "base/named_object.hpp"

#include "motion_planner_base.hpp"
#include "planning_queries.hpp"
#include "any_sbmp_reporter.hpp"
#include "any_motion_graph.hpp"
#include "any_knn_synchro.hpp"

#include "metric_space_concept.hpp"
#include "subspace_concept.hpp"

namespace ReaK {
  
namespace pp {

/**
 * This class template
 */
template <typename Derived,
          typename FreeSpaceType>
struct planning_visitor_base {
  
  typedef FreeSpaceType space_type;
  typedef typename subspace_traits<space_type>::super_space_type super_space_type;
  
  typedef typename topology_traits<space_type>::point_type point_type;
  typedef typename topology_traits<space_type>::point_difference_type point_difference_type;
  
  typedef typename boost::if_< is_temporal_space<space_type>,
    trajectory_base< super_space_type >,
    seq_path_base< super_space_type > >::type solution_base_type;
  
  typedef shared_ptr< solution_base_type > solution_record_ptr;
  
  typedef typename boost::if_< is_temporal_space<space_type>,
    sample_based_planner< motion_planner_base<space_type> >,
    sample_based_planner< path_planner_base<space_type> > >::type planner_base_type;
  
  typedef planning_query<space_type> query_type;
  
  planner_base_type* m_planner;
  query_type*        m_query;
  any_knn_synchro*   m_nn_synchro;
  
  boost::any m_start_node;
  boost::any m_goal_node;
  
  planning_visitor_base(planner_base_type* aPlanner,
                        query_type* aQuery = NULL,
                        any_knn_synchro* aNNSynchro = NULL,
                        boost::any aStartNode = boost::any(),
                        boost::any aGoalNode = boost::any()) : 
                        m_planner(aPlanner),
                        m_query(aQuery),
                        m_nn_synchro(aNNSynchro),
                        m_start_node(aStartNode),
                        m_goal_node(aGoalNode) { };
  
  typedef mg_edge_data<space_type> EdgeProp;
  
  template <typename Vertex, typename Graph>
  void dispatched_register_solution(Vertex start, Vertex, Vertex current, const Graph& g, 
                                    const mg_vertex_data<space_type>&) const {
    double goal_dist = m_query->get_distance_to_goal(g[current].position);
    solution_record_ptr srp = m_query->register_solution(start, current, goal_dist, g);
    if(srp)
      m_planner->report_solution(srp);
  };
  
  template <typename Vertex, typename Graph>
  void dispatched_register_solution(Vertex start, Vertex goal, Vertex, const Graph& g, 
                                    const optimal_mg_vertex<space_type>&) const {
    if((in_degree(goal, g)) && (g[goal].distance_accum < m_query->get_best_solution_distance())) {
      solution_record_ptr srp = m_query->register_solution(start, goal, 0.0, g);
      if(srp)
        m_planner->report_solution(srp);
    };
  };
  
  template <typename Graph>
  void publish_path(const Graph& g) const {
    if(!m_goal_node.empty()) {
      dispatched_register_solution(boost::any_cast<Vertex>(m_start_node), 
                                   boost::any_cast<Vertex>(m_goal_node), 
                                   boost::any_cast<Vertex>(m_goal_node), g, g[u]);
    };
  };
  
  
/***************************************************
                SBMPVisitorConcept
***************************************************/
  
  template <typename Vertex, typename Graph>
  void vertex_added(Vertex u, Graph& g) const {
    m_nn_synchro->added_vertex(u,g);
    
    static_cast<const Derived*>(this)->initialize_vertex(u, g);
    
    // Call progress reporter...
    m_planner->report_progress(g);
    
    if((m_planner->get_planning_method_flags() & PLANNING_DIRECTIONALITY_MASK) == BIDIRECTIONAL_PLANNING)
      return;  // do not check goal connection for a bi-directional planner (wait for "joining vertex").
    
    if(!m_goal_node.empty()) {
      // try to build solution if there is a good accumulated distance at the goal node.
      dispatched_register_solution(boost::any_cast<Vertex>(m_start_node), 
                                   boost::any_cast<Vertex>(m_goal_node), u, g, g[u]);
    };
  };
  
  template <typename Edge, typename Graph>
  void edge_added(Edge e, Graph& g) const { 
    if( ((m_planner->get_planning_method_flags() & PLANNING_DIRECTIONALITY_MASK) == BIDIRECTIONAL_PLANNING) ||
        (!m_goal_node.empty()) )
      return;  // do not check goal connection for a bi-directional planner (wait for "joining vertex") or a planner that contains the goal node.
    
    // try to connect the latest node to the goal node.
    double goal_dist = m_query->get_distance_to_goal(g[target(e,g)].position);
    if(goal_dist < std::numeric_limits<double>::infinity()) {
      solution_record_ptr srp = m_query->register_solution(boost::any_cast<Vertex>(m_start_node), target(e,g), goal_dist, g);
      if(srp)
        m_planner->report_solution(srp);
    };
  };
  
  bool keep_going() const {
    return m_planner->keep_going() && m_query->keep_going();
  };
  

/***************************************************
                SBMPPruningVisitorConcept
***************************************************/
  
  template <typename Vertex, typename Graph>
  void vertex_to_be_removed(Vertex u, Graph& g) const {
    m_nn_synchro->removed_vertex(u,g);
  };
  
  
/***************************************************
                SBMPJoiningVisitorConcept
***************************************************/
  
  template <typename Vertex, typename Graph>
  void joining_vertex_found(Vertex u1, Vertex u2, Graph& g1, Graph& g2) const {
    double join_dist = get(distance_metric, m_query->space->get_super_space())(g1[u1].position, g2[u2].position, m_query->space->get_super_space());
    solution_record_ptr srp = m_query->register_solution(
      boost::any_cast<Vertex>(m_start_node), 
      boost::any_cast<Vertex>(m_goal_node), 
      u1, u2, join_dist, g1, g2);
    if(srp)
      m_planner->report_solution(srp);
  };
  
  
/***************************************************
                CollisionCheckingVisitorConcept
***************************************************/
  
  /* Not sure if this is really needed, 
   * why not just provide high-level planner algs with the free-space instead of super-space.
  */
  bool is_position_free(const point_type& p) const {
    return m_query->space->is_free(p);
  };
  
  
  
/***************************************************
      Steering functions, dispatched by case
***************************************************/
  
  // case 1 : space is steerable, mg is optimal
  template <typename SwitchFreeSpace>
  typename boost::enable_if< is_steerable_space< SwitchFreeSpace >,
  double >::type dispatched_steer_towards_position(const SwitchFreeSpace& space, 
                                                   const point_type& p_src, const point_type& p_dest, 
                                                   point_type& p_result, double fraction, 
                                                   optimal_mg_edge<space_type>& ep_result) const {
    boost::tie(p_result, ep_result.steer_record) = space.steer_position_toward(p_src, fraction, p_dest);
    ep_result.weight = get(distance_metric, space.get_super_space())(p_src, p_result, space.get_super_space());
    return ep_result.weight;
  };
  
  // case 2 : space is not steerable, mg is optimal
  template <typename SwitchFreeSpace>
  typename boost::disable_if< is_steerable_space< SwitchFreeSpace >,
  double >::type dispatched_steer_towards_position(const SwitchFreeSpace& space, 
                                                 const point_type& p_src, const point_type& p_dest, 
                                                 point_type& p_result, double fraction, 
                                                 optimal_mg_edge<space_type>& ep_result) const {
    p_result = space.move_position_toward(p_src, fraction, p_dest);
    ep_result.weight = get(distance_metric, space.get_super_space())(p_src, p_result, space.get_super_space());
    return ep_result.weight;
  };
  
  // case 3 : space is steerable, mg is basic
  template <typename SwitchFreeSpace>
  typename boost::enable_if< is_steerable_space< SwitchFreeSpace >,
  double >::type dispatched_steer_towards_position(const SwitchFreeSpace& space, 
                                                 const point_type& p_src, const point_type& p_dest, 
                                                 point_type& p_result, double fraction, 
                                                 mg_edge_data<space_type>& ep_result) const {
    boost::tie(p_result, ep_result.steer_record) = space.steer_position_toward(p_src, fraction, p_dest);
    return get(distance_metric, space.get_super_space())(p_src, p_result, space.get_super_space());
  };
  
  // case 4 : space is not steerable, mg is basic
  template <typename SwitchFreeSpace>
  typename boost::disable_if< is_steerable_space< SwitchFreeSpace >,
  double >::type dispatched_steer_towards_position(const SwitchFreeSpace& space, 
                                                 const point_type& p_src, const point_type& p_dest, 
                                                 point_type& p_result, double fraction, 
                                                 mg_edge_data<space_type>&) const {
    p_result = space.move_position_toward(p_src, fraction, p_dest);
    return get(distance_metric, space.get_super_space())(p_src, p_result, space.get_super_space());
  };
  
/***************************************************
                NodePullingVisitorConcept
***************************************************/
  
  template <typename Vertex, typename Graph>
  boost::tuple<point_type, bool, typename Graph::edge_bundled> steer_towards_position(const point_type& p, Vertex u, Graph& g) const {
    typedef typename Graph::edge_bundled EdgeProp;
    typedef boost::tuple<point_type, bool, EdgeProp> ResultType;
    ResultType result;
    double traveled_dist = dispatched_steer_towards_position(*(m_query->space), g[u].position, p, get<0>(result), 1.0, get<2>(result));
    double best_case_dist = get(distance_metric, m_query->space->get_super_space())(g[u].position, p, m_query->space->get_super_space());
    get<1>(result) = (traveled_dist > m_planner->get_steer_progress_tolerance() * best_case_dist);
    return result;
  };
  
/***************************************************
                NodeReConnectVisitorConcept
***************************************************/
  
  template <typename Vertex, typename Graph>
  std::pair<bool, typename Graph::edge_bundled> can_be_connected(Vertex u, Vertex v, const Graph& g) const {
    typedef typename Graph::edge_bundled EdgeProp;
    typedef std::pair<bool, EdgeProp> ResultType;
    point_type p_result;
    ResultType result;
    double traveled_dist = dispatched_steer_towards_position(*(m_query->space), g[u].position, g[v].position, p_result, 1.0, result.second);
    double remaining_dist = get(distance_metric, m_query->space->get_super_space())(p_result, g[v].position, m_query->space->get_super_space());
    result.first = (remaining_dist < m_planner->get_connection_tolerance() * traveled_dist);
    return result;
  };
  
/***************************************************
                NodePushingVisitorConcept
***************************************************/
  
  template <typename Vertex, typename Graph>
  boost::tuple<point_type, bool, typename Graph::edge_bundled> random_walk(Vertex u, Graph& g) const {
    typedef typename Graph::edge_bundled EdgeProp;
    typedef boost::tuple<PointType, bool, EdgeProp > ResultType;
    
    const super_space_type& sup_space = m_query->space->get_super_space();
    typename point_distribution_traits< super_space_type >::random_sampler_type get_sample = get(random_sampler, sup_space);
    boost::variate_generator< pp::global_rng_type&, boost::normal_distribution<double> > var_rnd(pp::get_global_rng(), boost::normal_distribution<double>());
    
    unsigned int i = 0;
    point_type p_rnd = get_sample(sup_space);
    point_difference_type dp_rnd = sup_space.difference(p_rnd, sup_space.origin());
    ResultType result;
    do {
      p_rnd = sup_space.adjust(g[u].position, dp_rnd);
      double dist = get(distance_metric, sup_space)(g[u].position, p_rnd, sup_space);
      double target_dist = boost::uniform_01<global_rng_type&,double>(get_global_rng())() * m_planner->get_sampling_radius();
      double traveled_dist = dispatched_steer_towards_position(*(m_query->space), g[u].position, g[v].position, 
                                                               get<0>(result), target_dist / dist, get<2>(result));
      if( traveled_dist > m_planner->get_steer_progress_tolerance() * target_dist ) {
        get<1>(result) = true;
        return result;
      } else {
        p_rnd = get_sample(sup_space);
        dp_rnd = sup_space.difference(p_rnd, sup_space.origin());
      };
    } while(++i <= 10);
    get<1>(result) = false;
    return result;
  };
  
  
  
};





/**
 * This class template
 */
template <typename FreeSpaceType>
struct planning_visitor : planning_visitor_base< planning_visitor<FreeSpaceType>, FreeSpaceType> {
  
  typedef planning_visitor<FreeSpaceType> self;
  typedef planning_visitor_base< self, FreeSpaceType> base_type;
  
  typedef typename base_type::planner_base_type planner_base_type;
  typedef typename base_type::query_type query_type;
  
  planning_visitor(planner_base_type* aPlanner,
                   query_type* aQuery = NULL,
                   any_knn_synchro* aNNSynchro = NULL,
                   boost::any aStartNode = boost::any(),
                   boost::any aGoalNode = boost::any()) : 
                   base_type(aPlanner, aQuery, aNNSynchro, aStartNode, aGoalNode) { };
  
/***************************************************
                NodeExploringVisitorConcept
***************************************************/
  
  template <typename Vertex, typename Graph>
  void initialize_vertex(Vertex, const Graph&) const { };
  template <typename Vertex, typename Graph>
  void discover_vertex(Vertex, const Graph&) const { };
  template <typename Vertex, typename Graph>
  void examine_vertex(Vertex, const Graph&) const { };
  template <typename Edge, typename Graph>
  void examine_edge(Edge, const Graph&) const { };
  template <typename Vertex, typename Graph>
  bool has_search_potential(Vertex u, const Graph&) const { 
    if(this->m_goal_node.empty())
      return true;
    else
      return ( u != boost::any_cast<Vertex>(this->m_goal_node) );
  };
  template <typename Vertex, typename Graph>
  bool should_close(Vertex u, const Graph& g) const { 
    return !has_search_potential(u,g);
  };
  
/***************************************************
                AnytimeHeuristicVisitorConcept  (Anytime A* search)
***************************************************/
  
  template <typename Graph>
  double adjust_relaxation(double old_relaxation, const Graph& g) const {
    return old_relaxation * 0.5;
  };
  
};


/**
 * This class template
 */
template <typename FreeSpaceType>
struct heuristic_plan_visitor : planning_visitor_base< heuristic_plan_visitor<FreeSpaceType>, FreeSpaceType> {
  
  typedef heuristic_plan_visitor<FreeSpaceType> self;
  typedef planning_visitor_base< self, FreeSpaceType> base_type;
  
  typedef typename base_type::planner_base_type planner_base_type;
  typedef typename base_type::query_type query_type;
  
  heuristic_plan_visitor(planner_base_type* aPlanner,
                         query_type* aQuery = NULL,
                         any_knn_synchro* aNNSynchro = NULL,
                         boost::any aStartNode = boost::any(),
                         boost::any aGoalNode = boost::any()) : 
                         base_type(aPlanner, aQuery, aNNSynchro, aStartNode, aGoalNode) { };
  
/***************************************************
                NodeExploringVisitorConcept
***************************************************/
  
  template <typename Vertex, typename Graph>
  void initialize_vertex(Vertex u, Graph& g) const { 
    g[u].heuristic_value = this->m_query->get_heuristic_to_goal(g[u].position);
  };
  template <typename Vertex, typename Graph>
  void discover_vertex(Vertex, const Graph&) const { };
  template <typename Vertex, typename Graph>
  void examine_vertex(Vertex, const Graph&) const { };
  template <typename Edge, typename Graph>
  void examine_edge(Edge, const Graph&) const { };
  template <typename Vertex, typename Graph>
  bool has_search_potential(Vertex u, const Graph& g) const { 
    if(this->m_goal_node.empty())
      return ( g[u].heuristic_value > std::numeric_limits<double>::epsilon() * g[boost::any_cast<Vertex>(this->m_start_node)].heuristic_value );
    else
      return ( u != boost::any_cast<Vertex>(this->m_goal_node) );
  };
  template <typename Vertex, typename Graph>
  bool should_close(Vertex u, const Graph& g) const { 
    return !has_search_potential(u,g);
  };
  
/***************************************************
                AnytimeHeuristicVisitorConcept  (Anytime A* search)
***************************************************/
  
  template <typename Graph>
  double adjust_relaxation(double old_relaxation, const Graph& g) const {
    return old_relaxation * 0.5;
  };
  
};





};

};

#endif

