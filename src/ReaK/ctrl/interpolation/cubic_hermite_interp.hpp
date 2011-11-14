/**
 * \file cubic_hermite_interp.hpp
 * 
 * This library provides an implementation of a trajectory within a temporal and once-differentiable topology.
 * The trajectory is represented by a set of waypoints and all intermediate points 
 * are computed with a cubic Hermite interpolation (cubic hermite spline, or cspline).
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date October 2011
 */

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

#ifndef REAK_CUBIC_HERMITE_INTERP_HPP
#define REAK_CUBIC_HERMITE_INTERP_HPP

#include "path_planning/spatial_trajectory_concept.hpp"

#include "path_planning/differentiable_space_concept.hpp"

#include "interpolated_trajectory.hpp"

#include "lin_alg/arithmetic_tuple.hpp"

#include <boost/config.hpp>
#include <boost/concept_check.hpp>
#include <cmath>

#include <list>
#include <map>
#include <limits>
#include "lin_alg/mat_num_exceptions.hpp"

namespace ReaK {

namespace pp {
  
  
namespace detail {
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::less< 
      Idx, 
      boost::mpl::size_t<2> 
    >,
  void >::type cubic_hermite_interpolate_HOT_impl(PointType& result, const PointDiff1& dv1v0, const PointDiff1& d_ldp1p0_v0,
                                                  const DiffSpace& space, const TimeSpace& t_space,
				 	          double t_factor, double t_normal) {
    /* nothing to do. */
  };
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::equal_to< 
      Idx, 
      boost::mpl::size_t<2> 
    >,
  void >::type cubic_hermite_interpolate_HOT_impl(PointType& result, const PointDiff1& dv1v0, const PointDiff1& d_ldp1p0_v0,
                                                  const DiffSpace& space, const TimeSpace& t_space,
				 	          double t_factor, double t_normal) {
    get<2>(result) = get_space<2>(space,t_space).adjust( 
      lift_to_space<2>(dv1v0, t_factor, space, t_space),
      (6.0 - 12.0 * t_normal) * get_space<2>(space,t_space).difference( lift_to_space<2>( d_ldp1p0_v0, t_factor, space, t_space), 
									lift_to_space<2>( 0.5 * dv1v0, t_factor, space, t_space)));
  };
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::equal_to< 
      Idx, 
      boost::mpl::size_t<3> 
    >,
  void >::type cubic_hermite_interpolate_HOT_impl(PointType& result, const PointDiff1& dv1v0, const PointDiff1& d_ldp1p0_v0,
                                                  const DiffSpace& space, const TimeSpace& t_space,
				 	          double t_factor, double t_normal) {
    cubic_hermite_interpolate_HOT_impl< boost::mpl::size_t<2>, PointType, PointDiff1, DiffSpace, TimeSpace >(result,dv1v0,d_ldp1p0_v0,space,t_space,t_factor,t_normal);
    
    get<3>(result) = lift_to_space<3>(-12.0 * get_space<2>(space, t_space).difference( lift_to_space<2>( d_ldp1p0_v0, t_factor, space, t_space), 
										       lift_to_space<2>( 0.5 * dv1v0, t_factor, space, t_space)),t_factor, space, t_space);
  };
  
  
  
  template <typename Idx, typename PointType, typename PointDiff0, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::less< 
      Idx, 
      boost::mpl::size_t<4> 
    >,
  void >::type cubic_hermite_interpolate_impl(PointType& result, const PointType& a, const PointType& b,
					      const PointDiff0& dp1p0, const PointDiff1& dv1v0, const PointDiff1 d_ldp1p0_v0,
                                              const DiffSpace& space, const TimeSpace& t_space,
					      double t_factor, double t_normal) {

    double t2 = t_normal * t_normal;
    double t3 = t_normal * t2;
    
    get<0>(result) = get_space<0>(space,t_space).adjust(get<0>(a), 
      (3.0 * t2 - 2.0 * t3) * dp1p0
      + (t_normal - t2 * 2.0 + t3) * descend_to_space<0>(get<1>(a),t_factor, space, t_space)
      + (t3 - t2) * descend_to_space<0>(get<1>(b), t_factor, space, t_space) );
    
    get<1>(result) = get_space<1>(space,t_space).adjust(get<1>(a),
      ((t_normal - t2) * 6.0) * d_ldp1p0_v0
      - (2.0 * t_normal - 3.0 * t2) * dv1v0);
    
    cubic_hermite_interpolate_HOT_impl< Idx, PointType, PointDiff1, DiffSpace, TimeSpace >(result, dv1v0, d_ldp1p0_v0, space, t_space, t_factor, t_normal);
    
  };
  
