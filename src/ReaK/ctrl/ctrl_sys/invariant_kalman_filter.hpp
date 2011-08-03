
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

#ifndef INVARIANT_KALMAN_FILTER_HPP
#define INVARIANT_KALMAN_FILTER_HPP

#include "belief_state_concept.hpp"
#include "discrete_linear_sss_concept.hpp"
#include "invariant_system_concept.hpp"

#include <boost/utility/enable_if.hpp>
#include <math/vect_concepts.hpp>
#include <math/mat_alg.hpp>
#include <math/mat_cholesky.hpp>

#include <boost/static_assert.hpp>
#include "covariance_concept.hpp"

namespace ReaK {

namespace ctrl {




template <typename InvariantSystem, 
          typename BeliefState, 
	  typename SystemNoiseCovariance>
typename boost::enable_if_c< is_continuous_belief_state<BeliefState>::value &&
                             (belief_state_traits<BeliefState>::representation == belief_representation::gaussian) &&
                             (belief_state_traits<BeliefState>::distribution == belief_distribution::unimodal),
void >::type invariant_kalman_predict(const InvariantSystem& sys,
				      BeliefState& b,
				      const typename discrete_sss_traits<InvariantSystem>::input_type& u,
				      const SystemNoiseCovariance& Q,
				      typename discrete_sss_traits<InvariantSystem>::time_type t = 0) {
  //here the requirement is that the system models a linear system which is at worse a linearized system
  // - if the system is LTI or LTV, then this will result in a basic Kalman Filter (KF) update
  // - if the system is linearized, then this will result in an Extended Kalman Filter (EKF) update
  boost::function_requires< DiscreteLinearSSSConcept< InvariantSystem, DiscreteLinearizedSystemType > >();
  boost::function_requires< InvariantDiscreteSystemConcept<InvariantSystem> >();
  boost::function_requires< ContinuousBeliefStateConcept<BeliefState> >();

  typedef typename discrete_sss_traits<InvariantSystem>::point_type StateType;
  typedef typename discrete_sss_traits<InvariantSystem>::output_type OutputType;
  typedef typename continuous_belief_state_traits<BeliefState>::covariance_type CovType;
  typedef typename covariance_mat_traits< CovType >::matrix_type MatType;
  typedef typename mat_traits<MatType>::value_type ValueType;
  typedef typename invariant_system_traits<InvariantSystem>::invariant_frame_type InvarFrame;
  
  typename discrete_linear_sss_traits<InvariantSystem>::matrixA_type A;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixB_type B;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixC_type C;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixD_type D;
  
  typename invariant_system_traits<InvariantSystem>::invariant_error_type e;
  
  StateType x = b.get_mean_state();
  MatType P = b.get_covariance().get_matrix();
  sys.get_linear_blocks(A, B, C, D, t, x, u);
  
  StateType x_prior = sys.get_next_state(x,u,t);
  InvarFrame W = sys.get_invariant_prior_frame(x, x_prior, u, t + sys.get_time_step());
  P = W * (( A * P * transpose(A)) + B * Q.get_matrix() * transpose(B)) * transpose(W);
  b.set_mean_state( x_prior );
  b.set_covariance( CovType( P ) );
};





template <typename InvariantSystem, 
          typename BeliefState, 
	  typename MeasurementNoiseCovariance>
typename boost::enable_if_c< is_continuous_belief_state<BeliefState>::value &&
                             (belief_state_traits<BeliefState>::representation == belief_representation::gaussian) &&
                             (belief_state_traits<BeliefState>::distribution == belief_distribution::unimodal),
void >::type invariant_kalman_update(const InvariantSystem& sys,
					  BeliefState& b,
					  const typename discrete_sss_traits<InvariantSystem>::input_type& u,
					  const typename discrete_sss_traits<InvariantSystem>::output_type& z,
					  const MeasurementNoiseCovariance& R,
					  typename discrete_sss_traits<InvariantSystem>::time_type t = 0) {
  //here the requirement is that the system models a linear system which is at worse a linearized system
  // - if the system is LTI or LTV, then this will result in a basic Kalman Filter (KF) update
  // - if the system is linearized, then this will result in an Extended Kalman Filter (EKF) update
  boost::function_requires< DiscreteLinearSSSConcept< InvariantSystem, DiscreteLinearizedSystemType > >();
  boost::function_requires< InvariantDiscreteSystemConcept<InvariantSystem> >();
  boost::function_requires< ContinuousBeliefStateConcept<BeliefState> >();

  typedef typename discrete_sss_traits<InvariantSystem>::point_type StateType;
  typedef typename discrete_sss_traits<InvariantSystem>::output_type OutputType;
  typedef typename continuous_belief_state_traits<BeliefState>::covariance_type CovType;
  typedef typename covariance_mat_traits< CovType >::matrix_type MatType;
  typedef typename mat_traits<MatType>::value_type ValueType;
  typedef typename invariant_system_traits<InvariantSystem>::invariant_frame_type InvarFrame;
  
  typename discrete_linear_sss_traits<InvariantSystem>::matrixA_type A;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixB_type B;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixC_type C;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixD_type D;
  
  StateType x = b.get_mean_state();
  MatType P = b.get_covariance().get_matrix();
  sys.get_linear_blocks(A, B, C, D, t, x, u);
  
  typename invariant_system_traits<InvariantSystem>::invariant_error_type e = 
    sys.get_invariant_error(x, u, z, t + sys.get_time_step());
  
  mat< ValueType, mat_structure::rectangular, mat_alignment::column_major > CP = C * P;
  mat< ValueType, mat_structure::symmetric > S(CP * transpose(C) + R.get_matrix());
  linsolve_Cholesky(S,CP);
  mat< ValueType, mat_structure::rectangular, mat_alignment::row_major > K = transpose_move(CP);
   
  b.set_mean_state( sys.apply_correction(x, K * e, u, t + sys.get_time_step()) );
  W = sys.get_invariant_posterior_frame(x, b.get_mean_state(), u, t + sys.get_time_step());
  b.set_covariance( CovType( MatType( W * ((mat< ValueType, mat_structure::identity>(K.get_row_count()) - K * C) * P) * transpose(W) ) ) );
};




template <typename InvariantSystem, 
          typename BeliefState, 
	  typename SystemNoiseCovariance,
	  typename MeasurementNoiseCovariance>
typename boost::enable_if_c< is_continuous_belief_state<BeliefState>::value &&
                             (belief_state_traits<BeliefState>::representation == belief_representation::gaussian) &&
                             (belief_state_traits<BeliefState>::distribution == belief_distribution::unimodal),
void >::type invariant_kalman_filter_step(const InvariantSystem& sys,
					  BeliefState& b,
					  const typename discrete_sss_traits<InvariantSystem>::input_type& u,
					  const typename discrete_sss_traits<InvariantSystem>::output_type& z,
					  const SystemNoiseCovariance& Q,
					  const MeasurementNoiseCovariance& R,
					  typename discrete_sss_traits<InvariantSystem>::time_type t = 0) {
  //here the requirement is that the system models a linear system which is at worse a linearized system
  // - if the system is LTI or LTV, then this will result in a basic Kalman Filter (KF) update
  // - if the system is linearized, then this will result in an Extended Kalman Filter (EKF) update
  boost::function_requires< DiscreteLinearSSSConcept< InvariantSystem, DiscreteLinearizedSystemType > >();
  boost::function_requires< InvariantDiscreteSystemConcept<InvariantSystem> >();
  boost::function_requires< ContinuousBeliefStateConcept<BeliefState> >();

  typedef typename discrete_sss_traits<InvariantSystem>::point_type StateType;
  typedef typename discrete_sss_traits<InvariantSystem>::output_type OutputType;
  typedef typename continuous_belief_state_traits<BeliefState>::covariance_type CovType;
  typedef typename covariance_mat_traits< CovType >::matrix_type MatType;
  typedef typename mat_traits<MatType>::value_type ValueType;
  typedef typename invariant_system_traits<InvariantSystem>::invariant_frame_type InvarFrame;
  
  typename discrete_linear_sss_traits<InvariantSystem>::matrixA_type A;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixB_type B;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixC_type C;
  typename discrete_linear_sss_traits<InvariantSystem>::matrixD_type D;
  
  typename invariant_system_traits<InvariantSystem>::invariant_error_type e;
  
  StateType x = b.get_mean_state();
  MatType P = b.get_covariance().get_matrix();
  sys.get_linear_blocks(A, B, C, D, t, x, u);
  
  StateType x_prior = sys.get_next_state(x,u,t);
  InvarFrame W = sys.get_invariant_prior_frame(x, x_prior, u, t + sys.get_time_step());
  P = W * (( A * P * transpose(A)) + B * Q.get_matrix() * transpose(B)) * transpose(W);
  
  e = sys.get_invariant_error(x_prior, u, z, t + sys.get_time_step());
  
  mat< ValueType, mat_structure::rectangular, mat_alignment::column_major > CP = C * P;
  mat< ValueType, mat_structure::symmetric > S(CP * transpose(C) + R.get_matrix());
  linsolve_Cholesky(S,CP);
  mat< ValueType, mat_structure::rectangular, mat_alignment::row_major > K = transpose_move(CP);
   
  b.set_mean_state( sys.apply_correction(x_prior, K * e, u, t + sys.get_time_step()) );
  W = sys.get_invariant_posterior_frame(x_prior, b.get_mean_state(), u, t + sys.get_time_step());
  b.set_covariance( CovType( MatType( W * ((mat< ValueType, mat_structure::identity>(K.get_row_count()) - K * C) * P) * transpose(W) ) ) );
};





template <typename LinearSystem,
          typename BeliefState = gaussian_belief_state< decomp_covariance_matrix< typename discrete_sss_traits<LinearSystem>::point_type > >,
          typename SystemNoiseCovar = covariance_matrix< typename discrete_sss_traits< LinearSystem >::input_type >,
          typename MeasurementCovar = covariance_matrix< typename discrete_sss_traits< LinearSystem >::output_type > >
struct IKF_belief_transfer {
  typedef IKF_belief_transfer<LinearSystem, BeliefState> self;
  typedef BeliefState belief_state;
  typedef LinearSystem state_space_system;
  typedef typename discrete_sss_traits< state_space_system >::time_type time_type;
  typedef typename discrete_sss_traits< state_space_system >::time_difference_type time_difference_type;

