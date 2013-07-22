/**
 * \file motion_planner_base.hpp
 * 
 * This library defines base classes for path-planners and motion-planners as the 
 * stem of OOP path-planners used in ReaK. OOP-style planners are useful to hide away
 * the cumbersome details of calling the underlying planning algorithms which are 
 * generic programming (GP) style and thus provide a lot more flexibility but are difficult
 * to deal with in the user-space. The OOP planners are meant to offer a much simpler interface,
 * i.e., a member function that "solves the problem" and returns the solution path or trajectory.
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date July 2012
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

#ifndef REAK_MOTION_PLANNER_BASE_HPP
#define REAK_MOTION_PLANNER_BASE_HPP

#include "base/defs.hpp"
#include "base/named_object.hpp"

#include "metric_space_concept.hpp"
#include "steerable_space_concept.hpp"
#include "random_sampler_concept.hpp"
#include "subspace_concept.hpp"

#include "trajectory_base.hpp"
#include "seq_path_base.hpp"

#include "path_planner_options.hpp"
#include "any_motion_graphs.hpp"
#include "planning_queries.hpp"
#include "any_sbmp_reporter.hpp"

namespace ReaK {
  
namespace pp {


/**
 * This class is the basic OOP interface for a path planner. 
 * OOP-style planners are useful to hide away
 * the cumbersome details of calling the underlying planning algorithms which are 
 * generic programming (GP) style and thus provide a lot more flexibility but are difficult
 * to deal with in the user-space. The OOP planners are meant to offer a much simpler interface,
 * i.e., a member function that "solves the problem" and returns the solution path or trajectory.
 */
template <typename FreeSpaceType>
class planner_base : public named_object {
  public:
    typedef planner_base<FreeSpaceType> self;
    typedef FreeSpaceType space_type;
    typedef typename subspace_traits<FreeSpaceType>::super_space_type super_space_type;
    
    BOOST_CONCEPT_ASSERT((SubSpaceConcept<FreeSpaceType>));
    
    typedef typename topology_traits< super_space_type >::point_type point_type;
    typedef typename topology_traits< super_space_type >::point_difference_type point_difference_type;
    
    typedef typename boost::mpl::if_< is_temporal_space<space_type>,
      trajectory_base< super_space_type >,
      seq_path_base< super_space_type > >::type solution_base_type;
    
    typedef shared_ptr< solution_base_type > solution_record_ptr;
    
  protected:
    
    shared_ptr< space_type > m_space;
    
  public:
    
    /**
     * This function computes a valid path in the C-free. If it cannot 
     * achieve a valid path, an exception will be thrown. This algorithmic
     * path solver class is such that any settings that ought to be set for the 
     * path planning algorithm should be set before calling this function, otherwise
     * the function is likely to fail.
     * \param aQuery The query object that defines as input the parameters of the query, 
     *               and as output, the recorded solutions.
     */
    virtual void solve_planning_query(planning_query<FreeSpaceType>& aQuery) = 0;
    
    /**
     * This function is called to reset the internal state of the planner.
     */
    virtual void reset_internal_state() = 0;
    
    /**
     * Returns true if the solver should keep on going trying to solve the path-planning problem.
     * \return True if the solver should keep on going trying to solve the path-planning problem.
     */
    virtual bool keep_going() const { return true; };
    
    /**
     * Parametrized constructor.
     * \param aName The name for this object.
     * \param aWorld A topology which represents the C-free (obstacle-free configuration space).
     */
    planner_base(const std::string& aName,
                 const shared_ptr< space_type >& aWorld) :
                 named_object(),
                 m_space(aWorld) { setName(aName); };
    
    virtual ~planner_base() { };
    
    
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/

    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      named_object::save(A,named_object::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(m_space);
    };

    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      named_object::load(A,named_object::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(m_space);
    };

    RK_RTTI_MAKE_ABSTRACT_1BASE(self,0xC2460000,1,"planner_base",named_object)
};



namespace detail {
  
  template <typename Topology, typename Graph, typename Reporter>
  typename boost::enable_if< is_steerable_space< Topology >,
  void >::type do_report_progress_impl(const Topology& space, Graph& g, Reporter& reporter) {
    reporter.draw_motion_graph(space, g, get(&mg_edge_data<Topology>::steer_record, g));
  };
  
  template <typename Topology, typename Graph, typename Reporter>
  typename boost::disable_if< is_steerable_space< Topology >,
  void >::type do_report_progress_impl(const Topology& space, Graph& g, Reporter& reporter) {
    reporter.draw_motion_graph(space, g, get(&mg_vertex_data<Topology>::position,g));
  };
  
  
};