  template <typename Idx, typename PointType, typename PointDiff0, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::greater< 
      Idx, 
      boost::mpl::size_t<3> 
    >,
  void >::type cubic_hermite_interpolate_impl(PointType& result, const PointType& a, const PointType& b,
					      const PointDiff0& dp1p0, const PointDiff1& dv1v0, const PointDiff1 d_ldp1p0_v0,
                                              const DiffSpace& space, const TimeSpace& t_space,
					      double t_factor, double t_normal) {
    cubic_hermite_interpolate_impl< typename boost::mpl::prior<Idx>::type, PointType, DiffSpace, TimeSpace >(result,a,b,dp1p0,dv1v0,d_ldp1p0_v0,space,t_space,t_factor,t_normal);
    
    get< Idx::type::value >(result) = get_space< Idx::type::value >(space,t_space).origin();
  };
  
};



/**
 * This function template computes a cubic Hermite interpolation between two points in a 
 * temporal and once-differentiable topology.
 * \tparam PointType The point type on the temporal and once-differentiable topology.
 * \tparam Topology The temporal and once-differentiable topology type.
 * \param a The starting point of the interpolation.
 * \param b The ending point of the interpolation.
 * \param t The time value at which the interpolated point is sought.
 * \param space The space on which the points reside.
 * \return The interpolated point at time t, between a and b.
 */
template <typename PointType, typename Topology>
PointType cubic_hermite_interpolate(const PointType& a, const PointType& b, double t, const Topology& space) {
  BOOST_CONCEPT_ASSERT((TemporalSpaceConcept<Topology>));
  typedef typename temporal_topology_traits< Topology >::space_topology SpaceType;
  typedef typename temporal_topology_traits< Topology >::time_topology TimeSpaceType;
  BOOST_CONCEPT_ASSERT((DifferentiableSpaceConcept< SpaceType, 1, TimeSpaceType >));
  
  double t_factor = b.time - a.time;
  if(std::fabs(t_factor) < std::numeric_limits<double>::epsilon())
    throw singularity_error("Normalizing factor in cubic Hermite spline is zero!");
  double t_normal = (t - a.time) / (b.time - a.time);
      
  PointType result;
  result.time = t;
  
  typedef typename derived_N_order_space<SpaceType,TimeSpaceType,0>::type Space0;
  typedef typename derived_N_order_space<SpaceType,TimeSpaceType,1>::type Space1;
    
  typedef typename metric_topology_traits<Space0>::point_type PointType0;
  typedef typename metric_topology_traits<Space1>::point_type PointType1;
  
  typedef typename metric_topology_traits<Space0>::point_difference_type PointDiff0;
  typedef typename metric_topology_traits<Space1>::point_difference_type PointDiff1;
    
  PointDiff0 dp1p0 = get_space<0>(space.get_space_topology(),space.get_time_topology()).difference( get<0>(b.pt), get<0>(a.pt) );
  PointDiff1 dv1v0 = get_space<1>(space.get_space_topology(),space.get_time_topology()).difference( get<1>(b.pt), get<1>(a.pt) );
  PointDiff1 d_ldp1p0_v0 = get_space<1>(space.get_space_topology(),space.get_time_topology()).difference( lift_to_space<1>(dp1p0, t_factor, space.get_space_topology(), space.get_time_topology()), get<1>(a.pt));
  
  detail::cubic_hermite_interpolate_impl<boost::mpl::size_t<differentiable_space_traits< SpaceType >::order> >(result.pt, a.pt, b.pt, dp1p0, dv1v0, d_ldp1p0_v0, space.get_space_topology(), space.get_time_topology(), t_factor, t_normal);
  
  return result;
};





/**
 * This functor class implements a cubic Hermite interpolation in a temporal and once-differentiable 
 * topology.
 * \tparam TemporalTopology The temporal topology on which the interpolation is done.
 */
template <typename Factory>
class cubic_hermite_interpolator {
  public:
    typedef cubic_hermite_interpolator<Factory> self;
    typedef typename Factory::point_type point_type;
    typedef typename Factory::topology topology;
  