  typedef typename belief_state_traits< belief_state >::state_type state_type;
  typedef typename continuous_belief_state_traits<BeliefState>::covariance_type covariance_type;
  typedef typename covariance_mat_traits< covariance_type >::matrix_type matrix_type;

  typedef typename discrete_sss_traits< state_space_system >::input_type input_type;
  typedef typename discrete_sss_traits< state_space_system >::output_type output_type;

  const LinearSystem* sys;
  SystemNoiseCovar Q;
  MeasurementCovar R;

  IKF_belief_transfer(const LinearSystem& aSys, 
                       const SystemNoiseCovar& aQ,
                       const MeasurementCovar& aR) : sys(&aSys), Q(aQ), R(aR) { };
  
  time_difference_type get_time_step() const { return sys->get_time_step(); };

  const state_space_system& get_ss_system() const { return *sys; };

  belief_state get_next_belief(belief_state b, const time_type& t, const input_type& u, const input_type& y) const {
    invariant_kalman_filter_step(*sys,b,u,y,Q,R,t);
    return b;
  };
  
  belief_state predict_belief(belief_state b, const time_type& t, const input_type& u) const {
    invariant_kalman_predict(*sys,b,u,Q,t);
    return b;
  };
  
  belief_state prediction_to_ML_belief(belief_state b, const time_type& t, const input_type& u) const {
    invariant_kalman_update(*sys,b,u,sys->get_output(b.get_mean_state(),u,t),R,t);
    return b;
  };
  
  belief_state predict_ML_belief(belief_state b, const time_type& t, const input_type& u) const {
    invariant_kalman_predict(*sys,b,u,Q,t);
    invariant_kalman_update(*sys,b,u,sys->get_output(b.get_mean_state(),u,t),R,t + sys->get_time_step());
    return b;
  };
  
};








};

};


#endif