/**
 * This class is the basic OOP interface for a sampling-based motion planner. 
 * OOP-style planners are useful to hide away
 * the cumbersome details of calling the underlying planning algorithms which are 
 * generic programming (GP) style and thus provide a lot more flexibility but are difficult
 * to deal with in the user-space. The OOP planners are meant to offer a much simpler interface,
 * i.e., a member function that "solves the problem" and returns the solution path or trajectory.
 */
template <typename FreeSpaceType>
class sample_based_planner : public planner_base<FreeSpaceType> {
  protected:
    typedef planner_base<FreeSpaceType> base_type;
    typedef typename base_type::space_type space_type;
    typedef sample_based_planner<FreeSpaceType> self;
    
    typedef typename base_type::solution_record_ptr solution_record_ptr;
    
    std::size_t m_max_vertex_count;
    std::size_t m_progress_interval;
    std::size_t m_iteration_count;
    std::size_t m_data_structure_flags;
    std::size_t m_planning_method_flags;
    
    double m_steer_progress_tol;
    double m_connection_tol;
    double m_sampling_radius;
    std::size_t m_space_dimensionality;
    
    any_sbmp_reporter_chain<space_type> m_reporter;
    
  public:
    
    /**
     * Returns the maximum number of samples to generate during the motion planning.
     * \return the maximum number of samples to generate during the motion planning.
     */
    std::size_t get_max_vertex_count() const { return m_max_vertex_count; };
    
    /**
     * Sets the maximum number of samples to generate during the motion planning.
     * \param aMaxVertexCount The maximum number of samples to generate during the motion planning.
     */
    virtual void set_max_vertex_count(std::size_t aMaxVertexCount) { 
      m_max_vertex_count = aMaxVertexCount;
    };
    
    /**
     * Returns the number of new samples between each "progress report".
     * \return the number of new samples between each "progress report".
     */
    std::size_t get_progress_interval() const { return m_progress_interval; };
    
    /**
     * Sets the number of new samples between each "progress report".
     * \param aProgressInterval The number of new samples between each "progress report".
     */
    virtual void set_progress_interval(std::size_t aProgressInterval) { 
      m_progress_interval = aProgressInterval;
    };
    
    /**
     * Returns the integer flags that identify the kind of motion-graph data-structure to use.
     * Can be ADJ_LIST_MOTION_GRAPH or DVP_ADJ_LIST_MOTION_GRAPH. Any combination of those two and of KNN method flags to use 
     * for nearest-neighbor queries in the graph. KNN method flags can be LINEAR_SEARCH_KNN, DVP_BF2_TREE_KNN, DVP_BF4_TREE_KNN, 
     * DVP_COB2_TREE_KNN, or DVP_COB4_TREE_KNN. 
     * See path_planner_options.hpp documentation.
     * \return The integer flags that identify the kind of motion-graph data-structure to use (see path_planner_options.hpp).
     */
    std::size_t get_data_structure_flags() const { return m_data_structure_flags; };
    /**
     * Sets the integer flags that identify the kind of motion-graph data-structure to use.
     * Can be ADJ_LIST_MOTION_GRAPH or DVP_ADJ_LIST_MOTION_GRAPH. Any combination of those two and of KNN method flags to use 
     * for nearest-neighbor queries in the graph. KNN method flags can be LINEAR_SEARCH_KNN, DVP_BF2_TREE_KNN, DVP_BF4_TREE_KNN, 
     * DVP_COB2_TREE_KNN, or DVP_COB4_TREE_KNN. 
     * See path_planner_options.hpp documentation.
     * \param aDataStructureFlags The integer flags that identify the kind of motion-graph data-structure to use (see path_planner_options.hpp).
     */
    virtual void set_data_structure_flags(std::size_t aDataStructureFlags) { m_data_structure_flags = aDataStructureFlags; };
    
    /**
     * Returns the integer flags that identify various options to use with this planner.
     * The options available include EAGER_COLLISION_CHECKING or LAZY_COLLISION_CHECKING, NOMINAL_PLANNER_ONLY or any 
     * combination of PLAN_WITH_VORONOI_PULL, PLAN_WITH_NARROW_PASSAGE_PUSH and PLAN_WITH_ANYTIME_HEURISTIC,
     * UNIDIRECTIONAL_PLANNING or BIDIRECTIONAL_PLANNING, and USE_BRANCH_AND_BOUND_PRUNING_FLAG. 
     * See path_planner_options.hpp documentation.
     * \return The integer flags that identify various options to use with this planner (see path_planner_options.hpp).
     */
    std::size_t get_planning_method_flags() const { return m_planning_method_flags; };
    /**
     * Sets the integer flags that identify various options to use with this planner.
     * The options available include EAGER_COLLISION_CHECKING or LAZY_COLLISION_CHECKING, NOMINAL_PLANNER_ONLY or any 
     * combination of PLAN_WITH_VORONOI_PULL, PLAN_WITH_NARROW_PASSAGE_PUSH and PLAN_WITH_ANYTIME_HEURISTIC,
     * UNIDIRECTIONAL_PLANNING or BIDIRECTIONAL_PLANNING, and USE_BRANCH_AND_BOUND_PRUNING_FLAG. 
     * See path_planner_options.hpp documentation.
     * \param aPlanningMethodFlags The integer flags that identify various options to use with this planner (see path_planner_options.hpp).
     */
    virtual void set_planning_method_flags(std::size_t aPlanningMethodFlags) { m_planning_method_flags = aPlanningMethodFlags; };
    
    
    /**
     * Returns the steer progress tolerance (in the topology's distance metric) used by this planner when steering.
     * \return The steer progress tolerance used by this planner.
     */
    double get_steer_progress_tolerance() const { return m_steer_progress_tol; };
    /**
     * Sets the steer progress tolerance (in the topology's distance metric) to be used by this planner when steering.
     * \param aSteerProgressTolerance The steer progress tolerance to be used by this planner when making connections.
     */
    void set_steer_progress_tolerance(double aSteerProgressTolerance) { m_steer_progress_tol = aSteerProgressTolerance; };
    
