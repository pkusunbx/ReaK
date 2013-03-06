/**
 * \file rr_graph.hpp
 * 
 * This library contains the Rapidly-Exploring Random Graph generation algorithm.
 * This is a method to create a random graph that will span over a non-convex space
 * as rapidly as possible. The method relies on a simple randomized insertion algorithm.
 * At each step, a random point is picked from the underlying topology (i.e. configuration 
 * space in path-planning terms). Then, the points in the current graph that are nearest 
 * to the random point are picked for expansion. Finally, edges (of a maximum length) are 
 * added to the vertex of the graph towards the random point while it is still possible to
 * add such an edge without leaving the free space (the part of the configuration space which 
 * is not occupied by an obstacle). The algorithm will stop when either the number of vertices
 * in the tree has reached a maximum or when the user callback signals the stop.
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date May 2012
 */

/*
 *    Copyright 2012 Sven Mikael Persson
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

#ifndef REAK_RR_GRAPH_HPP
#define REAK_RR_GRAPH_HPP

#include <functional>
#include <vector>
#include <iterator>
#include <boost/limits.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_list.hpp>

#include "path_planning/metric_space_concept.hpp"
#include "path_planning/random_sampler_concept.hpp"

#include "rr_tree.hpp"

#include "node_generators.hpp"

namespace ReaK {
  
namespace graph {
  
  
  /**
   * This concept class defines what is required of a class to serve as a visitor to the RRG algorithm.
   * 
   * Required concepts:
   * 
   * the visitor should model the RRTVisitorConcept.
   * 
   * Valid expressions:
   * 
   * tie(b, ep) = vis.can_be_connected(u,v,g);  This function is called to attempt to steer from vertex u to vertex v, it returns true if a local path exists and is collision-free, and it also returns the edge-property of the edge that could connect those two vertices.
   * 
   * \tparam Visitor The visitor class to be checked for modeling this concept.
   * \tparam Graph The graph on which the visitor class is required to work with.
   * \tparam PositionMap The position property-map that provides the position descriptors with which the visitor class is required to work.
   */
  template <typename Visitor, typename Graph, typename PositionMap>
  struct RRGVisitorConcept : RRTVisitorConcept<Visitor,Graph,PositionMap> {
    typename Graph::edge_bundled ep;
    
    BOOST_CONCEPT_USAGE(RRGVisitorConcept) {
      boost::tie(this->b, ep) = this->vis.can_be_connected(this->u,this->u,this->g);
    };
  };

  /**
   * This class is simply a "null" visitor for the RRG algorithm. It is null in the sense that it
   * will do nothing (always returns true for the can_be_connected function).
   */
  struct default_rrg_visitor : default_rrt_visitor {
    template <typename Vertex, typename Graph>
    std::pair<bool, typename Graph::edge_bundled> can_be_connected(Vertex,Vertex,Graph&) { 
      typedef typename Graph::edge_bundled EdgeProp;
      return std::pair<bool, EdgeProp>(true, EdgeProp());
    };
  };

  /**
   * This class is a composite visitor class template. It can be used to glue together a function pointer (or functor)
   * for each of the functions of the RRGVisitorConcept so that it can be used as a light-weight,
   * copyable visitor object for the RRG algorithm (it is especially recommend to use the 
   * make_composite_rrg_visitor function template).
   */
  template <typename VertexAddedCallback,
            typename EdgeAddedCallback,
            typename SteerFunction,
            typename IsFreeQuery,
            typename KeepGoingQuery,
            typename CanConnectQuery>
  struct composite_rrg_visitor : composite_rrt_visitor<VertexAddedCallback, 
                                                       EdgeAddedCallback, 
                                                       SteerFunction,
                                                       IsFreeQuery,
                                                       KeepGoingQuery> {
    CanConnectQuery can_be_connected;
    composite_rrg_visitor(VertexAddedCallback aVertexAdded,
                          EdgeAddedCallback aEdgeAdded,
                          SteerFunction aSteerTowardsPosition,
                          IsFreeQuery aIsFree,
                          KeepGoingQuery aKeepGoing,
                          CanConnectQuery aCanBeConnected) :
                          composite_rrt_visitor<
                            VertexAddedCallback, 
                            EdgeAddedCallback, 
                            SteerFunction,
                            IsFreeQuery,
                            KeepGoingQuery>(aVertexAdded,
                                            aEdgeAdded,
                                            aSteerTowardsPosition,
                                            aIsFree,
                                            aKeepGoing),
                          can_be_connected(aCanBeConnected) { };
  };

  /**
   * This is a function template that is used to create an object of a class of the composite_rrg_visitor
   * class template. This is particularly convenient to avoid explicitely providing the list of template
   * arguments and let the compiler resolved them from the function parameter types.
   */
  template <typename VertexAddedCallback,
            typename EdgeAddedCallback,
            typename SteerFunction,
            typename IsFreeQuery,
            typename KeepGoingQuery,
            typename CanConnectQuery>
  inline composite_rrg_visitor<VertexAddedCallback, 
                               EdgeAddedCallback, 
                               SteerFunction, 
                               IsFreeQuery, 
                               KeepGoingQuery,
                               CanConnectQuery>
    make_composite_rrg_visitor(VertexAddedCallback aVertexAdded,
                               EdgeAddedCallback aEdgeAdded,
                               SteerFunction aSteerTowardsPosition,
                               IsFreeQuery aIsFree,
                               KeepGoingQuery aKeepGoing,
                               CanConnectQuery aCanBeConnected) {
    return composite_rrg_visitor<VertexAddedCallback, 
                                 EdgeAddedCallback, 
                                 SteerFunction, 
                                 IsFreeQuery, 
                                 KeepGoingQuery,
                                 CanConnectQuery>(aVertexAdded,
                                                  aEdgeAdded,
                                                  aSteerTowardsPosition,
                                                  aIsFree,
                                                  aKeepGoing,
                                                  aCanBeConnected);
  };