    typedef typename derived_N_order_space< typename temporal_topology_traits<topology>::space_topology,
                                            typename temporal_topology_traits<topology>::time_topology,0>::type Space0;
    typedef typename metric_topology_traits<Space0>::point_type PointType0;
    typedef typename metric_topology_traits<Space0>::point_difference_type PointDiff0;
    typedef typename derived_N_order_space< typename temporal_topology_traits<topology>::space_topology,
                                            typename temporal_topology_traits<topology>::time_topology,1>::type Space1;
    typedef typename metric_topology_traits<Space1>::point_type PointType1;
    typedef typename metric_topology_traits<Space1>::point_difference_type PointDiff1;
  
  private:
    const Factory* parent;
    const point_type* start_point;
    const point_type* end_point;
    PointDiff0 delta_first_order;
    PointDiff1 delta_second_order;
    PointDiff1 delta_lifted_first_and_second;
    
    void update_delta_value() {
      if(parent && start_point && end_point) {
	double t_factor = end_point->time - start_point->time;
        delta_first_order = get_space<0>(parent->get_temporal_space()->get_space_topology(),parent->get_temporal_space()->get_time_topology()).difference( get<0>(end_point->pt), get<0>(start_point->pt) );
	delta_second_order = get_space<1>(parent->get_temporal_space()->get_space_topology(),parent->get_temporal_space()->get_time_topology()).difference( get<1>(end_point->pt), get<1>(start_point->pt) );
	delta_lifted_first_and_second = get_space<1>(parent->get_temporal_space()->get_space_topology(),parent->get_temporal_space()->get_time_topology()).difference( lift_to_space<1>(delta_first_order, t_factor, parent->get_temporal_space()->get_space_topology(), parent->get_temporal_space()->get_time_topology()), get<1>(start_point->pt));
      };
    };
  
  public:
    
    
    /**
     * Default constructor.
     */
    cubic_hermite_interpolator(const Factory* aParent = NULL, const point_type* aStart = NULL, const point_type* aEnd = NULL) :
                               parent(aParent), start_point(aStart), end_point(aEnd) {
      update_delta_value();
    };
    
    void set_segment(const point_type* aStart, const point_type* aEnd) {
      start_point = aStart;
      end_point = aEnd;
      update_delta_value();
    };
    
    const point_type* get_start_point() const { return start_point; };
    const point_type* get_end_point() const { return end_point; };
    
    template <typename DistanceMetric>
    double travel_distance_to(const point_type& pt, const DistanceMetric& dist) const {
      BOOST_CONCEPT_ASSERT((DistanceMetricConcept<DistanceMetric,topology>));
      if(parent && start_point)
	return dist(pt, *start_point, *(parent->get_temporal_space()));
      else
	return 0.0;
    };
    
    template <typename DistanceMetric>
    double travel_distance_from(const point_type& pt, const DistanceMetric& dist) const {
      BOOST_CONCEPT_ASSERT((DistanceMetricConcept<DistanceMetric,topology>));
      if(parent && end_point)
	return dist(*end_point, pt, *(parent->get_temporal_space()));
      else
	return 0.0;
    };
    
    point_type get_point_at_time(double t) const {
      if(!parent || !start_point || !end_point)
	return point_type();
      double t_factor = end_point->time - start_point->time;
      if(std::fabs(t_factor) < std::numeric_limits<double>::epsilon())
        throw singularity_error("Normalizing factor in cubic Hermite spline is zero!");
      double t_normal = (t - start_point->time) / t_factor;
      
      point_type result;
      result.time = t;
      
      detail::cubic_hermite_interpolate_impl<boost::mpl::size_t<differentiable_space_traits< typename temporal_topology_traits<topology>::space_topology >::order> >(result.pt, start_point->pt, end_point->pt, delta_first_order, delta_second_order, delta_lifted_first_and_second, parent->get_temporal_space()->get_space_topology(), parent->get_temporal_space()->get_time_topology(), t_factor, t_normal);
  
      return result;   
    };
    
};


/**
 * This class is a factory class for cubic Hermite interpolators on a temporal differentiable space.
 * \tparam TemporalTopology The temporal topology on which the interpolation is done, should model TemporalSpaceConcept.
 */
template <typename TemporalTopology>
class cubic_hermite_interp_factory : public serialization::serializable {
  public:
    typedef cubic_hermite_interp_factory<TemporalTopology> self;
    typedef TemporalTopology topology;
    typedef typename temporal_topology_traits<TemporalTopology>::point_type point_type;
    typedef cubic_hermite_interpolator<self> interpolator_type;
  