    /**
     * Returns the connection tolerance (in the topology's distance metric) used by this planner when making connections.
     * \return The connection tolerance used by this planner.
     */
    double get_connection_tolerance() const { return m_connection_tol; };
    /**
     * Sets the connection tolerance (in the topology's distance metric) to be used by this planner when making connections.
     * \param aConnectionTolerance The connection tolerance to be used by this planner when making connections.
     */
    void set_connection_tolerance(double aConnectionTolerance) { m_connection_tol = aConnectionTolerance; };
    
    /**
     * Returns the sampling radius (in the topology's distance metric) used by this planner when doing random walks.
     * \return The sampling radius used by this planner.
     */
    double get_sampling_radius() const { return m_sampling_radius; };
    /**
     * Sets the sampling radius (in the topology's distance metric) to be used by this planner when doing random walks.
     * \param aSamplingRadius The sampling radius to be used by this planner when doing random walks.
     */
    void set_sampling_radius(double aSamplingRadius) { m_sampling_radius = aSamplingRadius; };
    
    /**
     * Returns the dimensionality of the space used by this planner.
     * \return The dimensionality of the space used by this planner.
     */
    std::size_t get_space_dimensionality() const { return m_space_dimensionality; };
    /**
     * Sets the dimensionality of the space used by this planner.
     * \param aSpaceDimensionality The dimensionality of the space used by this planner.
     */
    void set_space_dimensionality(std::size_t aSpaceDimensionality) { m_space_dimensionality = aSpaceDimensionality; };
    
    
    /**
     * Returns a const-reference to the path-planning reporter used by this planner.
     * \return A const-reference to the path-planning reporter used by this planner.
     */
    const any_sbmp_reporter_chain<space_type>& get_reporter() const { return m_reporter; };
    /**
     * Sets the path-planning reporter to be used by this planner.
     * \param aNewReporter The path-planning reporter to be used by this planner.
     */
    void set_reporter(const any_sbmp_reporter_chain<space_type>& aNewReporter = any_sbmp_reporter_chain<space_type>()) { m_reporter = aNewReporter; };
    
    
    /**
     * This function invokes the path-planning reporter to report on the progress of the path-planning
     * solver.
     * \note This function is for internal use by the path-planning algorithm (a visitor callback).
     * \param g The current motion-graph.
     * \param reporter The path-planning reporter used to report on the progress of the path-planning solver.
     */
    template <typename Graph>
    void report_progress(Graph& g) {
      m_iteration_count++;
      if(m_iteration_count % m_progress_interval == 0)
        detail::do_report_progress_impl(*(this->m_space), g, m_reporter);
    };
    
    virtual void report_solution(const solution_record_ptr& srp) {
      m_reporter.draw_solution(*(this->m_space), srp);
    };
    
    /**
     * Returns true if the solver has reached the maximum number of iterations.
     * \return True if the solver has reached the maximum number of iterations.
     */
    bool has_reached_max_iterations() const {
      return (m_iteration_count >= m_max_vertex_count);
    };
    
    
    /**
     * This function is called to reset the internal state of the planner.
     */
    virtual void reset_internal_state() {
      m_iteration_count = 0;
      m_reporter.reset_internal_state();
    };
    
