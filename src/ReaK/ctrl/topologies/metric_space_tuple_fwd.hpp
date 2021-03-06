/**
 * \file metric_space_tuple_fwd.hpp
 * 
 * This library provides classes that define a metric-space tuple class template. A metric-space tuple is 
 * a simple association of several topologies (metric-spaces) which, in turn, also models a metric-space
 * (conditional upon each underlying spaces being a metric-space as well).
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date September 2011
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

#ifndef REAK_METRIC_SPACE_TUPLE_FWD_HPP
#define REAK_METRIC_SPACE_TUPLE_FWD_HPP

#include <ReaK/core/base/defs.hpp>
#include <ReaK/core/lin_alg/arithmetic_tuple.hpp>
#include "metric_space_concept.hpp"
#include "reversible_space_concept.hpp"

#include "tuple_distance_metrics.hpp"
#include "default_random_sampler.hpp"

#include <boost/mpl/bool_fwd.hpp>
#include <boost/mpl/and.hpp>

namespace ReaK {

namespace pp {
  
namespace detail {
  
  template <std::size_t Size, typename SpaceTuple>
  struct is_metric_symmetric_tuple_impl;
  
  template <typename SpaceTuple>
  struct is_metric_symmetric_tuple_impl< 0, SpaceTuple > : boost::mpl::true_ { };
  
  template <std::size_t Size, typename SpaceTuple>
  struct is_metric_symmetric_tuple_impl : 
    boost::mpl::and_<
      is_metric_symmetric_tuple_impl<Size-1, SpaceTuple>,
      is_metric_symmetric< typename arithmetic_tuple_element<Size-1, SpaceTuple>::type >
    > { };
  
  template <typename SpaceTuple>
  struct is_metric_symmetric_tuple : is_metric_symmetric_tuple_impl< arithmetic_tuple_size<SpaceTuple>::type::value, SpaceTuple > { };
  
  
  template <std::size_t Size, typename SpaceTuple>
  struct is_reversible_space_tuple_impl;
  
  template <typename SpaceTuple>
  struct is_reversible_space_tuple_impl< 0, SpaceTuple > : boost::mpl::true_ { };
  
  template <std::size_t Size, typename SpaceTuple>
  struct is_reversible_space_tuple_impl : 
    boost::mpl::and_<
      is_reversible_space_tuple_impl<Size-1, SpaceTuple>,
      is_reversible_space< typename arithmetic_tuple_element<Size-1, SpaceTuple>::type >
    > { };
  
  template <typename SpaceTuple>
  struct is_reversible_space_tuple : is_reversible_space_tuple_impl< arithmetic_tuple_size<SpaceTuple>::type::value, SpaceTuple > { };
  
};
  
  

template <typename SpaceTuple, typename TupleDistanceMetric = manhattan_tuple_distance >
class metric_space_tuple;


template <typename SpaceTuple, typename TupleDistanceMetric>
struct is_metric_space< metric_space_tuple<SpaceTuple, TupleDistanceMetric> > : boost::mpl::true_ { };

template <typename SpaceTuple, typename TupleDistanceMetric>
struct is_reversible_space< metric_space_tuple<SpaceTuple, TupleDistanceMetric> > : 
  detail::is_reversible_space_tuple<SpaceTuple> { };

template <typename SpaceTuple, typename TupleDistanceMetric>
struct is_point_distribution< metric_space_tuple<SpaceTuple, TupleDistanceMetric> > : boost::mpl::true_ { };

template <typename SpaceTuple, typename TupleDistanceMetric>
struct is_metric_symmetric< metric_space_tuple<SpaceTuple, TupleDistanceMetric> > : 
  boost::mpl::and_< 
    is_metric_symmetric<TupleDistanceMetric>,
    detail::is_metric_symmetric_tuple<SpaceTuple>
  > { };


template <typename SpaceType, std::size_t N, typename TupleDistanceMetric = manhattan_tuple_distance >
struct metric_space_array;


};

};


namespace ReaK {
  
  
/* Specialization, see general template docs. */
  template <typename SpaceTuple, typename TupleDistanceMetric>
  struct arithmetic_tuple_size< pp::metric_space_tuple<SpaceTuple,TupleDistanceMetric> > : 
    arithmetic_tuple_size< SpaceTuple > { };
  
  
/* Specialization, see general template docs. */
  template <int Idx, typename SpaceTuple, typename TupleDistanceMetric>
  struct arithmetic_tuple_element< Idx, pp::metric_space_tuple<SpaceTuple,TupleDistanceMetric> > {
    typedef typename arithmetic_tuple_element< Idx, SpaceTuple >::type type;
  };
  
/* Specialization, see general template docs. */
  template <int Idx, typename SpaceTuple, typename TupleDistanceMetric>
  struct arithmetic_tuple_element< Idx, const pp::metric_space_tuple<SpaceTuple,TupleDistanceMetric> > {
    typedef typename arithmetic_tuple_element< Idx, const SpaceTuple >::type type;
  };
  
/* Specialization, see general template docs. */
  template <int Idx, typename SpaceTuple, typename TupleDistanceMetric>
  struct arithmetic_tuple_element< Idx, volatile pp::metric_space_tuple<SpaceTuple,TupleDistanceMetric> > {
    typedef typename arithmetic_tuple_element< Idx, volatile SpaceTuple >::type type;
  };
  
/* Specialization, see general template docs. */
  template <int Idx, typename SpaceTuple, typename TupleDistanceMetric>
  struct arithmetic_tuple_element< Idx, const volatile pp::metric_space_tuple<SpaceTuple,TupleDistanceMetric> > {
    typedef typename arithmetic_tuple_element< Idx, const volatile SpaceTuple >::type type;
  };
  
};

#endif