namespace detail {
  
  template <typename Graph,
            typename Topology,
            typename RRGVisitor,
            typename PositionMap,
            typename NodeGenerator,
            typename NcSelector>
  inline typename boost::enable_if< boost::is_undirected_graph<Graph> >::type 
    generate_rrg_loop(Graph& g,
                      const Topology& space,
                      RRGVisitor vis,
                      PositionMap position,
                      NodeGenerator node_generator_func,
                      NcSelector select_neighborhood,
                      unsigned int max_vertex_count) {
    typedef typename boost::property_traits<PositionMap>::value_type PositionValue;
    typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename boost::graph_traits<Graph>::edge_descriptor Edge; 
    typedef typename Graph::vertex_bundled VertexProp;
    typedef typename Graph::edge_bundled EdgeProp;
    using std::back_inserter;
    
    typedef boost::composite_property_map< 
      PositionMap, boost::whole_bundle_property_map< Graph, boost::vertex_bundle_t > > GraphPositionMap;
    GraphPositionMap g_position = GraphPositionMap(position, boost::whole_bundle_property_map< Graph, boost::vertex_bundle_t >(&g));
    
    while((num_vertices(g) < max_vertex_count) && (vis.keep_going())) {
      
      PositionValue p_new; Vertex x_near; EdgeProp eprop;
      boost::tie(x_near, p_new, eprop) = node_generator_func(g, vis, g_position);
      
      std::vector<Vertex> Nc;
      select_neighborhood(p_new, back_inserter(Nc), g, space, g_position);
      
      VertexProp xp_new;
      put(position, xp_new, p_new);
#ifdef RK_ENABLE_CXX0X_FEATURES
      Vertex x_new = add_vertex(std::move(xp_new),g);
#else
      Vertex x_new = add_vertex(xp_new,g);
#endif
      vis.vertex_added(x_new,g);
      
#ifdef RK_ENABLE_CXX0X_FEATURES
      std::pair<Edge, bool> ep = add_edge(x_near, x_new, std::move(eprop), g);
#else
      std::pair<Edge, bool> ep = add_edge(x_near, x_new, eprop, g);
#endif
      if(ep.second)
        vis.edge_added(ep.first, g);
      
      for(typename std::vector<Vertex>::const_iterator it = Nc.begin(); it != Nc.end(); ++it) {
        if(*it == x_near)
          continue;
        
        EdgeProp eprop2; bool can_connect;
        boost::tie(can_connect, eprop2) = vis.can_be_connected(*it, x_new, g);
        if(!can_connect)
          continue;
        
#ifdef RK_ENABLE_CXX0X_FEATURES
        ep = add_edge(*it, x_new, std::move(eprop2), g);
#else
        ep = add_edge(*it, x_new, eprop2, g);
#endif
        if(ep.second)
          vis.edge_added(ep.first, g);
      };
    };

  };
  
  
  template <typename Graph,
            typename Topology,
            typename RRGVisitor,
            typename PositionMap,
            typename NodeGenerator,
            typename NcSelector>
  inline typename boost::enable_if< boost::is_directed_graph<Graph> >::type 
    generate_rrg_loop(Graph& g,
                      const Topology& space,
                      RRGVisitor vis,
                      PositionMap position,
                      NodeGenerator node_generator_func,
                      NcSelector select_neighborhood,
                      unsigned int max_vertex_count) {
    typedef typename boost::property_traits<PositionMap>::value_type PositionValue;
    typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename boost::graph_traits<Graph>::edge_descriptor Edge; 
    typedef typename Graph::vertex_bundled VertexProp;
    typedef typename Graph::edge_bundled EdgeProp;
    using std::back_inserter;
    
    typedef boost::composite_property_map< 
      PositionMap, boost::whole_bundle_property_map< Graph, boost::vertex_bundle_t > > GraphPositionMap;
    GraphPositionMap g_position = GraphPositionMap(position, boost::whole_bundle_property_map< Graph, boost::vertex_bundle_t >(&g));
    
    while((num_vertices(g) < max_vertex_count) && (vis.keep_going())) {
      
      PositionValue p_new; Vertex x_near; EdgeProp eprop;
      boost::tie(x_near, p_new, eprop) = node_generator_func(g, vis, g_position);
      
      std::vector<Vertex> Pred, Succ;
      select_neighborhood(p_new, std::back_inserter(Pred), std::back_inserter(Succ), g, space, g_position);
      
      VertexProp xp_new;
      put(position, xp_new, p_new);
#ifdef RK_ENABLE_CXX0X_FEATURES
      Vertex x_new = add_vertex(std::move(xp_new),g);
#else
      Vertex x_new = add_vertex(xp_new,g);
#endif
      vis.vertex_added(x_new,g);
      
#ifdef RK_ENABLE_CXX0X_FEATURES
      std::pair<Edge, bool> ep = add_edge(x_near, x_new, std::move(eprop), g);
#else
      std::pair<Edge, bool> ep = add_edge(x_near, x_new, eprop, g);
#endif
      if(ep.second)
        vis.edge_added(ep.first, g);
      
      for(typename std::vector<Vertex>::iterator it = Pred.begin(); it != Pred.end(); ++it) {
        if(*it == x_near)
          continue;
        
        EdgeProp eprop2; bool can_connect;
        boost::tie(can_connect, eprop2) = vis.can_be_connected(*it, x_new, g);
        if(!can_connect)
          continue;
        
#ifdef RK_ENABLE_CXX0X_FEATURES
        ep = add_edge(*it, x_new, std::move(eprop2), g);
#else
        ep = add_edge(*it, x_new, eprop2, g);
#endif
        if(ep.second)
          vis.edge_added(ep.first, g);
      };
      
      for(typename std::vector<Vertex>::iterator it = Succ.begin(); it != Succ.end(); ++it) {
        EdgeProp eprop2; bool can_connect;
        boost::tie(can_connect, eprop2) = vis.can_be_connected(x_new, *it, g);
        if(!can_connect)
          continue;
        
#ifdef RK_ENABLE_CXX0X_FEATURES
        ep = add_edge(x_new, *it, std::move(eprop2), g);
#else
        ep = add_edge(x_new, *it, eprop2, g);
#endif
        if(ep.second)
          vis.edge_added(ep.first, g);
      };
    };

  };
  
  
  
};
  
  
  /**
   * This function template is the unidirectional version of the RRT algorithm (refer to rr_tree.hpp dox).
   * \tparam Graph A mutable graph type that will represent the generated tree, should model boost::VertexListGraphConcept and boost::MutableGraphConcept
   * \tparam Topology A topology type that will represent the space in which the configurations (or positions) exist, should model BGL's Topology concept
   * \tparam RRGVisitor An RRG visitor type that implements the customizations to this RRG algorithm, should model the RRGVisitorConcept.
   * \tparam PositionMap A property-map type that can store the configurations (or positions) of the vertices.
   * \tparam RandomSampler This is a random-sampler over the topology (see pp::RandomSamplerConcept).
   * \tparam NcSelector A functor type which can perform a neighborhood search of a point to a graph in the topology (see topological_search.hpp).
   * \param g A mutable graph that should initially store the starting and goal 
   *        vertex and will store the generated graph once the algorithm has finished.
   * \param space A topology (as defined by the Boost Graph Library). Note 
   *        that it is not required to generate only random points in 
   *        the free-space.
   * \param vis A RRG visitor implementing the RRGVisitorConcept. This is the 
   *        main point of customization and recording of results that the 
   *        user can implement.
   * \param position A mapping that implements the MutablePropertyMap Concept. Also,
   *        the value_type of this map should be the same type as the topology's 
   *        value_type.
   * \param get_sample A random sampler of positions in the free-space (obstacle-free sub-set of the topology).
   * \param select_neighborhood A callable object (functor) which can perform a 
   *        nearest neighbor search of a point to a graph in the topology. (see star_neighborhood)
   * \param max_vertex_count The maximum number of vertices beyond which the algorithm 
   *        should stop regardless of whether the resulting tree is satisfactory or not.
   * 
   */
  template <typename Graph,
            typename Topology,
            typename RRGVisitor,
            typename PositionMap,
            typename RandomSampler,
            typename NcSelector>
  inline void 
    generate_rrg(Graph& g,
                 const Topology& space,
                 RRGVisitor vis,
                 PositionMap position,
                 RandomSampler get_sample,
                 NcSelector select_neighborhood,
                 unsigned int max_vertex_count) {
    BOOST_CONCEPT_ASSERT((RRGVisitorConcept<RRGVisitor,Graph,PositionMap>));
    BOOST_CONCEPT_ASSERT((ReaK::pp::RandomSamplerConcept<RandomSampler,Topology>));
    
    typedef typename boost::property_traits<PositionMap>::value_type PositionValue;
    typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename boost::graph_traits<Graph>::edge_descriptor Edge; 
    typedef typename Graph::vertex_bundled VertexProp;
    using std::back_inserter;
    
    if(num_vertices(g) == 0) {
      PositionValue p = get_sample(space);
      while(!vis.is_position_free(p))
        p = get_sample(space);
      
      VertexProp up;
      put(position, up, p);
#ifdef RK_ENABLE_CXX0X_FEATURES
      Vertex u = add_vertex(std::move(up),g);
#else
      Vertex u = add_vertex(up,g);
#endif
      vis.vertex_added(u, g);
    };
    
    detail::generate_rrg_loop(
      g, space, vis, position, 
      rrg_node_generator<Topology, RandomSampler, NcSelector>(&space, get_sample, select_neighborhood),
      select_neighborhood, max_vertex_count);
    
  };
  
};

};


#endif