    /**
     * Returns true if the solver should keep on going trying to solve the path-planning problem.
     * \return True if the solver should keep on going trying to solve the path-planning problem.
     */
    virtual bool keep_going() const { return !has_reached_max_iterations(); };
    
    
    /**
     * Parametrized constructor.
     * \param aName The name for this object.
     * \param aWorld A topology which represents the C-free (obstacle-free configuration space).
     * \param aMaxVertexCount The maximum number of samples to generate during the motion planning.
     * \param aProgressInterval The number of new samples between each "progress report".
     * \param aDataStructureFlags An integer flags representing the kind of motion graph data-structure to use in the 
     *                            planning algorithm. Can be ADJ_LIST_MOTION_GRAPH or DVP_ADJ_LIST_MOTION_GRAPH.
     *                            Any combination of those two and of KNN method flags to use for nearest
     *                            neighbor queries in the graph. KNN method flags can be LINEAR_SEARCH_KNN, 
     *                            DVP_BF2_TREE_KNN, DVP_BF4_TREE_KNN, DVP_COB2_TREE_KNN, or DVP_COB4_TREE_KNN.
     *                            See path_planner_options.hpp documentation.
     * \param aPlanningMethodFlags The integer flags that identify various options to use with this planner.
     *                             The options available include EAGER_COLLISION_CHECKING or LAZY_COLLISION_CHECKING, 
     *                             NOMINAL_PLANNER_ONLY or any combination of PLAN_WITH_VORONOI_PULL, 
     *                             PLAN_WITH_NARROW_PASSAGE_PUSH and PLAN_WITH_ANYTIME_HEURISTIC, UNIDIRECTIONAL_PLANNING 
     *                             or BIDIRECTIONAL_PLANNING, and USE_BRANCH_AND_BOUND_PRUNING_FLAG. 
     * \param aSteerProgressTolerance The steer progress tolerance to be used by this planner when making connections.
     * \param aConnectionTolerance The connection tolerance to be used by this planner when making connections.
     * \param aSamplingRadius The sampling radius to be used by this planner when doing random walks.
     * \param aSpaceDimensionality The dimensionality of the space used by this planner.
     * \param aReporter The path-planning reporter to be used by this planner.
     */
    sample_based_planner(const std::string& aName,
                         const shared_ptr< typename base_type::space_type >& aWorld, 
                         std::size_t aMaxVertexCount = 0, 
                         std::size_t aProgressInterval = 0,
                         std::size_t aDataStructureFlags = ADJ_LIST_MOTION_GRAPH | DVP_BF2_TREE_KNN,
                         std::size_t aPlanningMethodFlags = 0,
                         double aSteerProgressTolerance = 0.1,
                         double aConnectionTolerance = 0.1,
                         double aSamplingRadius = 1.0,
                         std::size_t aSpaceDimensionality = 1,
                         const any_sbmp_reporter_chain<space_type>& aReporter = any_sbmp_reporter_chain<space_type>()) :
                         base_type(aName,aWorld), 
                         m_max_vertex_count(aMaxVertexCount), 
                         m_progress_interval(aProgressInterval),
                         m_iteration_count(0),
                         m_data_structure_flags(aDataStructureFlags),
                         m_planning_method_flags(aPlanningMethodFlags),
                         m_steer_progress_tol(aSteerProgressTolerance),
                         m_connection_tol(aConnectionTolerance),
                         m_sampling_radius(aSamplingRadius),
                         m_space_dimensionality(aSpaceDimensionality),
                         m_reporter(aReporter) { };
    
    virtual ~sample_based_planner() { };
    
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/

    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      base_type::save(A,base_type::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(m_max_vertex_count)
        & RK_SERIAL_SAVE_WITH_NAME(m_progress_interval)
        & RK_SERIAL_SAVE_WITH_NAME(m_data_structure_flags)
        & RK_SERIAL_SAVE_WITH_NAME(m_planning_method_flags)
        & RK_SERIAL_SAVE_WITH_NAME(m_steer_progress_tol)
        & RK_SERIAL_SAVE_WITH_NAME(m_connection_tol)
        & RK_SERIAL_SAVE_WITH_NAME(m_sampling_radius)
        & RK_SERIAL_SAVE_WITH_NAME(m_space_dimensionality)
        & RK_SERIAL_SAVE_WITH_NAME(m_reporter);
    };

    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      base_type::load(A,base_type::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(m_max_vertex_count)
        & RK_SERIAL_LOAD_WITH_NAME(m_progress_interval)
        & RK_SERIAL_LOAD_WITH_NAME(m_data_structure_flags)
        & RK_SERIAL_LOAD_WITH_NAME(m_planning_method_flags)
        & RK_SERIAL_LOAD_WITH_NAME(m_steer_progress_tol)
        & RK_SERIAL_LOAD_WITH_NAME(m_connection_tol)
        & RK_SERIAL_LOAD_WITH_NAME(m_sampling_radius)
        & RK_SERIAL_LOAD_WITH_NAME(m_space_dimensionality)
        & RK_SERIAL_LOAD_WITH_NAME(m_reporter);
      m_iteration_count = 0;
    };

    RK_RTTI_MAKE_ABSTRACT_1BASE(self,0xC2460001,1,"sample_based_planner",base_type)
};

};

};

#endif