    BOOST_CONCEPT_ASSERT((TemporalSpaceConcept<TemporalTopology>));
    BOOST_CONCEPT_ASSERT((DifferentiableSpaceConcept< typename temporal_topology_traits<TemporalTopology>::space_topology, 1, typename temporal_topology_traits<TemporalTopology>::time_topology >));
  private:
    const topology* p_space;
  public:
    cubic_hermite_interp_factory(const topology* aPSpace = NULL) : p_space(aPSpace) { };
  
    void set_temporal_space(const topology* aPSpace) { p_space = aPSpace; };
    const topology* get_temporal_space() const { return p_space; };
  
    interpolator_type create_interpolator(const point_type* pp1, const point_type* pp2) const {
      return interpolator_type(this, pp1, pp2);
    };
  
  
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/
    
    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const { };

    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) { };

    RK_RTTI_MAKE_ABSTRACT_1BASE(self,0xC2430002,1,"cubic_hermite_interp_factory",serialization::serializable)
};




  
/**
 * This class implements a trajectory in a temporal and once-differentiable topology.
 * The trajectory is represented by a set of waypoints and all intermediate points 
 * are computed with a cubic Hermite interpolation. This class models the SpatialTrajectoryConcept.
 * \tparam Topology The topology type on which the points and the path can reside, should model the TemporalSpaceConcept and the DifferentiableSpaceConcept (order 1 with space against time).
 * \tparam DistanceMetric The distance metric used to assess the distance between points in the path, should model the DistanceMetricConcept.
 */
template <typename Topology, typename DistanceMetric = default_distance_metric>
class cubic_hermite_interp_traj : public interpolated_trajectory<Topology,cubic_hermite_interp_factory<Topology>,DistanceMetric> {
  public:
    
    BOOST_CONCEPT_ASSERT((TemporalSpaceConcept<Topology>));
    BOOST_CONCEPT_ASSERT((DifferentiableSpaceConcept< typename temporal_topology_traits<Topology>::space_topology, 1, typename temporal_topology_traits<Topology>::time_topology >));
    
    typedef cubic_hermite_interp_traj<Topology,DistanceMetric> self;
    typedef interpolated_trajectory<Topology,cubic_hermite_interp_factory<Topology>,DistanceMetric> base_class_type;
    
    typedef typename base_class_type::topology topology;
    typedef typename base_class_type::distance_metric distance_metric;
    typedef typename base_class_type::point_type point_type;
    
    
  public:
    /**
     * Constructs the path from a space, assumes the start and end are at the origin 
     * of the space.
     * \param aSpace The space on which the path is.
     * \param aDist The distance metric functor that the path should use.
     */
    explicit cubic_hermite_interp_traj(const topology& aSpace, const distance_metric& aDist = distance_metric()) : 
                                       base_class_type(aSpace, aDist, cubic_hermite_interp_factory<Topology>(&aSpace)) { };
    
    /**
     * Constructs the path from a space, the start and end points.
     * \param aSpace The space on which the path is.
     * \param aStart The start point of the path.
     * \param aEnd The end-point of the path.
     * \param aDist The distance metric functor that the path should use.
     */
    cubic_hermite_interp_traj(const topology& aSpace, const point_type& aStart, const point_type& aEnd, const distance_metric& aDist = distance_metric()) :
                              base_class_type(aSpace, aStart, aEnd, aDist, cubic_hermite_interp_factory<Topology>(&aSpace)) { };
			
    /**
     * Constructs the path from a range of points and their space.
     * \tparam ForwardIter A forward-iterator type for getting points to initialize the path with.
     * \param aBegin An iterator to the first point of the path.
     * \param aEnd An iterator to the second point of the path.
     * \param aSpace The space on which the path is.
     * \param aDist The distance metric functor that the path should use.
     */
    template <typename ForwardIter>
    cubic_hermite_interp_traj(ForwardIter aBegin, ForwardIter aEnd, const topology& aSpace, const distance_metric& aDist = distance_metric()) : 
                              base_class_type(aBegin, aEnd, aSpace, aDist, cubic_hermite_interp_factory<Topology>(&aSpace)) { };
    
};



};

};

#endif








